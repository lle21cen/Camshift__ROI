#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/core/mat.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <raspicam/raspicam_cv.h>

using namespace std;
using namespace cv;

Mat objectHistogram, globalHistogram, bp;
Mat img_rgb, img_binary, temp_img, img_hsv;
bool isMouseUp = false;
Point p1, p2;

void cbMouse(int event, int x, int y, int flags, void *userdata) {

	if (isMouseUp) return;
	Mat img_temp;
	static bool p2set = false;

	if (event == EVENT_LBUTTONDOWN) {
		p1 = Point(x,y);
	}
	else if (event == EVENT_MOUSEMOVE && flags == EVENT_FLAG_LBUTTON) {
		if (x > img_rgb.size().width)
			x = img_rgb.size().width;
		else if (x < 0)
			x = 0;
		if (y >img_rgb.size().height)
			y = img_rgb.size().height;
		else if (y < 0)
			y = 0;
		p2 = Point(x,y);
		p2set = true;

		img_rgb.copyTo(img_temp);
		rectangle(img_temp, p1, p2, Scalar(255, 255, 255));
		imshow("Video Camera", img_temp);
	}
	else if (event == EVENT_LBUTTONUP && p2set) {
		isMouseUp = true;
	}
}

void makeBinary() {
	Mat gray;
	cvtColor(temp_img, gray, CV_BGR2GRAY);
	threshold(gray, img_binary, 1, 255, THRESH_BINARY);
	
}

void trackingCamShift(Rect search_window) {
	TermCriteria criteria(TermCriteria::COUNT | TermCriteria::EPS,10,1);

	RotatedRect found_object = CamShift(img_binary, search_window, criteria);
	Rect found_rect = found_object.boundingRect();
	rectangle(img_rgb, found_rect, Scalar(255,255,255),1);

	p1 = Point(found_rect.x, found_rect.y);
	p2 = Point(found_rect.x+found_rect.width, found_rect.y+found_rect.height);
	if (found_rect.x < 0)
		p1.x = 0;
	if (found_rect.y < 0)
		p1.y = 0;
	if (found_rect.x + found_rect.width > img_rgb.size().width)
		p2.x = img_rgb.size().width;
	if (found_rect.y + found_rect.height > img_rgb.size().height)
		p2.y = img_rgb.size().height;
}

int main ( int argc,char **argv ) {

	raspicam::RaspiCam_Cv cap;

	cap.set( CV_CAP_PROP_FORMAT, CV_8UC3);
	cap.set( CV_CAP_PROP_FRAME_WIDTH, 480 );
	cap.set( CV_CAP_PROP_FRAME_HEIGHT, 480 );

	if (!cap.open()) {
		cout << "Cannot open raspberry pi camera." << endl;
		return -1;
	}

	namedWindow("Video Camera");
	setMouseCallback("Video Camera", cbMouse, NULL);

	while(1){

		cap.grab();
		cap.retrieve (img_rgb);

		if (isMouseUp) {
			img_rgb.convertTo(temp_img, -1, 0);
			Mat roi(temp_img, Rect(p1,p2));
			Mat logo = img_rgb(Rect(p1,p2));

			logo.copyTo(roi, logo);
			Rect search_window(p1, p2);
			makeBinary();

			cvtColor(img_rgb, img_hsv, CV_BGR2HSV);
			trackingCamShift(search_window);
			imshow( "Video Binary", img_binary);
			imshow("Video temp", temp_img);
		}


		imshow("Video Camera", img_rgb);
		if (waitKey(10) == 27) break;
	}

	cap.release();
	return 0;
}
