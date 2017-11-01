#pragma once
#include "stdafx.h"
#include <locale>
#include <codecvt>
#include "opencv2/opencv.hpp"
#include "..\..\..\..\opencv_contrib331\modules\xfeatures2d\include\opencv2\xfeatures2d.hpp"
#include "..\..\..\..\opencv_contrib331\modules\xfeatures2d\include\opencv2\xfeatures2d\nonfree.hpp"

//opencv layer

void VideoCleanup(LPVOID p)
{
	if (p == NULL) return;
	cv::VideoCapture* pvc = (cv::VideoCapture*)p;
	pvc->release();
	delete pvc;
}

cv::Mat GetVideoFrame(LPVOID p, int iFrameNum, cv::Mat & bkgnd, cv::Mat & fgnd)
{
	cv::Mat frame, fgimg;
	cv::VideoCapture* pvc = (cv::VideoCapture*)p;
	double dwWidthInPixels, dwHeightInPixels;
	cv::Ptr<cv::BackgroundSubtractorMOG2> bg;
	std::vector<std::vector<cv::Point>> contours;
	HBITMAP hbit = NULL;
	if (pvc->isOpened()) {
		dwWidthInPixels = pvc->get(CV_CAP_PROP_FRAME_WIDTH);
		dwHeightInPixels = pvc->get(CV_CAP_PROP_FRAME_HEIGHT);
		bg = cv::createBackgroundSubtractorMOG2(); //default history is 500
		bg->setDetectShadows(false);
		bg->setNMixtures(3);
		pvc->set(CV_CAP_PROP_POS_FRAMES, (iFrameNum > 500) ? iFrameNum - 500 : 0); //CV_CAP_PROP_POS_MSEC
	}
	while (pvc->isOpened()) {
		*pvc >> frame; //calls read which is the same as grab/retrieve combined
		if (frame.dims != 0) {
			bg->apply(frame, fgimg);
			cv::erode(fgimg, fgimg, cv::Mat(), cv::Point(-1, -1), 2);
			cv::dilate(fgimg, fgimg, cv::Mat(), cv::Point(-1, -1), 2);
			if (pvc->get(CV_CAP_PROP_POS_FRAMES) == iFrameNum + 1) {
				cv::findContours(fgimg, contours, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE);
				//cv::findContours(fgimg, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);
				bg.get()->getBackgroundImage(bkgnd);
				cvtColor(bkgnd, bkgnd, CV_BGRA2RGBA);

				//fgnd = fgimg.clone();
				cvtColor(fgimg, fgnd, CV_GRAY2RGBA);

				if (contours.size() != 0) cv::drawContours(frame, contours, -1, cv::Scalar(0, 0, 255), 2);
				//cv::drawKeypoints();
				//cv::drawMatches();

				cvtColor(frame, frame, CV_BGRA2RGBA);

				return frame;
			}
		} else break;
	}
	return cv::Mat();
}

struct ProcessParams
{
	bool bGrayScale;
	bool bMOG;
	int history;
	double threshold;
	bool bShadowDetection;
	int nMixtures;
	double dDesiredFPS;
	int featDetector;
	int matcher;
	bool drawContours;
	bool drawFeaturePoints;
	bool drawMatches;
};

