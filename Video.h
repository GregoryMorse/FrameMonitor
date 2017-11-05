//birth to 6 weeks: 30–40 breaths per minute - average per https://en.wikipedia.org/wiki/Respiratory_rate
//healthy resting adults is much slower, 12–18 breaths per minute
//premature birth would have an even high breath per minute

//FFT, Laplacian pyramids, multithreading technique:
//Detecting breathing rate of a person from video http://algomuse.com/c-c/detecting-breathing-rate-of-a-person-from-video

//Eulerian video magnification technique to find subtle motion

#pragma once
#include <thread>
#include <mutex>
#include "opencv2/opencv.hpp"
#include "opencv2/xfeatures2d.hpp"

#define TRACKBARNAME "Frame"
#define WINDOWNAME "VideoOscillation"

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

void VideoCleanup(LPVOID p)
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
	int frameNum, int framesPerSecond, time_t startTime, std::vector<double> & ft)
{
	cv::Mat canvas = cv::Mat3b(540, 640, cv::Vec3b(0, 0, 0));
	//480x420, 480x60, 160x210, 160x210
	cv::resize(frame, frame, cv::Size(480, 420), cv::INTER_CUBIC);
	frame.copyTo(canvas(cv::Rect(0, 0, 480, 420)));
	cv::resize(bkgnd, bkgnd, cv::Size(160, 210), cv::INTER_CUBIC);
	bkgnd.copyTo(canvas(cv::Rect(480, 0, 160, 210)));
	cv::resize(fgnd, fgnd, cv::Size(160, 210), cv::INTER_CUBIC);
	fgnd.copyTo(canvas(cv::Rect(480, 210, 160, 210)));

	if (motionDetected.size() != 0) {
		cv::Mat timeline = cv::Mat(1, motionDetected.size(), CV_8UC3);
		for (int i = 0; i < motionDetected.size(); i++) {
			double pct = motionDetected[i] / dMaxContourSize;
			timeline.at<cv::Vec3b>(cv::Point(i, 0)) = cv::Vec3b(pct > 0.5 ? 0 : 255 * (1 - pct * 2), 255 * (pct > 0.5 ? (pct * 2 - 1) : 1), 255);
		}
		cv::resize(timeline, timeline, cv::Size(640, 30), cv::INTER_MAX);
		timeline.copyTo(canvas(cv::Rect(0, 420, 640, 30)));
	}

	if (oscillations.size() != 0) {
		double dMaxOsc = 0;
		cv::Mat timeline = cv::Mat::zeros(30, oscillations.size(), CV_8UC3);
		for (int i = 0; i < oscCount.size(); i++) {
			if (oscCount[i] != 0) {
				dMaxOsc = std::max(oscillations[i] / oscCount[i], dMaxOsc);
			}
		}
		if (dMaxOsc != 0) {
			for (int i = 0; i < oscillations.size(); i++) {
				if (oscCount[i] != 0) {
					double pct = (oscillations[i] / oscCount[i]) / dMaxOsc;
					if (i != 0 && i != oscillations.size() - 1 && (oscillations[i] / oscCount[i]) > (oscillations[i - 1] / oscCount[i - 1]) && (oscillations[i] / oscCount[i]) > (oscillations[i + 1] / oscCount[i + 1])) {
						timeline.at<cv::Vec3b>(cv::Point(i, 29 * pct)) = cv::Vec3b(0, 0, 255);
					} else if (i != 0 && i != oscillations.size() - 1 && (oscillations[i] / oscCount[i]) < (oscillations[i - 1] / oscCount[i - 1]) && (oscillations[i] / oscCount[i]) < (oscillations[i + 1] / oscCount[i + 1])) {
						timeline.at<cv::Vec3b>(cv::Point(i, 29 * pct)) = cv::Vec3b(0, 255, 0);
					} else {
						timeline.at<cv::Vec3b>(cv::Point(i, 29 * pct)) = cv::Vec3b(255, 255, 255);
					}
				}
			}
		}
		cv::resize(timeline, timeline, cv::Size(640, 30), cv::INTER_MAX);
		//FPS
		char buf[256];
		time_t now = time(NULL);
		sprintf(buf, "Frame: %lu Time: %lu Processing FPS: %lu", frameNum, frameNum / framesPerSecond, now == startTime ? 0 : frameNum / (int)(now - startTime));
		int baseLine = 0;
		cv::Size s = cv::getTextSize(cv::String(buf), cv::FONT_HERSHEY_PLAIN, 1, 1, &baseLine);
		cv::putText(timeline, cv::String(buf), cv::Point((640 - s.width) / 2, 25), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255, 255, 255));
		timeline.copyTo(canvas(cv::Rect(0, 450, 640, 30)));
	}


	return canvas;
}

