//birth to 6 weeks: 30–40 breaths per minute - average per https://en.wikipedia.org/wiki/Respiratory_rate
//healthy resting adults is much slower, 12–18 breaths per minute
//premature birth would have an even high breath per minute

//FFT, Laplacian pyramids, multithreading technique:
//Detecting breathing rate of a person from video http://algomuse.com/c-c/detecting-breathing-rate-of-a-person-from-video

//Eulerian video magnification technique to find subtle motion

#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <chrono>
#include <thread>
#include <mutex>
#include "opencv2/opencv.hpp"
#include "opencv2/xfeatures2d.hpp"

#define TRACKBARNAME "Frame"
#define WINDOWNAME "VideoOscillation"
#define PAUSEBTN "PauseBtn"

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
	double threshold;
	bool bShadowDetection;
	int nMixtures;
	double dDesiredFPS;
	int featDetector;
	int matcher;
	bool drawContours;
	bool drawFeaturePoints;
	bool drawMatches;
	bool noProcessing;
};

struct VidInfo
{
	double dwWidthInPixels;
	double dwHeightInPixels;
	double dFPS;
	double dFrameCount;
};

struct FeatureDataPoint
{
	int iLastFrame;
	std::vector<cv::KeyPoint> kps;
	std::vector<int> descs;
	bool isOscilPoint;
};

void VideoCleanup(void* p)
{
	if (p == NULL) return;
	cv::VideoCapture* pvc = (cv::VideoCapture*)p;
	pvc->release();
	delete pvc;
}

typedef void (ProgressFunc)(cv::Mat mat, int Frame);

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

//percentage of oscillation range returned using reference point distance to determine minimum distance as base, and maximum distance for %
double get_oscillation(std::vector<cv::KeyPoint> kpoints, cv::Point2f ReferencePoint, bool bLastOnly, std::vector<double> & netVals)
{
	netVals.clear();
	//int baseIdx;
	std::transform(kpoints.begin(), kpoints.end(), std::back_inserter(netVals), [ReferencePoint](cv::KeyPoint& v) { return cv::norm(ReferencePoint - v.pt); });
	//baseIdx = std::min_element(pts.begin(), pts.end()) - pts.begin();
	double base = *std::min_element(netVals.begin(), netVals.end());
	double max = *std::min_element(netVals.begin(), netVals.end());
	if (bLastOnly) return (netVals.back() - base) / max;
	std::vector<double>::iterator it;
	for (it = netVals.begin(); it != netVals.end(); it++) {
		*it = (*it - base) / max;
	}
	return 0;
}

