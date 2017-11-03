#pragma once
#include "stdafx.h"

#include <locale>
#include <codecvt>
#include "opencv2/opencv.hpp"
#include "opencv2/xfeatures2d.hpp"

//for std::map<cv::Point, ...> which is an ordered map
//unordered_map more appropriate but must make a hash function like x ^ y or using boost::hash_combine to make a hash_value function
namespace cv
{
	bool operator<(cv::Point const& a, cv::Point const& b)
	{
		return (a.x < b.x) || (a.x == b.x && a.y < b.y);
	}
}

//opencv layer

struct ProcessParams
{
	bool bGrayScale;
	int iMinWidth;
	int iMinHeight;
	int iMaxFrames;
	int iCalibrationTimeWindow;
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

struct FeatureDataPoint
{
	int iLastFrame;
	std::vector<cv::KeyPoint> kps;
	std::vector<int> descs;
	bool isOscilPoint;
};

void VideoCleanup(LPVOID p)
{
	if (p == NULL) return;
	cv::VideoCapture* pvc = (cv::VideoCapture*)p;
	pvc->release();
	delete pvc;
}

typedef void (ProgressFunc)(int Frame, int TotalFrames);

cv::Mat GetVideoFrame(LPVOID p, int iFrameNum, cv::Mat & bkgnd, cv::Mat & fgnd, std::function<ProgressFunc> pf, int* pCancel)
{
	ProcessParams pp = ProcessParams{ true, 800, 600, 1000, 10, true, 500, 16, true, 3, 5, 0, 0, true, true, true };
	cv::Mat frame, fgimg;
	cv::VideoCapture* pvc = (cv::VideoCapture*)p;
	double dwWidthInPixels, dwHeightInPixels, dFPS, dFrameCount;
	cv::Ptr<cv::BackgroundSubtractor> bg;
	std::vector<std::vector<cv::Point>> contours;
	HBITMAP hbit = NULL;
	int UseEvery, FrameCount = 0;
	if (pvc->isOpened()) {
		dFPS = pvc->get(CV_CAP_PROP_FPS);
		dFrameCount = pvc->get(CV_CAP_PROP_FRAME_COUNT);
		dwWidthInPixels = pvc->get(CV_CAP_PROP_FRAME_WIDTH);
		dwHeightInPixels = pvc->get(CV_CAP_PROP_FRAME_HEIGHT);
		bg = pp.bMOG ? (cv::Ptr<cv::BackgroundSubtractor>)cv::createBackgroundSubtractorMOG2(pp.history, pp.threshold, pp.bShadowDetection) : (cv::Ptr<cv::BackgroundSubtractor>)cv::createBackgroundSubtractorKNN(pp.history, pp.threshold, pp.bShadowDetection);
		if (pp.bMOG) {
			((cv::BackgroundSubtractorMOG2*)bg.get())->setNMixtures(pp.nMixtures);
		}
		UseEvery = floor(dFPS / pp.dDesiredFPS); if (UseEvery == 0) UseEvery++;
		iFrameNum *= UseEvery;
		pvc->set(CV_CAP_PROP_POS_FRAMES, (iFrameNum > 500) ? iFrameNum - 500 : 0); //CV_CAP_PROP_POS_MSEC
	}
	cv::Ptr<cv::Feature2D> featDetector;
	cv::Ptr<cv::DescriptorMatcher> matcher;
	featDetector = cv::xfeatures2d::SIFT::create();
	matcher = cv::FlannBasedMatcher::create();
	while (pvc->isOpened()) {
		*pvc >> frame; //calls read which is the same as grab/retrieve combined
		if (pvc->get(CV_CAP_PROP_POS_FRAMES) == iFrameNum + 1) {
			cv::Mat detFrame = frame.clone();
			if (pp.iMinHeight != 0 || pp.iMinWidth != 0) cv::resize(detFrame, detFrame, cv::Size(pp.iMinWidth, pp.iMinHeight), 0, 0, cv::INTER_AREA); //cv::INTER_LANCZOS4, cv:INTER_LINEAR for zooming, cv::INTER_CUBIC slow for zooming, cv::INTER_AREA for shrinking
			if (pp.bGrayScale) cvtColor(detFrame, detFrame, CV_BGRA2GRAY);
			bg->apply(detFrame, fgimg);
			cv::erode(fgimg, fgimg, cv::Mat(), cv::Point(-1, -1), 2);
			cv::dilate(fgimg, fgimg, cv::Mat(), cv::Point(-1, -1), 2);

			cv::findContours(fgimg, contours, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE);
			//cv::findContours(fgimg, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);

			std::vector<cv::KeyPoint> kps;
			cv::Mat descs;
			cv::Mat mask = cv::Mat::zeros(cv::Size(pp.iMinWidth, pp.iMinHeight), CV_8UC1);
			drawContours(mask, contours, -1, cv::Scalar(255), CV_FILLED);
			featDetector->detectAndCompute(detFrame, mask, kps, descs);
			std::vector<cv::DMatch> matches;
			matcher->match(descs, matches);

			bg.get()->getBackgroundImage(bkgnd);
			cvtColor(bkgnd, bkgnd, pp.bGrayScale ? CV_GRAY2RGBA : CV_BGRA2RGBA);

			//fgnd = fgimg.clone();
			cvtColor(fgimg, fgnd, CV_GRAY2RGBA);

			if (contours.size() != 0) cv::drawContours(frame, contours, -1, cv::Scalar(0, 0, 255), 2);
			cv::drawKeypoints(frame, kps, frame);
			//cv::drawMatches(frame, kps, frame, kps, matches, frame);

			cvtColor(frame, frame, CV_BGRA2RGBA);

			return frame;
		} else if (!*pCancel && frame.dims != 0 && FrameCount != pp.iMaxFrames) {
			if (FrameCount % UseEvery == 0) {
				if (pf) pf(FrameCount, 500);
				if (pp.iMinHeight != 0 || pp.iMinWidth != 0) cv::resize(frame, frame, cv::Size(pp.iMinWidth, pp.iMinHeight), 0, 0, cv::INTER_AREA); //cv::INTER_LANCZOS4, cv:INTER_LINEAR for zooming, cv::INTER_CUBIC slow for zooming, cv::INTER_AREA for shrinking
				if (pp.bGrayScale) cvtColor(frame, frame, CV_BGRA2GRAY);
				bg->apply(frame, fgimg);
				cv::erode(fgimg, fgimg, cv::Mat(), cv::Point(-1, -1), 2);
				cv::dilate(fgimg, fgimg, cv::Mat(), cv::Point(-1, -1), 2);
			}
		} else break;
		FrameCount++;
	}
	return cv::Mat();
}

#define DIRCHANGE_THRESHOLD 4

bool check_oscillation(std::vector<cv::KeyPoint> kpoints, cv::Point2f ReferencePoint)
{
	//breathing monitor is a very general application, could deal with sleep difficulties for individuals monitoring their own sleep, or doctors monitoring patients, at home or at the clinical setting
	//breathing is a concave activity, reference point should be centroid and not outside where it would be on the border of the image
	std::vector<cv::KeyPoint>::iterator kpit;
	bool directionAway = false;
	int dirChanges = -1;
	double lastdist;
	for (kpit = kpoints.begin(); kpit != kpoints.end(); kpit++) {
		//most distant point is the base - but not from centroid of this set, centroid of all points meaning the reference point
		//cv::Moments m = cv::moments(kpoints);
		//cv::Point centroid = cv::Point(m.m10 / m.m00, m.m01 / m.m00);
		//cv::Vec4f line; //[vx, vy, x, y] vector and point on line
		//fit to a line
		//cv::fitLine(kpoints, line, cv::DIST_L2, 0, 0.01, 0.01);
		//check by verifying that is going away from or towards refrence point in sequence
		double dist = cv::norm(ReferencePoint - kpit->pt);
		if (kpit != kpoints.begin() && lastdist != dist) {
			if (directionAway != (lastdist < dist) || dirChanges == -1) {
				directionAway = (lastdist < dist);
				dirChanges++;
			}
		}
		lastdist = dist;
		if (dirChanges == DIRCHANGE_THRESHOLD) break;
	}
	return (kpit != kpoints.end());
}

double get_oscillation(std::vector<cv::KeyPoint> kpoints, cv::Point2f ReferencePoint, bool bLastOnly, std::vector<double> & netVals)
{
	//int baseIdx;
	std::transform(kpoints.begin(), kpoints.end(), std::back_inserter(netVals), [ReferencePoint](cv::KeyPoint& v) { return cv::norm(ReferencePoint - v.pt); });
	//baseIdx = std::min_element(pts.begin(), pts.end()) - pts.begin();
	double base = *std::min_element(netVals.begin(), netVals.end());
	if (bLastOnly) return netVals.back() - base;
	std::vector<double>::iterator it;
	for (it = netVals.begin(); it != netVals.end(); it++) {
		*it -= base;
	}
	return 0;
}

void ProcessVideo(LPCTSTR szFileName, LPVOID & p, std::vector<double> & motionDetected, std::vector<double> & oscillations, double & dMaxContourSize, double & dMaxOscillation, std::function<ProgressFunc> pf, int* pCancel)
{
	ProcessParams pp = ProcessParams{ true, 800, 600, 1000, 10, true, 500, 16, true, 3, 5, 0, 0, true, true, true };
	using convert_type = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_type, wchar_t> converter;

	cv::Mat frame, fgimg;
	std::string str = converter.to_bytes(szFileName);
	double dwWidthInPixels, dwHeightInPixels, dFPS, dFrameCount, dSumContourSize;
	cv::Ptr<cv::BackgroundSubtractor> bg;
	std::vector<std::vector<cv::Point>> contours;
	std::vector<double> areas;
	std::vector<double> oscCount;
	cv::VideoCapture* pvc = new cv::VideoCapture();
	p = (LPVOID)pvc;
	int UseEvery, FrameCount = 0;
	if (pvc->open(str)) {
		dFPS = pvc->get(CV_CAP_PROP_FPS);
		dFrameCount = pvc->get(CV_CAP_PROP_FRAME_COUNT);
		dwWidthInPixels = pvc->get(CV_CAP_PROP_FRAME_WIDTH);
		dwHeightInPixels = pvc->get(CV_CAP_PROP_FRAME_HEIGHT);
		bg = pp.bMOG ? (cv::Ptr<cv::BackgroundSubtractor>)cv::createBackgroundSubtractorMOG2(pp.history, pp.threshold, pp.bShadowDetection) : (cv::Ptr<cv::BackgroundSubtractor>)cv::createBackgroundSubtractorKNN(pp.history, pp.threshold, pp.bShadowDetection);
		if (pp.bMOG) {
			((cv::BackgroundSubtractorMOG2*)bg.get())->setNMixtures(pp.nMixtures);
		}
		UseEvery = floor(dFPS / pp.dDesiredFPS); if (UseEvery == 0) UseEvery++;
	}
	dMaxContourSize = 0;
	std::vector<std::vector<std::vector<cv::Point>>> contourWindow;
	std::map<cv::Point, FeatureDataPoint> fdps;
	cv::Ptr<cv::Feature2D> featDetector;
	cv::Ptr<cv::DescriptorMatcher> matcher;
	std::vector<cv::KeyPoint> lastkps;
	cv::Mat lastdescs;
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

	//featDetector = cv::xfeatures2d::SIFT::create();
	featDetector = cv::xfeatures2d::SURF::create();
	
	//cv::BFMatcher::create();
	matcher = cv::FlannBasedMatcher::create();
	//matcher = cv::FlannBasedMatcher::FlannBasedMatcher();

	while (pvc->isOpened()) {
		/*while (vc.grab()) {
			cv::OutputArray img();
			vc.retrieve(img);
		}*/
		*pvc >> frame; //calls read which is the same as grab/retrieve combined
		//CV_CAP_PROP_FRAME_COUNT, CV_CAP_PROP_FORMAT, CV_CAP_PROP_POS_MSEC, CV_CAP_PROP_POS_FRAMES
		//if (!vc.isOpened()) break;
		if (!*pCancel && frame.dims != 0 && FrameCount != pp.iMaxFrames) {
			if (FrameCount % UseEvery == 0) {
				if (pf) pf(FrameCount, dFrameCount);
				if (pp.iMinHeight != 0 || pp.iMinWidth != 0) cv::resize(frame, frame, cv::Size(pp.iMinWidth, pp.iMinHeight), 0, 0, cv::INTER_AREA); //cv::INTER_LANCZOS4, cv:INTER_LINEAR for zooming, cv::INTER_CUBIC slow for zooming, cv::INTER_AREA for shrinking
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
					dMaxContourSize = std::max(dMaxContourSize, *std::max_element(areas.begin(), areas.end()) / (double)(dwWidthInPixels * dwHeightInPixels) * 100.0F);
					dSumContourSize = (cv::sum(cv::InputArray(areas))).val[0] / (double)(dwWidthInPixels * dwHeightInPixels) * 100.0F;
				} else {
					dSumContourSize = 0;
				}
				motionDetected.push_back(dSumContourSize);

				oscillations.push_back(0);
				oscCount.push_back(0);

				contourWindow.push_back(contours);
				if (contourWindow.size() > pp.iCalibrationTimeWindow * pp.dDesiredFPS) {
					contourWindow.erase(contourWindow.begin(), contourWindow.begin());
				}
				std::vector<cv::KeyPoint> kps;
				cv::Mat descs;
				cv::Mat mask = cv::Mat::zeros(cv::Size(pp.iMinWidth, pp.iMinHeight), CV_8UC1);
				//should make a convex hull to get a more inclusive shape around the contours...
				for (int i = 0; i < contourWindow.size(); i++) {
					drawContours(mask, contourWindow[i], -1, cv::Scalar(255), CV_FILLED);
				}
				featDetector->detectAndCompute(frame, mask, kps, descs);
				if (lastkps.size() != 0) {
					std::vector<cv::DMatch> matches;
					std::map<cv::Point, FeatureDataPoint> newfdps;
					matcher->match(descs, lastdescs, matches);
					for (std::vector<cv::DMatch>::iterator curMatch = matches.begin(); curMatch != matches.end(); curMatch++) {
						//filter for periodic moving feature points -> direction, opposite direction, direction, opposite direction
						//based on reference point - 2 breaths/oscillations - include only if meets criteria
						std::map<cv::Point, FeatureDataPoint>::iterator it;
						if ((it = fdps.find(lastkps[curMatch->trainIdx].pt)) != fdps.end()) {
							it->second.kps.push_back(kps[curMatch->queryIdx]);
							//take centroid of all points or to start use center of image
							cv::Point2f refpt(dwWidthInPixels / 2, dwHeightInPixels / 2);
							std::vector<double> netVals;
							if (!it->second.isOscilPoint && check_oscillation(it->second.kps, refpt)) {
								get_oscillation(it->second.kps, refpt, false, netVals);
								it->second.isOscilPoint = true;
								oscillations.back() += netVals.back();
								oscCount.back()++;
							} else if (it->second.isOscilPoint) {
								double lastval = get_oscillation(it->second.kps, refpt, false, netVals);
								oscillations.back() += lastval;
								oscCount.back()++;
							}
							if (it->second.kps.size() > pp.iCalibrationTimeWindow * pp.dDesiredFPS) {
								it->second.kps.erase(it->second.kps.begin(), it->second.kps.begin());
							}
							newfdps[kps[curMatch->queryIdx].pt] = FeatureDataPoint{ FrameCount, it->second.kps, std::vector<int>(), it->second.isOscilPoint };
						} else {
							std::vector<cv::KeyPoint> ks;
							ks.push_back(kps[curMatch->queryIdx]);
							newfdps[kps[curMatch->queryIdx].pt] = FeatureDataPoint{ FrameCount, std::vector<cv::KeyPoint>(), std::vector<int>(), false };
						}
					}
					//cleans up all stale
					fdps = newfdps;
				}
				lastkps = kps;
				lastdescs = descs;
			}
		} else break;
		FrameCount++;
	}
	dMaxOscillation = 0;
	for (int i = 0; i < oscCount.size(); i++) {
		if (oscCount[i] != 0) oscillations[i] /= oscCount[i];
		dMaxOscillation = std::max(oscillations[i], dMaxOscillation);
	}
}

//portable adapter layer
HBITMAP GetVideoFrameAdapt(LPVOID p, int iFrameNum, HBITMAP & hBackground, HBITMAP & hForeground, std::function<ProgressFunc> pf, int* pCancel)
{
	if (p == NULL) return (HBITMAP)INVALID_HANDLE_VALUE;
	cv::Mat fgnd, bkgnd, bit = GetVideoFrame(p, iFrameNum, bkgnd, fgnd, pf, pCancel);
	hBackground = CreateBitmap(bkgnd.cols, bkgnd.rows, 1, 32, bkgnd.data);
	//hForeground = CreateBitmap(fgnd.cols, fgnd.rows, 1, 1, fgnd.data); //scanline width must be DWORD aligned but it should be here as 640, 480 are multiples of 32!...
	hForeground = CreateBitmap(fgnd.cols, fgnd.rows, 1, 32, fgnd.data);
	return CreateBitmap(bit.cols, bit.rows, 1, 32, bit.data);
}