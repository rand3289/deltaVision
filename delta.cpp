// Displays how video looks when only changes in pixel values and not values themselves are used.
// This program requires OpenCV library to compile and link.
#include <opencv2/opencv.hpp>
#include <iostream>
#include <iomanip> // format stream IO
#include <cstring> // memset()
#include <chrono>
using namespace std;
using namespace cv;


struct WinInfo {
    int algorithm = 0;
};


void mouse(int event, int x, int y, int flags, void* userdata){
    if( EVENT_LBUTTONUP != event ) { return; }
    WinInfo* data = static_cast<WinInfo*>(userdata);
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


double getFps(){ // frames per second
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
    if( !cam.isOpened() ){
        cerr << "ERROR: could not open camera." << endl;
        exit(1);
    }

    const char dataWin[] = "original";
    namedWindow(dataWin);
    moveWindow(dataWin,  10, 50);

    const char deltaWin[] = "delta";
    namedWindow(deltaWin);
    moveWindow(deltaWin,  600, 300);

    WinInfo info1; // one for each window
    WinInfo info2;
    setMouseCallback(dataWin,  mouse, (void*)&info1); 
    setMouseCallback(deltaWin, mouse, (void*)&info2);

    cv::Mat img;
    cv::Mat imgGrey;
    cv::Mat imgPrev;
    cv::Mat imgOut;

    if( cam.read(img) ){ // init two greyscale Mat imgOut and imgPrev
        cv::cvtColor(img, imgOut, cv::COLOR_BGR2GRAY);
        imgPrev = imgOut.clone();
        imgPrev = 0; // clear the image
    } else {
        cerr << "ERROR: could not read an image from camera." << endl;
        exit(2);
    }

    double data [img.rows][img.cols]; // use the resolution of an aquired image
    memset(data, 0, sizeof(data));

    while( cam.read(img) ){
        cv::cvtColor(img, imgGrey, cv::COLOR_BGR2GRAY); // convert to grayscale

        for(int y=0; y< imgGrey.rows; ++y){
            for(int x=0; x < imgGrey.cols; ++x){
		int pix = 0;
                bool checkBounds = true;
                double delta = imgGrey.at<uchar>(y,x) - imgPrev.at<uchar>(y,x);

		switch(info2.algorithm){
		    default: info2.algorithm = 0; // fall through to case 0
                    case 0: pix = 127-delta; break;
                    case 1: pix = delta;  checkBounds = false; break;
                    case 2:
                        data[y][x] += abs(delta); // dt starts with UNINITIALIZED (random) data !!!
                        pix = data[y][x];
                        data[y][x] = pix / 2;
                        break;
		}

		if(checkBounds){ // allow 8 bit overflow?
                    pix = pix > 255 ? 255 : (pix < 0 ? 0 : pix); 
                }
                imgOut.at<uchar>(y,x) = pix;
            }
        }

        imgPrev = imgGrey.clone();

        stringstream ss;
	ss << "Q to quit.  algorithm=" << info2.algorithm << "  fps=" << std::setprecision(3) << getFps();
	const cv::Point xy(5,img.rows-10);
        const cv::Scalar colorW = CV_RGB(255,255,255);
        Mat curr = info1.algorithm % 2 ? img : imgGrey; // output is changed by left mouse click
	cv::putText(curr, ss.str(), xy, 0, 0.8, colorW);
        imshow(dataWin, curr);
        imshow(deltaWin, imgOut);

        char c = waitKey(1);
	const char ESCAPE = 27;
        if('q'==c || 'Q'==c || ESCAPE==c) { break; }
    }
    cam.release();
}