cv::Mat DrawUI(cv::Mat frame, cv::Mat bkgnd, cv::Mat fgnd, std::vector<double> & motionDetected, std::vector<double> & oscillations, std::vector<double> & oscCount, double dMaxContourSize,
	int frameNum, int framesPerSecond, time_t startTime, std::vector<double> & ft, double dScale)
{
	cv::Mat canvas = cv::Mat3b(570, 640, cv::Vec3b(0, 0, 0));
	//480x420, 480x60, 160x210, 160x210
	cv::resize(frame, frame, cv::Size(480, 420), cv::INTER_CUBIC);
	frame.copyTo(canvas(cv::Rect(0, 0, 480, 420)));
	cv::resize(bkgnd, bkgnd, cv::Size(160, 210), cv::INTER_CUBIC);
	bkgnd.copyTo(canvas(cv::Rect(480, 0, 160, 210)));
	cv::resize(fgnd, fgnd, cv::Size(160, 210), cv::INTER_CUBIC);
	fgnd.copyTo(canvas(cv::Rect(480, 210, 160, 210)));

	//FPS
	char buf[256];
	time_t now = time(NULL);
	sprintf(buf, "Frame: %lu", frameNum);
	int baseLine = 0;
	cv::Size s = cv::getTextSize(cv::String(buf), cv::FONT_HERSHEY_PLAIN, 1, 1, &baseLine);
	cv::putText(canvas, cv::String(buf), cv::Point(480 + (160 - s.width) / 2, 210 + 10), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255, 255, 255));
	sprintf(buf, "Time: %lu", frameNum / framesPerSecond);
	s = cv::getTextSize(cv::String(buf), cv::FONT_HERSHEY_PLAIN, 1, 1, &baseLine);
	cv::putText(canvas, cv::String(buf), cv::Point(480 + (160 - s.width) / 2, 210 + 10 + 15), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255, 255, 255));
	sprintf(buf, "Processing FPS: %lu", now == startTime ? 0 : frameNum / (int)(now - startTime));
	s = cv::getTextSize(cv::String(buf), cv::FONT_HERSHEY_PLAIN, 1, 1, &baseLine);
	cv::putText(canvas, cv::String(buf), cv::Point(480 + (160 - s.width) / 2, 210 + 10 + 30), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255, 255, 255));


	if (motionDetected.size() != 0) {
		std::pair<std::vector<double>::iterator, std::vector<double>::iterator> dMinMax = std::minmax_element(motionDetected.begin(), motionDetected.end());
		double d = *dMinMax.second;
		cv::Mat timeline = cv::Mat(1, motionDetected.size(), CV_8UC3);
		for (int i = 0; i < motionDetected.size(); i++) {
			double pct = motionDetected[i] / dMaxContourSize;
			timeline.at<cv::Vec3b>(cv::Point(i, 0)) = cv::Vec3b(pct > 0.5 ? 0 : 255 * (1 - pct * 2), 255 * (pct > 0.5 ? (pct * 2 - 1) : 1), 255);
		}
		cv::resize(timeline, timeline, cv::Size(std::min(640, (int)motionDetected.size()), 30), cv::INTER_MAX);
		timeline.copyTo(canvas(cv::Rect(0, 420, std::min(640, (int)motionDetected.size()), 30)));
	}

	if (oscillations.size() != 0) {
		double dMaxOsc = -1 * std::numeric_limits<double>::infinity(), dMinOsc = std::numeric_limits<double>::infinity();
		cv::Mat timeline = cv::Mat::zeros(30, std::max(640, (int)(oscillations.size() * 1)), CV_8UC3);
		for (int i = 0; i < oscCount.size(); i++) {
			if (oscCount[i] != 0) {
				dMaxOsc = std::max(oscillations[i] / oscCount[i], dMaxOsc);
				dMinOsc = std::min(oscillations[i] / oscCount[i], dMinOsc);
			}
		}
		int breaths = 0;
		if (dMaxOsc != 0) {
			double lastpct = 0;
			for (int i = 0; i < oscillations.size(); i++) {
				if (oscCount[i] != 0) {
					double pct = (dMaxOsc - dMinOsc) == 0 ? 0 : ((oscillations[i] / oscCount[i]) - dMinOsc) / (dMaxOsc - dMinOsc);
					if (i != 0 && lastpct < 0.25 && pct > 0.25) {
						timeline.at<cv::Vec3b>(cv::Point(i * 1, 29 * pct)) = cv::Vec3b(0, 0, 255); breaths++;
					} else if (i != 0 && lastpct > 0.25 && pct < 0.25) {
						timeline.at<cv::Vec3b>(cv::Point(i * 1, 29 * pct)) = cv::Vec3b(0, 255, 0); //breaths++;
					} else {
						timeline.at<cv::Vec3b>(cv::Point(i * 1, 29 * pct)) = cv::Vec3b(255, 255, 255);
					}
					lastpct = pct;
				}
				else lastpct = 0;
			}
		}
		if (oscillations.size() * 1 > 640) cv::resize(timeline, timeline, cv::Size(640, 30), cv::INTER_MAX);
		timeline.copyTo(canvas(cv::Rect(0, 450, 640, 30)));

		char buf[256];
		sprintf(buf, "Breaths: %lu", breaths / 2);
		int baseLine = 0;
		cv::Size s = cv::getTextSize(cv::String(buf), cv::FONT_HERSHEY_PLAIN, 1, 1, &baseLine);
		cv::putText(canvas, cv::String(buf), cv::Point((640 - s.width) / 2, 450 + 10), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255, 255, 255));
	}

	if (ft.size() != 0) {
		std::pair<std::vector<double>::iterator, std::vector<double>::iterator> dMinMax = std::minmax_element(ft.begin(), ft.end());
		double dMin = *dMinMax.first;
		double d = *dMinMax.second;
		cv::Mat timeline = cv::Mat::zeros(30, ft.size() * dScale, CV_8UC3);
		int breaths = 0;
		if (d != 0) {
			for (int i = 0; i < ft.size(); i++) {
				if (ft[i] != 0) {
					double pct = (ft[i] - dMin) / (d - dMin);
					if (i != 0 && i != ft.size() - 1 && (ft[i] > ft[i - 1]) && (ft[i] > ft[i + 1])) {
						timeline.at<cv::Vec3b>(cv::Point(i * dScale, 29 * pct)) = cv::Vec3b(0, 0, 255); breaths++;
					} else if (i != 0 && i != ft.size() - 1 && (ft[i] < ft[i - 1]) && (ft[i] < ft[i + 1])) {
						timeline.at<cv::Vec3b>(cv::Point(i * dScale, 29 * pct)) = cv::Vec3b(0, 255, 0); //breaths++;
					} else {
						timeline.at<cv::Vec3b>(cv::Point(i * dScale, 29 * pct)) = cv::Vec3b(255, 255, 255);
					}
				}
			}
		}
		cv::resize(timeline, timeline, cv::Size(std::min(640, (int)(ft.size() * dScale)), 30), cv::INTER_MAX);
		timeline.copyTo(canvas(cv::Rect(0, 480, std::min(640, (int)(ft.size() * dScale)), 30)));

		char buf[256];
		sprintf(buf, "Breaths: %lu", breaths / 2);
		int baseLine = 0;
		cv::Size s = cv::getTextSize(cv::String(buf), cv::FONT_HERSHEY_PLAIN, 1, 1, &baseLine);
		cv::putText(canvas, cv::String(buf), cv::Point((640 - s.width) / 2, 480 + 10), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255, 255, 255));
	}

	if (motionDetected.size() != 0) {
		//total range of motion area is stable, maximum found (probably should only consider a window here as well)
		//motion detected percentage depends on camera view and if camera moves changes dynamically
		//in a stable case, it depends on the sharpness of features in breathing region
		//must filter out oscillation wave based on 2 thresholds
		cv::Mat timeline = cv::Mat::zeros(30, motionDetected.size() * dScale, CV_8UC3);
		int breaths = 0;
		double lastpct = 0;
		for (int i = 0; i < motionDetected.size(); i++) {
			std::pair<std::vector<double>::iterator, std::vector<double>::iterator> dMinMax = std::minmax_element(motionDetected.begin() + (i < 5 ? 0 : (i - 5)), motionDetected.begin() + i + 1);
			double d = *dMinMax.second;
			//log(exp(N) * Value) / N brings normalized value to logarithmic scale with correctly chosen N factor which is big enough to bring all values above 1
			//log(1+Value) also works
			double pct = d == 0 ? 0 : motionDetected[i] / d; //if (pct > 1.0) pct = 1.0; if (pct > .001) pct = .001; pct *= 1000;
			//pct = pct == 0 ? 0 : log(exp(10) * pct) / 10;
			pct = log(1 + pct * (exp(1) - 1));
			if (i != 0 && (i != motionDetected.size() - 1) && motionDetected[i] > motionDetected[i - 1] && motionDetected[i] > motionDetected[i + 1]) {
				timeline.at<cv::Vec3b>(cv::Point(i * dScale, 29 * pct)) = cv::Vec3b(0, 0, 255); breaths++;
			} else if (i != 0 && (i != motionDetected.size() - 1) && motionDetected[i] < motionDetected[i - 1] && motionDetected[i] < motionDetected[i + 1]) {
				timeline.at<cv::Vec3b>(cv::Point(i * dScale, 29 * pct)) = cv::Vec3b(0, 255, 0); //breaths++;
			} else {
				timeline.at<cv::Vec3b>(cv::Point(i * dScale, 29 * pct)) = cv::Vec3b(255, 255, 255);
			}
			lastpct = pct;
		}
		cv::resize(timeline, timeline, cv::Size(std::min(640, (int)(motionDetected.size() * dScale)), 30), cv::INTER_MAX);
		timeline.copyTo(canvas(cv::Rect(0, 510, std::min(640, (int)(motionDetected.size() * dScale)), 30)));

		char buf[256];
		sprintf(buf, "Breaths: %lu", breaths / 2);
		int baseLine = 0;
		cv::Size s = cv::getTextSize(cv::String(buf), cv::FONT_HERSHEY_PLAIN, 1, 1, &baseLine);
		cv::putText(canvas, cv::String(buf), cv::Point((640 - s.width) / 2, 510 + 10), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255, 255, 255));
	}

	return canvas;
}

