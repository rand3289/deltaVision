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
    int camID = 0;
};


void mouse(int event, int x, int y, int flags, void* userdata){
    if( EVENT_LBUTTONUP != event ) { return; }
}


int getVideoSrcCount(cv::VideoCapture& cam){ // how many video sources are there in the current system?
    int i = 0;
    for( ; cam.open(i); ++i ) {
        cam.release(); 
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
    cout << "Found " << camCount << " video sources" << endl;
    cout.flush();

    FiberInfo info;
    info.camID = camCount; // for use with files, set the trackbar to the last value (non-existent device)
    cam.open( --info.camID ); // convert device count to the largest existing zero based device index and open it

    const char dataWin[] = "grayscale";
    namedWindow(dataWin);
    moveWindow(dataWin,  10, 50);
    setMouseCallback(dataWin, mouse, (void*)&info);

    const char deltaWin[] = "delta";
    namedWindow(deltaWin);
    moveWindow(deltaWin,  600, 300);


    const cv::Scalar colorR = CV_RGB(255,0,0); // const cv::Scalar color(0,0,255);
//    const cv::Scalar colorG = CV_RGB(0,255,0);
//    const cv::Scalar colorB = CV_RGB(0,0,255);

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
    }

    double data [img.rows][img.cols];
    for(int y =0; y < img.rows; ++y){
        for(int x=0; x<img.cols; ++x){
           data[y][x] = 127;
        }
    }

    while( cam.read(img) ){
        cv::cvtColor(img, imgGrey, cv::COLOR_BGR2GRAY);

        stringstream ss;
	ss << "s to save, q to quit. fps=" << std::setprecision(3) << getFps();
	const cv::Point xy(5,img.rows-5);
	cv::putText(img, ss.str(), xy, 0, 0.4, colorR);
        imshow(dataWin, img);


        for(int y=0; y< imgGrey.rows; ++y){
            for(int x=0; x < imgGrey.cols; ++x){
                int delta = imgGrey.at<uchar>(y,x) - imgPrev.at<uchar>(y,x);
                data[y][x] += delta;
		int pix = data[y][x];
                imgOut.at<uchar>(y,x) = pix>255 ? 255 : pix;
            }
        }

        imgPrev = imgGrey.clone();
        imshow(deltaWin, imgOut);

        char c = waitKey(1);
	const char ESCAPE = 27;
        if('q'==c || 'Q'==c || ESCAPE==c) { break; }
    }
    cam.release();
}
