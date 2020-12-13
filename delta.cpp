// This program requires OpenCV library to compile and link
#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>
#include <iomanip> // format stream printing
#include <vector>
#include <algorithm>
using namespace std;
using namespace cv;


struct FiberInfo {
    int camID = 0;
    int minD = 0;
    std::vector<cv::Point2f> fibers;      // detected fiber centers
};



int getVideoSrcCount(cv::VideoCapture& cam){ // how many video sources are there in the current system?
    int i = 0;
    for( ; cam.open(i); ++i ) {
        cam.release(); 
    }
    return i;
}


struct VideoInfo{
	cv::VideoCapture& cam;
	const string& file;
	const int max; // max possible trackbar value
	VideoInfo(cv::VideoCapture& camera, string& fileName, int maxIndex): cam(camera), file(fileName), max(maxIndex) {}
};

void idChanged(int pos, void* userData){ // trackbar callback
	VideoInfo& inf = *static_cast<VideoInfo*>(userData);
	inf.cam.release();
	if( inf.max==pos  &&  inf.file.length() > 0){
	    inf.cam.open(inf.file);
	} else {
	    inf.cam.open(pos);
	}
}


// allow user to add or delete fibers by clicking on the video
void mouse(int event, int x, int y, int flags, void* userdata){
	if( EVENT_LBUTTONUP != event ) { return; }
	FiberInfo* info = static_cast<FiberInfo*> (userdata);
	std::vector<cv::Point2f>& fibers = info->fibers;

	auto found = std::find_if(fibers.begin(), fibers.end(), [=](const cv::Point2f& pt){ 
		const int dx = pt.x - x;
		const int dy = pt.y - y;
		return info->minD > sqrt(dx*dx+dy*dy); // find distance between points (hypotenuse)
	} );
	if(fibers.end() ==  found){
		fibers.emplace_back((float)x, (float)y);
	} else {
		fibers.erase(found);
	}
}


#include <chrono>
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


//    const cv::Scalar colorR = CV_RGB(255,0,0); // const cv::Scalar color(0,0,255);
//    const cv::Scalar colorG = CV_RGB(0,255,0);
    const cv::Scalar colorB = CV_RGB(0,0,255);

    cv::Mat img;
    cv::Mat imgGrey;
    cv::Mat imgOut;

    while( cam.isOpened() ){
        if( !cam.read(img) ){
            cam.release();
        }

        stringstream ss;
	ss << "s to save, q to quit. fps=" << std::setprecision(3) << getFps();
	const cv::Point xy(5,img.rows-5);
	cv::putText(img, ss.str(), xy, 0, 0.4, colorB);
        imshow(dataWin, img);

        cv::cvtColor(img, imgGrey, cv::COLOR_BGR2GRAY);
        imshow(deltaWin, imgGrey);

        char c = waitKey(1);
	const char ESCAPE = 27;
        if('q'==c || 'Q'==c || ESCAPE==c) { break; }
    }
}