void ProcessVideo(VidInfo vi, cv::VideoCapture & pvc, std::vector<double> & motionDetected, std::vector<double> & oscillations, double & dMaxContourSize, double & dMaxOscillation, std::function<ProgressFunc> pf, int* pCancel, int* iJumpFrame)
{
	ProcessParams pp = ProcessParams{ true, 800, 600, -1, 10, true, 500, 10, true, 3, 5, 0, 0, true, true, true };
	cv::Mat frame, fgimg;
	double dSumContourSize;
	cv::Ptr<cv::BackgroundSubtractor> bg;
	std::vector<std::vector<cv::Point>> contours;
	std::vector<double> areas;
	std::vector<double> oscCount;
	int UseEvery, FrameCount = 0;
	//1440x1080 vidoes or 4:3 aspect ratio
	if (pvc.isOpened()) {
		bg = pp.bMOG ? (cv::Ptr<cv::BackgroundSubtractor>)cv::createBackgroundSubtractorMOG2(pp.history, pp.threshold, pp.bShadowDetection) : (cv::Ptr<cv::BackgroundSubtractor>)cv::createBackgroundSubtractorKNN(pp.history, pp.threshold, pp.bShadowDetection);
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
	
	time_t start = time(NULL);
	//cv::BFMatcher::create();
	matcher = cv::FlannBasedMatcher::create();
	//matcher = cv::FlannBasedMatcher::FlannBasedMatcher();
	while (pvc.isOpened()) {
		int iJump;
		if ((iJump = *iJumpFrame) != 0) {
			*iJumpFrame = 0;
			pvc.set(CV_CAP_PROP_POS_FRAMES, (iJump > pp.history) ? iJump - pp.history : 0); //CV_CAP_PROP_POS_MSEC
			bg = pp.bMOG ? (cv::Ptr<cv::BackgroundSubtractor>)cv::createBackgroundSubtractorMOG2(pp.history, pp.threshold, pp.bShadowDetection) : (cv::Ptr<cv::BackgroundSubtractor>)cv::createBackgroundSubtractorKNN(pp.history, pp.threshold, pp.bShadowDetection);
			if (pp.bMOG) {
				((cv::BackgroundSubtractorMOG2*)bg.get())->setNMixtures(pp.nMixtures);
			}
			contourWindow.clear();
			dMaxContourSize = 0;
			motionDetected.clear();
			oscillations.clear();
			oscCount.clear();
			lastkps.clear();
			lastdescs = cv::Mat();
			fdps.clear();
			cumulativeMask = cv::Mat::zeros(cv::Size(pp.iMinWidth, pp.iMinHeight), CV_8UC1);
			FrameCount = iJump;
			start = time(NULL);
		}

		/*while (vc.grab()) {
			cv::OutputArray img();
			vc.retrieve(img);
		}*/
		pvc >> frame; //calls read which is the same as grab/retrieve combined
		cv::Mat detFrame = frame.clone();
		//CV_CAP_PROP_FRAME_COUNT, CV_CAP_PROP_FORMAT, CV_CAP_PROP_POS_MSEC, CV_CAP_PROP_POS_FRAMES
		//if (!vc.isOpened()) break;
		if (!*pCancel && frame.dims != 0 && FrameCount != pp.iMaxFrames) {
			if (FrameCount % UseEvery == 0) {
				//conversion for processing
				if (pp.iMinHeight != 0 || pp.iMinWidth != 0) cv::resize(frame, frame, cv::Size(pp.iMinWidth, pp.iMinHeight), 0, 0, cv::INTER_AREA); //cv::INTER_LANCZOS4, cv:INTER_LINEAR for zooming, cv::INTER_CUBIC slow for zooming, cv::INTER_AREA for shrinking
				if (pp.bGrayScale) cvtColor(frame, frame, CV_BGRA2GRAY);

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
				} else {
					dSumContourSize = 0;
				}
				motionDetected.push_back(dSumContourSize);

				oscillations.push_back(0);
				oscCount.push_back(0);

				contourWindow.push_back(contours);
				mask = cv::Mat::zeros(cv::Size(pp.iMinWidth, pp.iMinHeight), CV_8UC1);
				cv::drawContours(mask, contourWindow.back(), -1, cv::Scalar(1), CV_FILLED);
				cumulativeMask += mask;
				if (contourWindow.size() > pp.iCalibrationTimeWindow * pp.dDesiredFPS) {
					mask = cv::Mat::zeros(cv::Size(pp.iMinWidth, pp.iMinHeight), CV_8UC1);
					cv::drawContours(mask, contourWindow.front(), -1, cv::Scalar(1), CV_FILLED);
					cumulativeMask -= mask;
					contourWindow.erase(contourWindow.begin(), contourWindow.begin() + 1);
				}				
				//could make a convex hull to get a more inclusive shape around the contours...
				//cv::convexHull()
				cv::threshold(cumulativeMask, mask, 0, 255, cv::THRESH_BINARY);
				kps.clear();

				//feature point tracking
				mask = cv::Mat::zeros(cv::Size(pp.iMinWidth, pp.iMinHeight), CV_8UC1);
				cv::ellipse(mask, cv::Point(pp.iMinWidth / 2, pp.iMinHeight / 2), cv::Size(pp.iMinWidth / 4, pp.iMinHeight / 4), 0, 0, 360, cv::Scalar(255), CV_FILLED);

				featDetector->detectAndCompute(frame, mask, kps, descs);
				if (lastkps.size() != 0 && kps.size() != 0) {
					matches.clear();
					newfdps.clear();
					matcher->match(descs, lastdescs, matches);
					double minDist = std::min_element(matches.begin(), matches.end(), [](cv::DMatch& d1, cv::DMatch& d2) { return d1.distance < d2.distance; })->distance;
					for (std::vector<cv::DMatch>::iterator curMatch = matches.begin(); curMatch != matches.end(); curMatch++) {
						if (curMatch->distance > std::max(2 * minDist, 0.02)) continue; //filter noise matches
						//filter for periodic moving feature points -> direction, opposite direction, direction, opposite direction
						//based on reference point - 2 breaths/oscillations - include only if meets criteria
						if ((it = fdps.find(lastkps[curMatch->trainIdx].pt)) != fdps.end()) {
							it->second.kps.push_back(kps[curMatch->queryIdx]);
							//could take centroid of all points or to start use center of image
							//reference point is the further place of gravity, so the bottom center makes perfect sense
							cv::Point2f refpt(pp.iMinWidth / 2, pp.iMinHeight);
							std::vector<double> netVals;
							if (!it->second.isOscilPoint && check_oscillation(it->second.kps, refpt)) {
								get_oscillation(it->second.kps, refpt, false, netVals);
								it->second.isOscilPoint = true;
								oscillations.back() += netVals.back();
								oscCount.back()++;
							} else if (it->second.isOscilPoint) {
								double lastval = get_oscillation(it->second.kps, refpt, true, netVals);
								oscillations.back() += lastval;
								oscCount.back()++;
							}
							if (it->second.kps.size() > pp.iCalibrationTimeWindow * pp.dDesiredFPS) {
								it->second.kps.erase(it->second.kps.begin(), it->second.kps.begin() + 1);
							}
							newfdps[kps[curMatch->queryIdx].pt] = FeatureDataPoint{ FrameCount, it->second.kps, std::vector<int>(), it->second.isOscilPoint };
						} else {
							ks.clear();
							ks.push_back(kps[curMatch->queryIdx]);
							newfdps[kps[curMatch->queryIdx].pt] = FeatureDataPoint{ FrameCount, std::vector<cv::KeyPoint>(), std::vector<int>(), false };
						}
					}
					//cleans up all stale
					fdps = newfdps;
				}
				lastkps = kps;
				lastdescs = descs;

				//color intensity monitoring
				//build laplacian pyramid and select best image
				cv::Mat ellipse;
				cv::bitwise_and(frame, mask, ellipse);
				FFTWindow.push_back(ellipse.isContinuous() ? ellipse.reshape(0, 1) : ellipse.clone().reshape(0, 1));
				if (FFTWindow.rows > pp.iCalibrationTimeWindow * pp.dDesiredFPS) {
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
					MultiMat = planes[0].colRange(FFTWindow.rows * .1 / pp.dDesiredFPS - 1, FFTWindow.rows / pp.dDesiredFPS + 1);
					cv::Mat MaxIndexes;
					ellipse = MultiMat.isContinuous() ? MultiMat.reshape(0, 1) : MultiMat.clone().reshape(0, 1);
					cv::sortIdx(ellipse, MaxIndexes, CV_SORT_DESCENDING | CV_SORT_EVERY_ROW);
					int idx = MaxIndexes.at<int>(cv::Point(0, 0));
					double d = MultiMat.at<double>(cv::Point(idx % MultiMat.cols, idx / MultiMat.cols));
					FFTWindow = FFTWindow.rowRange(1, FFTWindow.rows);
				}

				if (pf != NULL)
				{
					bg.get()->getBackgroundImage(bkgnd);
					cv::cvtColor(bkgnd, bkgnd, pp.bGrayScale ? CV_GRAY2RGB : CV_BGRA2RGB);

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
						} else cv::drawContours(detFrame, contours, -1, cv::Scalar(0, 0, 255), 2);
					}

					if (pp.iMinHeight != 0 || pp.iMinWidth != 0) {
						std::vector<cv::KeyPoint> kpoints;
						std::transform(lastkps.begin(), lastkps.end(), std::back_inserter(kpoints), [pp, vi](cv::KeyPoint& v) { return cv::KeyPoint(cv::Point2f(v.pt.x * vi.dwWidthInPixels / pp.iMinWidth, v.pt.y * vi.dwHeightInPixels / pp.iMinHeight), v.size * vi.dwWidthInPixels / pp.iMinWidth, v.angle, v.response, v.octave, v.class_id); });
						cv::drawKeypoints(detFrame, kpoints, detFrame);
					} else cv::drawKeypoints(detFrame, lastkps, detFrame);
					//cv::drawMatches(frame, kps, frame, kps, matches, frame);

					//cvtColor(detFrame, detFrame, CV_BGRA2RGB);

					pf(DrawUI(detFrame, bkgnd, fgnd, motionDetected, oscillations, oscCount, dMaxContourSize, FrameCount, vi.dFPS, start), FrameCount);
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
};

void VideoProcessor(StartParams& p)
{
	ProcessVideo(p.vi, *p.pvc, p.motionDetect, p.osc, p.dMaxContour, p.dMaxOsc,
		std::move(std::function<ProgressFunc>([&p](cv::Mat mat, int Frame) -> void {
		std::lock_guard<std::mutex> lock(p.m);
		p.imgFrameNum.push(Frame);
		p.imgQueue.push(mat);
	})), &p.pc, &p.iJumpFrame);
}

void ChangeVideoPos(int Pos, void* p)
{
	if (((StartParams*)p)->iCurPos != Pos)
		((StartParams*)p)->iJumpFrame = Pos;
}

int main(int argc, char** argv)
{
	cvNamedWindow(WINDOWNAME, CV_WINDOW_AUTOSIZE);
	StartParams params = { new cv::VideoCapture() };
	//1440x1080 vidoes or 4:3 aspect ratio
	if (argc == 0 ? params.pvc->open(0) : params.pvc->open(argv[0])) {
		params.vi.dFPS = params.pvc->get(CV_CAP_PROP_FPS);
		params.vi.dFrameCount = params.pvc->get(CV_CAP_PROP_FRAME_COUNT);
		params.vi.dwWidthInPixels = params.pvc->get(CV_CAP_PROP_FRAME_WIDTH);
		params.vi.dwHeightInPixels = params.pvc->get(CV_CAP_PROP_FRAME_HEIGHT);
	}
	cvCreateTrackbar2(TRACKBARNAME, WINDOWNAME, NULL, params.vi.dFrameCount, ChangeVideoPos, &params);
	std::thread vidProc(&VideoProcessor, std::ref(params));
	while (true) {
		{
			std::lock_guard<std::mutex> lock(params.m);
			if (params.imgQueue.size() != 0) {
				cv::Mat mat;
				int idx = 0;
				while (params.imgQueue.size() != 0) {
					int nextIdx = params.imgFrameNum.front();
					params.imgFrameNum.pop();
					if (nextIdx > idx) {
						idx = nextIdx;
						mat = params.imgQueue.front();
					}
					params.imgQueue.pop();
				}
				cvSetTrackbarPos(TRACKBARNAME, WINDOWNAME, params.iCurPos = idx);
				cv::imshow(WINDOWNAME, mat);
			}
		}
		int c = cvWaitKey(1); if (c == 27) break;
	}
	params.pc = 1;
	vidProc.join();
	cvDestroyWindow(WINDOWNAME);
	VideoCleanup((LPVOID)params.pvc);
	return 0;
}