void ProcessVideo(VidInfo vi, ProcessParams pp, cv::VideoCapture & pvc, std::vector<double> & motionDetected, std::vector<double> & oscillations, double & dMaxContourSize, double & dMaxOscillation, std::function<ProgressFunc> pf, int* pCancel, int* iJumpFrame, bool* bPaused)
{
	cv::Mat frame, fgimg;
	double dSumContourSize;
	cv::Ptr<cv::BackgroundSubtractor> bg;
	std::vector<std::vector<cv::Point>> contours;
	std::vector<double> areas;
	std::vector<double> oscCount;
	std::vector<double> ft;
	int UseEvery, FrameCount = 0;
	//1440x1080 vidoes or 4:3 aspect ratio, 25fps
	int frameHistory = pp.iCalibrationTimeWindow * pp.dDesiredFPS;
	if (pvc.isOpened()) {
		bg = pp.bMOG ? (cv::Ptr<cv::BackgroundSubtractor>)cv::createBackgroundSubtractorMOG2(frameHistory, pp.threshold, pp.bShadowDetection) : (cv::Ptr<cv::BackgroundSubtractor>)cv::createBackgroundSubtractorKNN(frameHistory, pp.threshold, pp.bShadowDetection);
		if (pp.bMOG) {
			((cv::BackgroundSubtractorMOG2*)bg.get())->setNMixtures(pp.nMixtures);
		}
		UseEvery = (int)floor(vi.dFPS / pp.dDesiredFPS); if (UseEvery == 0) UseEvery++;
	}
	dMaxContourSize = 0;
	std::vector<std::vector<std::vector<cv::Point>>> contourWindow;
	std::map<cv::Point, FeatureDataPoint> fdps;
	cv::Ptr<cv::Feature2D> featDetector;
	cv::Ptr<cv::DescriptorMatcher> matcher;
	std::vector<cv::KeyPoint> lastkps;
	cv::Mat lastdescs;
	cv::Mat bkgnd, fgnd, mask, cumulativeMask = cv::Mat::zeros(cv::Size(pp.iMinWidth, pp.iMinHeight), CV_8UC1);
	std::vector<cv::KeyPoint> kps;
	std::vector<cv::DMatch> matches;
	std::map<cv::Point, FeatureDataPoint> newfdps;
	cv::Mat descs;
	std::map<cv::Point, FeatureDataPoint>::iterator it;
	std::vector<cv::KeyPoint> ks;
	cv::Mat FFTWindow(0, pp.iMinWidth * pp.iMinHeight, CV_8UC1);

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
	((cv::xfeatures2d::SURF*)featDetector.get())->setHessianThreshold(2000);
	
	cv::Mat err, lastframe;
	std::vector<cv::Mat> pyramid, nextpyr;
	std::vector<cv::Point2f> pts, lastpts, nextpts;
	std::vector<uchar> status, nextstatus;
	cv::TermCriteria termcrit(cv::TermCriteria::COUNT | cv::TermCriteria::EPS, 20, 0.03);
	cv::Mat ellipse, refellipse, nextellipse, distellipse, tellipse, calcdistellipse, checkellipse, nextcheckellipse;

	time_t start = time(NULL);
	//cv::BFMatcher::create();
	matcher = cv::FlannBasedMatcher::create();
	//matcher = cv::FlannBasedMatcher::FlannBasedMatcher();
	while (pvc.isOpened()) {
		if (*bPaused) {
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
			if (*bPaused) {
				continue;
			}
		}
		int iJump;
		if ((iJump = *iJumpFrame) != 0) {
			*iJumpFrame = 0;
			pvc.set(CV_CAP_PROP_POS_FRAMES, (iJump > frameHistory) ? iJump - frameHistory : 0); //CV_CAP_PROP_POS_MSEC
			bg = pp.bMOG ? (cv::Ptr<cv::BackgroundSubtractor>)cv::createBackgroundSubtractorMOG2(frameHistory, pp.threshold, pp.bShadowDetection) : (cv::Ptr<cv::BackgroundSubtractor>)cv::createBackgroundSubtractorKNN(frameHistory, pp.threshold, pp.bShadowDetection);
			if (pp.bMOG) {
				((cv::BackgroundSubtractorMOG2*)bg.get())->setNMixtures(pp.nMixtures);
			}
			contourWindow.clear();
			dMaxContourSize = 0;
			motionDetected.clear();
			oscillations.clear();
			oscCount.clear();
			ft.clear();
			pyramid.clear(); nextstatus.clear(); nextpts.clear(); nextellipse = cv::Mat();
			lastkps.clear();
			lastdescs = cv::Mat();
			fdps.clear();
			cumulativeMask = cv::Mat::zeros(cv::Size(pp.iMinWidth, pp.iMinHeight), CV_8UC1);
			FFTWindow = cv::Mat(0, pp.iMinWidth * pp.iMinHeight, CV_8UC1);
			FrameCount = iJump;
			start = time(NULL);
		}

		/*while (vc.grab()) {
			cv::OutputArray img();
			vc.retrieve(img);
		}*/
		pvc >> frame; //calls read which is the same as grab/retrieve combined
		//CV_CAP_PROP_FRAME_COUNT, CV_CAP_PROP_FORMAT, CV_CAP_PROP_POS_MSEC, CV_CAP_PROP_POS_FRAMES
		//if (!vc.isOpened()) break;
		if (!*pCancel && frame.dims != 0 && FrameCount != pp.iMaxFrames) {
			//frame.cols; frame.rows;
			//int chan = frame.channels(), type = frame.type(), depth = frame.depth(); //CV_8UC3
			if (pp.noProcessing) {
				if (pf != NULL) {
					cv::Mat canvas = cv::Mat3b(510, 640, cv::Vec3b(0, 0, 0));
					cv::resize(frame, frame, cv::Size(640, 480), cv::INTER_CUBIC);
					frame.copyTo(canvas(cv::Rect(0, 0, 640, 480)));
					pf(canvas, FrameCount);
				}
				FrameCount++;
				continue;
			}
			else {
				cv::Mat detFrame = frame.clone();
				//conversion for processing
				if (pp.iMinHeight != 0 || pp.iMinWidth != 0) cv::resize(frame, frame, cv::Size(pp.iMinWidth, pp.iMinHeight), 0, 0, cv::INTER_AREA); //cv::INTER_LANCZOS4, cv:INTER_LINEAR for zooming, cv::INTER_CUBIC slow for zooming, cv::INTER_AREA for shrinking
				if (pp.bGrayScale) cvtColor(frame, frame, CV_BGR2GRAY);

				if (oscillations.size() % frameHistory == 0) {
					cv::goodFeaturesToTrack(frame, lastpts, 100, 0.3, 7);
					cv::cornerSubPix(frame, lastpts, cv::Size(10, 10), cv::Size(-1, -1), termcrit);
					cv::buildOpticalFlowPyramid(frame, pyramid, cv::Size(21, 21), 3);
				}
				cv::buildOpticalFlowPyramid(frame, nextpyr, cv::Size(21, 21), 3);
				//best features are the ones closest to the motion window
				cv::calcOpticalFlowPyrLK(pyramid, nextpyr, lastpts, pts, status, err);
				if (oscillations.size() % frameHistory == 0) { nextstatus = status; nextpts = pts; }
				//cv::calcOpticalFlowFarneback(bkgnd, frame, pyramid, 0.5, 3, 15, 3, 5, 1.2, 0);
				double totaldist = 0, totalneg = 0;
				int totalosc = 0, totalnegosc = 0;
				for (int i = 0; i < pts.size(); i++) {
					if (!status[i] || !nextstatus[i]) continue;
					//if (err.at<float>(cv::Point(0, i)) > 2) continue;;
					double dist = cv::norm(pts[i] - nextpts[i]);
					//based on reference point - either fixed in image preferably by gravity vector but hard to determine
					//or based on point in background image, must determine which are positive and negative distance contributors
					if (cv::norm(pts[i] - lastpts[i]) > cv::norm(nextpts[i] - lastpts[i])) {
						totaldist += dist; totalosc++;
					}
					else {
						totalneg += dist; totalnegosc++;
					}
				}
				nextstatus = status;
				nextpts = pts;
				//pyramid = nextpyr;
				oscillations.push_back((totaldist - totalneg));
				oscCount.push_back(std::max(totalosc, totalnegosc));
				if (FrameCount % UseEvery == 0) {

					//motion detection
					bg->apply(frame, fgimg);
					//cv::erode(fgimg, fgimg, cv::Mat(), cv::Point(-1, -1), 2);
					//cv::dilate(fgimg, fgimg, cv::Mat(), cv::Point(-1, -1), 2);
					if (FrameCount == 0) { FrameCount++; continue; }
					contours.clear();
					mask = cv::Mat::zeros(cv::Size(pp.iMinWidth, pp.iMinHeight), CV_8UC1);
					cv::ellipse(mask, cv::Point(pp.iMinWidth / 2, pp.iMinHeight / 2), cv::Size(pp.iMinWidth / 4, pp.iMinHeight / 4), 0, 0, 360, cv::Scalar(255), CV_FILLED);
					cv::bitwise_and(mask, fgimg, fgimg);
					cv::findContours(fgimg, contours, CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE); //CV_RETR_CCOMP, CV_RETR_EXTERNAL   CV_CHAIN_APPROX_NONE
					if (contours.size() != 0) {
						areas.clear();
						std::transform(contours.begin(), contours.end(), std::back_inserter(areas), [](std::vector<cv::Point>& v) { return cv::contourArea(v); });
						dMaxContourSize = std::max(dMaxContourSize, *std::max_element(areas.begin(), areas.end()) / (double)(pp.iMinWidth * pp.iMinHeight) * 100.0F);
						dSumContourSize = (cv::sum(cv::InputArray(areas))).val[0] / (double)(pp.iMinWidth * pp.iMinHeight) * 100.0F;
					}
					else {
						dSumContourSize = 0;
					}
					motionDetected.push_back(dSumContourSize);

					//oscillations.push_back(0);
					//oscCount.push_back(0);

					contourWindow.push_back(contours);
					mask = cv::Mat::zeros(cv::Size(pp.iMinWidth, pp.iMinHeight), CV_8UC1);
					cv::drawContours(mask, contourWindow.back(), -1, cv::Scalar(1), CV_FILLED);
					cumulativeMask += mask;
					if (contourWindow.size() > frameHistory) {
						mask = cv::Mat::zeros(cv::Size(pp.iMinWidth, pp.iMinHeight), CV_8UC1);
						cv::drawContours(mask, contourWindow.front(), -1, cv::Scalar(1), CV_FILLED);
						cumulativeMask -= mask;
						contourWindow.erase(contourWindow.begin(), contourWindow.begin() + 1);
					}

					cv::threshold(cumulativeMask, mask, 0, 255, cv::THRESH_BINARY);
					cv::bitwise_and(frame, mask, ellipse);
					if (ft.size() % frameHistory == 0) nextellipse = ellipse.clone();
					else cv::bitwise_and(lastframe, mask, nextellipse);
					bg.get()->getBackgroundImage(bkgnd);
					cv::bitwise_and(bkgnd, mask, refellipse);

					double totaldist, totalneg;
					cv::absdiff(ellipse, nextellipse, distellipse);
					cv::absdiff(ellipse, refellipse, checkellipse);
					cv::absdiff(nextellipse, refellipse, nextcheckellipse);
					cv::subtract(checkellipse, nextcheckellipse, tellipse, cv::noArray(), CV_16SC1);
					cv::threshold(tellipse, tellipse, 0, 255, cv::THRESH_BINARY);
					tellipse.convertTo(tellipse, CV_8UC1);
					cv::bitwise_and(distellipse, tellipse, calcdistellipse);
					totaldist = cv::sum(calcdistellipse)[0];
					cv::subtract(nextcheckellipse, checkellipse, tellipse, cv::noArray(), CV_16SC1);
					cv::threshold(tellipse, tellipse, 0, 255, cv::THRESH_BINARY);
					tellipse.convertTo(tellipse, CV_8UC1);
					cv::bitwise_and(distellipse, tellipse, calcdistellipse);
					totalneg = cv::sum(calcdistellipse)[0];
					ft.push_back(totaldist - totalneg);
					lastframe = frame.clone();

					mask = fgimg.clone();
					//cv::calcOpticalFlowSF() //contrib optflow module
					//could make a convex hull to get a more inclusive shape around the contours...
					//cv::convexHull()
					/*cv::threshold(cumulativeMask, mask, 0, 255, cv::THRESH_BINARY);
					kps.clear();

					//fourier transform is not particularly applicable because breathing does not have regular frequency or periodicity over short intervals
					//oscillation can be detected by instead of using peaks and troughs, using a threshold between the minimum and maximum after it has been properly scaled

					//feature point tracking
					mask = cv::Mat::zeros(cv::Size(pp.iMinWidth, pp.iMinHeight), CV_8UC1);
					cv::ellipse(mask, cv::Point(pp.iMinWidth / 2, pp.iMinHeight / 2), cv::Size(pp.iMinWidth / 4, pp.iMinHeight / 4), 0, 0, 360, cv::Scalar(255), CV_FILLED);

					featDetector->detectAndCompute(frame, mask, kps, descs);
					if (lastkps.size() != 0 && kps.size() != 0) {
						std::sort(kps.begin(), kps.end(), [](cv::KeyPoint &d1, cv::KeyPoint &d2) -> bool { return d1.pt < d2.pt; });
						kps.erase(std::unique(kps.begin(), kps.end(), [](cv::KeyPoint& p1, cv::KeyPoint& p2) { return p1.pt == p2.pt; }), kps.end());
						matches.clear();
						newfdps.clear();
						matcher->match(descs, lastdescs, matches);
						double minDist = std::min_element(matches.begin(), matches.end(), [](cv::DMatch& d1, cv::DMatch& d2) { return d1.distance < d2.distance; })->distance;
						std::sort(matches.begin(), matches.end(), [](cv::DMatch &d1, cv::DMatch &d2) -> bool { return d1.trainIdx < d2.trainIdx; });
						matches.erase(std::unique(matches.begin(), matches.end(), [](cv::DMatch &d1, cv::DMatch &d2) -> bool { return d1.trainIdx == d2.trainIdx; }), matches.end());
						for (std::vector<cv::DMatch>::iterator curMatch = matches.begin(); curMatch != matches.end(); curMatch++) {
							//if (curMatch->distance > std::max(2 * minDist, 0.02)) continue; //filter noise matches
							//filter for periodic moving feature points -> direction, opposite direction, direction, opposite direction
							//based on reference point - 2 breaths/oscillations - include only if meets criteria
							if ((it = fdps.find(lastkps[curMatch->trainIdx].pt)) != fdps.end()) {
								it->second.kps.push_back(kps[curMatch->queryIdx]);
								//could take centroid of all points or to start use center of image
								//reference point is the further place of gravity, so the bottom center makes perfect sense
								cv::Point2f refpt(pp.iMinWidth / 2, pp.iMinHeight);
								std::vector<double> netVals;
								if (!it->second.isOscilPoint && check_oscillation(it->second.kps, refpt)) {
									//get_oscillation(it->second.kps, refpt, false, netVals);
									//it->second.isOscilPoint = true;
									//oscillations.back() += netVals.back();
									//oscCount.back()++;
								} else if (it->second.isOscilPoint) {
									//double lastval = get_oscillation(it->second.kps, refpt, true, netVals);
									//oscillations.back() += lastval;
									//oscCount.back()++;
								}
								if (it->second.kps.size() > frameHistory) {
									//it->second.kps.erase(it->second.kps.begin(), it->second.kps.begin() + 1);
								}
								newfdps[kps[curMatch->queryIdx].pt] = FeatureDataPoint{ FrameCount, it->second.kps, std::vector<int>(), it->second.isOscilPoint };
							} else {
								ks.clear();
								ks.push_back(kps[curMatch->queryIdx]);
								newfdps[kps[curMatch->queryIdx].pt] = FeatureDataPoint{ FrameCount, ks, std::vector<int>(), false };
							}
						}
						//cleans up all stale
						if (std::any_of(newfdps.begin(), newfdps.end(), [frameHistory](std::pair<cv::Point, FeatureDataPoint> fdp) -> bool { return fdp.second.kps.size() >= frameHistory; })) {
							cv::Mat KpData = cv::Mat(0, frameHistory, CV_64F), MultiMat;
							cv::Point2f refpt(pp.iMinWidth / 2, pp.iMinHeight);
							std::vector<double> netVals;
							for (std::map<cv::Point, FeatureDataPoint>::iterator fdp = newfdps.begin(); fdp != newfdps.end(); fdp++) {
								get_oscillation(fdp->second.kps, refpt, false, netVals);
								netVals.insert(netVals.begin(), frameHistory - netVals.size(), 0);
								KpData.push_back(cv::Mat(1, frameHistory, CV_64F, netVals.data()));
							}
							cv::dft(KpData, MultiMat, cv::DFT_ROWS | cv::DFT_COMPLEX_OUTPUT);
							cv::Mat planes[2];
							cv::split(MultiMat, planes);
							magnitude(planes[0], planes[1], planes[0]);
							//first bin in FFT is DC (0 Hz), second bin Fs / N where Fs = sample rate, N = size of FFT 5 / 50 = .1Hz, 25 * 5 / 50 = 1.5Hz
							//second half > N / 2 is symetrical to first half just complex conjugates
							MultiMat = planes[0].colRange(KpData.cols * .1 / pp.dDesiredFPS - 1, KpData.cols * .9 / pp.dDesiredFPS + 1);
							cv::Mat MaxIndexes;
							cv::Mat Sort = MultiMat.isContinuous() ? MultiMat.reshape(0, 1) : MultiMat.clone().reshape(0, 1);
							cv::sortIdx(Sort, MaxIndexes, CV_SORT_DESCENDING | CV_SORT_EVERY_ROW);
							for (int i = 0; i < 1; i++) { //cannot take more than 1 value without dealing with the phase shift...
								int idx = MaxIndexes.at<int>(cv::Point(i, 0));
								for (int j = 0; j < frameHistory; j++) {
									oscillations.push_back(KpData.at<double>(cv::Point(j, idx / MultiMat.cols)));
									oscCount.push_back(1);
								}
							}
							newfdps.clear();
						}
						fdps = newfdps;
					}
					lastkps = kps;
					lastdescs = descs;

					//color intensity monitoring
					//build laplacian pyramid and select best image?
					cv::Mat ellipse;
					cv::bitwise_and(frame, mask, ellipse);
					FFTWindow.push_back(ellipse.isContinuous() ? ellipse.reshape(0, 1) : ellipse.clone().reshape(0, 1));
					if (FFTWindow.rows >= frameHistory) {
						cv::Mat MultiMat;
						FFTWindow.convertTo(MultiMat, CV_64F);
						int m = cv::getOptimalDFTSize(MultiMat.rows);
						int n = cv::getOptimalDFTSize(MultiMat.cols);
						//cv::copyMakeBorder(MultiMat, MultiMat, 0, m - MultiMat.rows, 0, n - MultiMat.cols, BORDER_CONSTANT, cv::Scalar::all(0));
						cv::dft(MultiMat.t(), MultiMat, cv::DFT_ROWS | cv::DFT_COMPLEX_OUTPUT);
						cv::Mat planes[2];
						cv::split(MultiMat, planes);
						magnitude(planes[0], planes[1], planes[0]);
						//apply a bandpass filter
						//tolerance range 1Hz (1 breath per second) to .1Hz (1 breath every 10 seconds)
						MultiMat = planes[0].colRange(FFTWindow.rows * .1 / pp.dDesiredFPS - 1, FFTWindow.rows * .9 / pp.dDesiredFPS + 1);
						cv::Mat MaxIndexes;
						ellipse = MultiMat.isContinuous() ? MultiMat.reshape(0, 1) : MultiMat.clone().reshape(0, 1);
						cv::sortIdx(ellipse, MaxIndexes, CV_SORT_DESCENDING | CV_SORT_EVERY_ROW);
						for (int j = 0; j < FFTWindow.rows; j++) {
							double d = 0;
							for (int i = 0; i < 1; i++) { //cannot take more than 1 value without dealing with the phase shift...
								int idx = MaxIndexes.at<int>(cv::Point(i, 0));
								//cv::Point pt = cv::Point(idx % MultiMat.cols, idx / MultiMat.cols);
								//double dMax = MultiMat.at<double>(pt);
								d += FFTWindow.at<unsigned char>(cv::Point(idx / MultiMat.cols, j));
							}
							ft.push_back(d);
						}

						FFTWindow = cv::Mat(0, pp.iMinWidth * pp.iMinHeight, CV_8UC1);
						//FFTWindow = FFTWindow.rowRange(1, FFTWindow.rows);
					}*/

					if (pf != NULL)
					{
						bg.get()->getBackgroundImage(bkgnd);
						cv::cvtColor(bkgnd, bkgnd, pp.bGrayScale ? CV_GRAY2RGB : CV_BGR2RGB);

						//fgnd = fgimg.clone();
						cv::cvtColor(fgimg, fgnd, CV_GRAY2RGB);

						contours.clear();
						cv::findContours(mask, contours, CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE); //CV_RETR_CCOMP, CV_RETR_EXTERNAL   CV_CHAIN_APPROX_NONE
						//drawContours(mask, contours, -1, cv::Scalar(255), 2);

						if (contours.size() != 0) {
							if (pp.iMinHeight != 0 || pp.iMinWidth != 0) {
								std::vector<std::vector<cv::Point>> cts;
								std::vector<cv::Point> ct;
								std::transform(contours.begin(), contours.end(), std::back_inserter(cts), [pp, &ct, vi](std::vector<cv::Point>& vec) {
									ct.clear();
									std::transform(vec.begin(), vec.end(), std::back_inserter(ct), [pp, vi](cv::Point& v) {
										return cv::Point(v.x * vi.dwWidthInPixels / pp.iMinWidth, v.y * vi.dwHeightInPixels / pp.iMinHeight);
									});
									return ct;
								});
								cv::drawContours(detFrame, cts, -1, cv::Scalar(0, 0, 255), 2);
							}
							else cv::drawContours(detFrame, contours, -1, cv::Scalar(0, 0, 255), 2);
						}

						if (pp.iMinHeight != 0 || pp.iMinWidth != 0) {
							std::vector<cv::KeyPoint> kpoints;
							std::transform(lastkps.begin(), lastkps.end(), std::back_inserter(kpoints), [pp, vi](cv::KeyPoint& v) { return cv::KeyPoint(cv::Point2f(v.pt.x * vi.dwWidthInPixels / pp.iMinWidth, v.pt.y * vi.dwHeightInPixels / pp.iMinHeight), v.size * vi.dwWidthInPixels / pp.iMinWidth, v.angle, v.response, v.octave, v.class_id); });
							cv::drawKeypoints(detFrame, kpoints, detFrame);
						}
						else cv::drawKeypoints(detFrame, lastkps, detFrame);

						for (int i = 0; i < pts.size(); i++) {
							if (!status[i]) continue;
							if (pp.iMinHeight != 0 || pp.iMinWidth != 0) {
								cv::circle(detFrame, cv::Point2f(pts[i].x * vi.dwWidthInPixels / pp.iMinWidth, pts[i].y * vi.dwHeightInPixels / pp.iMinHeight), 5, cv::Scalar(255, 0, 0), 2);
							}
							else cv::circle(detFrame, pts[i], 5, cv::Scalar(255, 0, 0), 2);
						}

						//cv::drawMatches(frame, kps, frame, kps, matches, frame);

						//cvtColor(detFrame, detFrame, CV_BGR2RGB);

						pf(DrawUI(detFrame, bkgnd, fgnd, motionDetected, oscillations, oscCount, dMaxContourSize, FrameCount, vi.dFPS, start, ft, vi.dFPS / pp.dDesiredFPS), FrameCount);
					}
				}
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

struct StartParams
{
	cv::VideoCapture* pvc;
	VidInfo vi;
	std::vector<double> motionDetect;
	std::vector<double> osc;
	double dMaxContour;
	double dMaxOsc;
	int pc = 0;
	std::queue<cv::Mat> imgQueue;
	std::queue<int> imgFrameNum;
	mutable std::mutex m;
	int iCurPos;
	int iJumpFrame;
	bool bPause;
	std::vector<int> breathPos;
	ProcessParams pp;
};

void VideoProcessor(StartParams& p)
{
	ProcessVideo(p.vi, p.pp, *p.pvc, p.motionDetect, p.osc, p.dMaxContour, p.dMaxOsc,
		std::move(std::function<ProgressFunc>([&p](cv::Mat mat, int Frame) -> void {
		while (p.imgQueue.size() > 10 && !p.pc) {
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		std::lock_guard<std::mutex> lock(p.m);
		p.imgFrameNum.push(Frame);
		p.imgQueue.push(mat);
	})), &p.pc, &p.iJumpFrame, &p.bPause);
}

void ChangeVideoPos(int Pos, void* p)
{
	if (((StartParams*)p)->iCurPos != Pos)
		((StartParams*)p)->iJumpFrame = Pos;
}

void callbackPause(int, void* p)
{
	((StartParams*)p)->bPause = !((StartParams*)p)->bPause;
}

void VideoMouseEvent(int event, int x, int y, int flags, void* userdata)
{
	cv::Rect button(620, (((StartParams*)userdata)->pp.noProcessing ? 480 : 540) + 10, 20, 20);
	if (event == cv::EVENT_LBUTTONDOWN) {
		if (!button.contains(cv::Point(x, y))) ((StartParams*)userdata)->breathPos.push_back(((StartParams*)userdata)->iCurPos);
	} else if (event == cv::EVENT_LBUTTONUP) {
		if (button.contains(cv::Point(x, y))) callbackPause(0, userdata);
		else ((StartParams*)userdata)->breathPos.push_back(((StartParams*)userdata)->iCurPos);
	}
}

#if defined(_WIN32) || defined(_WIN64) || defined(_WINDOWS)
#define FIRST_ARG 0
#else
#define FIRST_ARG 1
#endif

int main(int argc, char** argv)
{
	cvNamedWindow(WINDOWNAME, CV_WINDOW_AUTOSIZE);
	StartParams params = { };
	params.pvc = new cv::VideoCapture();
	//1440x1080 vidoes or 4:3 aspect ratio
	std::string metaFile = (argc == FIRST_ARG) ? "cap0.md" : (std::string(argv[FIRST_ARG]).substr(std::string(argv[FIRST_ARG]).find_last_of("/\\") + 1) + ".md");
	std::ifstream inpBreath(metaFile);
	if (inpBreath.is_open()) {
		std::string line;
		int size;
		inpBreath >> size;
		while (size-- != 0) {
			int b;
			inpBreath >> b;
			params.breathPos.push_back(b);
		}
		//while (std::getline(inpBreath, line)) {}
		inpBreath.close();
	}
	if (argc == FIRST_ARG ? params.pvc->open(0) : params.pvc->open(argv[FIRST_ARG])) {
		params.vi.dFPS = params.pvc->get(CV_CAP_PROP_FPS);
		params.vi.dFrameCount = params.pvc->get(CV_CAP_PROP_FRAME_COUNT);
		params.vi.dwWidthInPixels = params.pvc->get(CV_CAP_PROP_FRAME_WIDTH);
		params.vi.dwHeightInPixels = params.pvc->get(CV_CAP_PROP_FRAME_HEIGHT);
	}
	params.pp = ProcessParams{ true, 800, 600, -1, 10, true, 10, true, 3, 5, 0, 0, true, true, true, false };
	cv::setMouseCallback(WINDOWNAME, VideoMouseEvent, &params);
	cv::createTrackbar(TRACKBARNAME, WINDOWNAME, NULL, params.vi.dFrameCount, ChangeVideoPos, &params);
	//cv::createButton(PAUSEBTN, callbackPause, &params, CV_PUSH_BUTTON);
	std::thread vidProc(&VideoProcessor, std::ref(params));
	std::chrono::time_point<std::chrono::high_resolution_clock> start = std::chrono::high_resolution_clock::now();
	while (true) {
		//locking scope, not a unique_lock because only used once
		{
			std::lock_guard<std::mutex> lock(params.m);
			if (params.imgQueue.size() != 0) {
				cv::Mat mat;
				int idx = -1; 
				while (params.imgQueue.size() != 0) {
					int nextIdx = params.imgFrameNum.front();
					params.imgFrameNum.pop();
					if (nextIdx > idx) {
						idx = nextIdx;
						mat = params.imgQueue.front();
					}
					params.imgQueue.pop();
					if (params.pp.noProcessing) break;
				}
				if (params.breathPos.size() != 0) {
					double dScale = 1;// params.pp.dDesiredFPS / params.vi.dFPS;
					cv::Mat timeline = cv::Mat::zeros(1, 1 + idx * dScale, CV_8UC3);
					int i;
					for (i = 0; i < params.breathPos.size(); i++) {
						if (params.breathPos[i] <= idx)
							timeline.at<cv::Vec3b>(cv::Point(params.breathPos[i] * dScale, 0)) = i % 2 == 0 ? cv::Vec3b(0, 255, 0) : cv::Vec3b(0, 0, 255);
						else break;
					}
					cv::resize(timeline, timeline, cv::Size(std::min(640, (1 + (int)(idx * dScale))), 30), cv::INTER_MAX);

					timeline.copyTo(mat(cv::Rect(0, params.pp.noProcessing ? 480 : 540, std::min(640, (1 + (int)(idx * dScale))), 30)));

					char buf[256];
					sprintf(buf, "Breaths: %lu", i / 2);
					int baseLine = 0;
					cv::Size s = cv::getTextSize(cv::String(buf), cv::FONT_HERSHEY_PLAIN, 1, 1, &baseLine);
					cv::putText(mat, cv::String(buf), cv::Point((640 - s.width) / 2, (params.pp.noProcessing ? 480 : 540) + 10), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255, 255, 255));
					cv::Rect button(620, (params.pp.noProcessing ? 480 : 540) + 10, 20, 20);
					mat(button) = cv::Vec3b(200, 200, 200);
					cv::putText(mat(button), cv::String("\""), cv::Point(2, 27), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 0), 3);
				}
				cvSetTrackbarPos(TRACKBARNAME, WINDOWNAME, params.iCurPos = idx);
				cv::imshow(WINDOWNAME, mat);
			}
		}
		std::chrono::duration<double, std::milli> elapsed = std::chrono::high_resolution_clock::now() - start;
		//2000 would be half speed, etc should be a speed parameter...
		int c = cvWaitKey(std::max(1, (int)(1000 * params.iCurPos / params.vi.dFPS - elapsed.count()))); if (c == 27) break;
	}
	params.pc = 1;
	vidProc.join();
	if (params.breathPos.size() != 0) {
		std::ofstream breathfile;
		breathfile.open(metaFile);
		breathfile << params.breathPos.size() << std::endl;
		for (int i = 0; i < params.breathPos.size(); i++) {
			breathfile << params.breathPos[i] << std::endl;
		}
		breathfile.close();
	}
	cvDestroyWindow(WINDOWNAME);
	VideoCleanup((void*)params.pvc);
	return 0;
}