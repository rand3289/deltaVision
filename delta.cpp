// This program requires OpenCV library to compile and link
#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>
#include <iomanip> // format stream printing
#include <vector>
#include <algorithm>
#include <chrono>
using namespace std;
using namespace cv;


struct FiberInfo {
    int algorithm = 0;
};


void mouse(int event, int x, int y, int flags, void* userdata){
    if( EVENT_LBUTTONUP != event ) { return; }
    FiberInfo* data = static_cast<FiberInfo*>(userdata);
    ++data->algorithm;
}


int getVideoSrcCount(cv::VideoCapture& cam){ // how many video sources are there in the current system?
    int i = 0;
    while( cam.open(i) ) {
        cam.release();
        ++i;
    }
    return i;
}


double getFps(){
	static int count = 0;
	static double fps = 0;
	static auto start = std::chrono::steady_clock::now();
	auto now = std::chrono::steady_clock::now();
	++count;
	if( now > start + std::chrono::seconds(1) ){
		fps = 1000.0 * (double)count / (double) std::chrono::duration_cast<std::chrono::milliseconds> (now - start).count();
		start = now;
		count = 0;
	}
	return fps;
}


int main(int argc, char* argv[]){
    cv::VideoCapture cam;
    const int camCount = getVideoSrcCount(cam);
    cout << "######## Found " << camCount << " video sources" << endl;
    cout.flush();
    cam.open( camCount-1 ); // convert device count to the largest existing zero based device index and open it

    const char dataWin[] = "original";
    namedWindow(dataWin);
    moveWindow(dataWin,  10, 50);

    const char deltaWin[] = "delta";
    namedWindow(deltaWin);
    moveWindow(deltaWin,  600, 300);

    FiberInfo info1; // for each window
    FiberInfo info2;
    setMouseCallback(dataWin,  mouse, (void*)&info1); 
    setMouseCallback(deltaWin, mouse, (void*)&info2);

    const cv::Scalar colorW = CV_RGB(255,255,255);

    cv::Mat img;
    cv::Mat imgGrey;
    cv::Mat imgPrev;
    cv::Mat imgOut;

    if( !cam.isOpened() ){
        cerr << "ERR: could not open camera." << endl;
        exit(1);
    }

    if( cam.read(img) ){
        cv::cvtColor(img, imgOut, cv::COLOR_BGR2GRAY);
        imgPrev = imgOut.clone();
        imgPrev = 0; // clear the image
    } else {
        cerr << "ERR: could not read an image from camera." << endl;
        exit(2);
    }

    double data [img.rows][img.cols]; // use the resolution of an aquired image
    for(int y =0; y < img.rows; ++y){
        for(int x=0; x<img.cols; ++x){
           data[y][x] = 0;
        }
    }

    while( cam.read(img) ){
        cv::cvtColor(img, imgGrey, cv::COLOR_BGR2GRAY); // convert to grayscale

        for(int y=0; y< imgGrey.rows; ++y){
            for(int x=0; x < imgGrey.cols; ++x){
		int pix = 0;
                bool checkBounds = true;
                double delta = imgGrey.at<uchar>(y,x) - imgPrev.at<uchar>(y,x);

		switch(info2.algorithm){
		    default: info2.algorithm = 0; // fall through to case 0
                    case 0: pix = 127+delta; break;
                    case 1: pix = 127-delta; break;
                    case 2: pix = delta;  checkBounds = false; break;
                    case 3: pix = -delta; checkBounds = false; break;
                    case 4:
                        data[y][x] += abs(delta); // dt starts with UNINITIALIZED (random) data !!!
                        pix = data[y][x];
                        data[y][x] = pix - (pix / 3);
                        pix *= 2;
                        break;
		}
		if(checkBounds){ // allow overflow?
                    pix = pix > 255 ? 255 : (pix < 0 ? 0 : pix); 
                }
                imgOut.at<uchar>(y,x) = pix;
            }
        }

        imgPrev = imgGrey.clone();

        stringstream ss;
	ss << "Q to quit.  fps=" << std::setprecision(3) << getFps() << "  algorithm=" << info2.algorithm;
	const cv::Point xy(5,img.rows-10);
        Mat curr = info1.algorithm % 2 ? img : imgGrey; // algorithm is changed by mouse click
	cv::putText(curr, ss.str(), xy, 0, 0.8, colorW);
        imshow(dataWin, curr);
        imshow(deltaWin, imgOut);

        char c = waitKey(1);
	const char ESCAPE = 27;
        if('q'==c || 'Q'==c || ESCAPE==c) { break; }
    }
    cam.release();
}