void ProcessVideo(LPCTSTR szFileName, LPVOID & p, std::vector<double> & motionDetected, double & dMaxContourSize)
{
	using convert_type = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_type, wchar_t> converter;

	cv::Mat frame, fgimg;
	std::string str = converter.to_bytes(szFileName);
	ProcessParams pp = ProcessParams{ true, true, 500, 16, true, 3, 5, 0, 0, true, true, true };
	double dwWidthInPixels, dwHeightInPixels, dFPS, dSumContourSize;
	cv::Ptr<cv::BackgroundSubtractor> bg;
	std::vector<std::vector<cv::Point>> contours;
	std::vector<double> areas;
	cv::VideoCapture* pvc = new cv::VideoCapture();
	p = (LPVOID)pvc;
	int UseEvery;
	if (pvc->open(str)) {
		dFPS = pvc->get(CV_CAP_PROP_FPS);
		dwWidthInPixels = pvc->get(CV_CAP_PROP_FRAME_WIDTH);
		dwHeightInPixels = pvc->get(CV_CAP_PROP_FRAME_HEIGHT);
		bg = pp.bMOG ? (cv::Ptr<cv::BackgroundSubtractor>)cv::createBackgroundSubtractorMOG2(pp.history, pp.threshold, pp.bShadowDetection) : (cv::Ptr<cv::BackgroundSubtractor>)cv::createBackgroundSubtractorKNN(pp.history, pp.threshold, pp.bShadowDetection);
		if (pp.bMOG) {
			((cv::BackgroundSubtractorMOG2*)bg.get())->setNMixtures(pp.nMixtures);
		}
		UseEvery = floor(dFPS / pp.dDesiredFPS); if (UseEvery == 0) UseEvery++;
	}
	cv::Ptr<cv::Feature2D> featDetector;
	//cv::BRISK::create();
	//cv::ORB::create();
	//cv::MSER::create();
	//featDetector = cv::FastFeatureDetector::create(); //cv::FAST()
	//cv::AgastFeatureDetector::create(); // cv::AGAST()
	//cv::GFTTDetector::create();
	//cv::SimpleBlobDetector::create();
	//cv::KAZE::create();
	//cv::AKAZE::create();

	//cv::xfeatures2d::FREAK::create();
	//cv::xfeatures2d::StarDetector::create();
	//cv::xfeatures2d::BriefDescriptorExtractor::create();
	//cv::xfeatures2d::LUCID::create();
	//cv::xfeatures2d::LATCH::create();
	//cv::xfeatures2d::DAISY::create();
	//cv::xfeatures2d::MSDDetector::create();
	//cv::xfeatures2d::VGG::create();
	//cv::xfeatures2d::BoostDesc::create();
	//cv::xfeatures2d::HarrisLaplaceFeatureDetector::create();
	//cv::xfeatures2d::AffineFeature2D::create(); //wraps other feature detector by doing affine transform
	//cv::xfeatures2d::FASTForPointSet();

	featDetector = cv::xfeatures2d::SIFT::create();
	//cv::xfeatures2d::SURF::create();
	
	cv::Ptr<cv::DescriptorMatcher> matcher;

	//cv::BFMatcher::create();
	matcher = cv::FlannBasedMatcher::create();
	//matcher = cv::FlannBasedMatcher::FlannBasedMatcher();

	int FrameCount = 0;
	while (pvc->isOpened()) {
		/*while (vc.grab()) {
			cv::OutputArray img();
			vc.retrieve(img);
		}*/
		*pvc >> frame; //calls read which is the same as grab/retrieve combined
		//CV_CAP_PROP_FRAME_COUNT, CV_CAP_PROP_FORMAT, CV_CAP_PROP_POS_MSEC, CV_CAP_PROP_POS_FRAMES
		//if (!vc.isOpened()) break;
		if (FrameCount % UseEvery == 0) {
			if (frame.dims != 0) {
				if (pp.bGrayScale) cvtColor(frame, frame, CV_BGRA2GRAY);
				bg->apply(frame, fgimg);
				cv::erode(fgimg, fgimg, cv::Mat(), cv::Point(-1, -1), 2);
				cv::dilate(fgimg, fgimg, cv::Mat(), cv::Point(-1, -1), 2);
				contours.clear();
				cv::findContours(fgimg, contours, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE);
				//cv::findContours(fgimg, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);
				if (contours.size() != 0) {
					areas.clear();
					std::transform(contours.begin(), contours.end(), std::back_inserter(areas), [](std::vector<cv::Point>& v) { return cv::contourArea(v); });
					dMaxContourSize = *std::max_element(areas.begin(), areas.end()) / (double)(dwWidthInPixels * dwHeightInPixels) * 100.0F;
					dSumContourSize = (cv::sum(cv::InputArray(areas))).val[0] / (double)(dwWidthInPixels * dwHeightInPixels) * 100.0F;
				} else {
					dSumContourSize = 0;
				}
				motionDetected.push_back(dSumContourSize);
				std::vector<cv::KeyPoint> kps;
				cv::Mat descs;
				featDetector->detectAndCompute(frame, cv::noArray(), kps, descs);
				std::vector<cv::DMatch> matches;
				matcher->match(descs, matches);
			} else break;
		}
		FrameCount++;
	}
}

//portable adapter layer
HBITMAP GetVideoFrameAdapt(LPVOID p, int iFrameNum, HBITMAP & hBackground, HBITMAP & hForeground)
{
	if (p == NULL) return (HBITMAP)INVALID_HANDLE_VALUE;
	cv::Mat fgnd, bkgnd, bit = GetVideoFrame(p, iFrameNum, bkgnd, fgnd);
	hBackground = CreateBitmap(bkgnd.cols, bkgnd.rows, 1, 32, bkgnd.data);
	//hForeground = CreateBitmap(fgnd.cols, fgnd.rows, 1, 1, fgnd.data); //scanline width must be DWORD aligned but it should be here as 640, 480 are multiples of 32!...
	hForeground = CreateBitmap(fgnd.cols, fgnd.rows, 1, 32, fgnd.data);
	return CreateBitmap(bit.cols, bit.rows, 1, 32, bit.data);
}