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
#if defined(_WIN32) || defined(_WIN64) || defined(_WINDOWS)
#include <numeric>
#else
#include <algorithm>
#endif
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

void SmoothData(std::vector<int> & v, std::vector<double> & vals,
	std::vector<std::pair<double, double>> & minmaxes,
	std::vector<double> & pcts)
{
	/*cv::KalmanFilter kf(2, 1, 0);
	cv::Mat state(2, 1, CV_32F);
	cv::Mat processNoise(2, 1, CV_32F);
	cv::Mat measurement = cv::Mat::zeros(1, 1, CV_32F);
	kf.transitionMatrix = (cv::Mat_<float>(2, 2) << 1, 1, 0, 1);
	cv::setIdentity(kf.measurementMatrix);
	cv::setIdentity(kf.processNoiseCov, cv::Scalar::all(1e-5));
	cv::setIdentity(kf.measurementNoiseCov, cv::Scalar::all(1e-1));
	cv::setIdentity(kf.errorCovPost, cv::Scalar::all(1));
	std::transform(v.begin(), v.end(), std::back_inserter(pcts), [vals, minmaxes, &measurement, &kf](int idx) ->
		double { measurement.at<float>(0) = *minmaxes[idx].second - *minmaxes[idx].first == 0 ? 0 : (vals[idx] - *minmaxes[idx].first) / (*minmaxes[idx].second - *minmaxes[idx].first); kf.correct(measurement); return kf.predict().at<float>(0); });*/

	std::vector<double> normpcts;
	int base = *v.begin();
	std::transform(v.begin(), v.end(), std::back_inserter(normpcts), [&vals, &minmaxes, base](int idx) ->
		double { return minmaxes[idx - base].second - minmaxes[idx - base].first == 0 ? 0 : (vals[idx - base] - minmaxes[idx - base].first) / (minmaxes[idx - base].second - minmaxes[idx - base].first); });
	//window size should be configurable...
	int sz = normpcts.size();
	std::transform(v.begin(), v.end(), std::back_inserter(pcts), [&normpcts, sz, base](int idx) ->
		double { return ((idx - base >= 3 ? normpcts[idx - base - 3] : 0) + (idx - base >= 2 ? normpcts[idx - base - 2] : 0) + (idx - base >= 1 ? normpcts[idx - base - 1] : 0) + normpcts[idx - base] +
				(idx - base + 2 <= sz ? normpcts[idx - base + 1] : 0) + (idx - base + 3 <= sz ? normpcts[idx - base + 2] : 0) + (idx - base + 4 <= sz ? normpcts[idx - base + 3] : 0)) /
			((idx - base >= 3 ? 1 : 0) + (idx - base >= 2 ? 1 : 0) + (idx - base >= 1 ? 1 : 0) + 1 + (idx - base + 2 <= sz ? 1 : 0) + (idx - base + 3 <= sz ? 1 : 0) + (idx - base + 4 <= sz ? 1 : 0)); });
}

void findFrequencyMinMax(int freq, std::vector<double> & pcts, std::set<int> & maxindexes, std::set<int> & minindexes)
{
	std::vector<int> peaks, troughs;
	int sz = pcts.size();
	for (int i = 0; i < sz; i++) {
		//if (i > 1 && i < sz - 2 && pcts[i] > pcts[i - 2] && pcts[i] > pcts[i - 1] && pcts[i] > pcts[i + 1] && pcts[i] > pcts[i + 2]) {
		if (i > 0 && i < sz - 1 && pcts[i] > pcts[i - 1] && pcts[i] == pcts[i + 1]) {
			int n = 2;
			while (i < sz - n && pcts[i] == pcts[i + n]) n++;
			if (i < sz - n && pcts[i] > pcts[i + n]) peaks.push_back(i);
		} else if (i > 0 && i < sz - 1 && pcts[i] > pcts[i - 1] && pcts[i] > pcts[i + 1]) {
			peaks.push_back(i);
		//} else if (i > 1 && i < sz - 2 && pcts[i] < pcts[i - 2] && pcts[i] < pcts[i - 1] && pcts[i] < pcts[i + 1] && pcts[i] < pcts[i + 2]) {
		} else if (i > 0 && i < sz - 1 && pcts[i] < pcts[i - 1] && pcts[i] == pcts[i + 1]) {
			int n = 2;
			while (i < sz - n && pcts[i] == pcts[i + n]) n++;
			if (i < sz - n && pcts[i] < pcts[i + n]) {
				troughs.push_back(i);
				//if (peaks.size() != 0 && troughs.size() != 0 && peaks.back() == i - 1 && troughs.back() == i - 2) peaks.pop_back();
			}
		} else if (i > 0 && i < sz - 1 && pcts[i] < pcts[i - 1] && pcts[i] < pcts[i + 1]) {
			//if (peaks.size() != 0 && troughs.size() != 0 && peaks.back() == i - 1 && troughs.back() == i - 2) peaks.pop_back();
			troughs.push_back(i);
		}
	}
	int lasti = 0, maxlasti = 0;
	int lastmin = 0;
	int ps = peaks.size(); int ts = troughs.size();
	if (ps == 0) return;
	for (int i = 0; i <= ps; i++) {
		if (peaks[maxlasti] > peaks[lasti] + freq) {
			while (maxlasti + 1 < ps && peaks[maxlasti] <= peaks[lasti] + freq * 3 / 2 && pcts[peaks[maxlasti + 1]] > pcts[peaks[maxlasti]]) maxlasti++;
			maxindexes.insert(peaks[maxlasti]);
			if (lastmin < ts && troughs[lastmin] < peaks[maxlasti]) {
				int min;
				for (min = lastmin; min < ts && peaks[maxlasti] > troughs[min]; min++) {}
				minindexes.insert(*std::min_element(troughs.begin() + lastmin, troughs.begin() + min, [pcts](int& i1, int& i2) { return pcts[i1] < pcts[i2]; }));
				lastmin = min;
			}
			lasti = maxlasti; maxlasti = i;
		}
		if (i == ps) break;
		if (peaks[maxlasti] <= peaks[lasti] + freq || pcts[peaks[i]] > pcts[peaks[maxlasti]]) {
			maxlasti = i;
		}
	}
}

void calcResults(std::vector<double> & motionDetected, std::vector<double> & oscillations, std::vector<double> & oscCount, std::vector<double> & ft,
	std::vector<double> & finalpcts, std::set<int> & maxindexes, std::set<int> & minindexes, int windowSize)
{
	int ms = motionDetected.size();

	std::vector<double> vals;
	std::vector<int> v(windowSize == -1 ? ms : std::min(windowSize, ms));
	std::iota(v.begin(), v.end(), windowSize == -1 ? 0 : std::max(0, ms - windowSize));
	std::transform(v.begin(), v.end(), std::back_inserter(vals),
		[oscillations, oscCount](int idx) ->
		double { return oscCount[idx] == 0 ? 0 : oscillations[idx] / oscCount[idx]; });
	std::vector<std::pair<double, double>> minmaxes;
	int vs = oscillations.size();
	int base = *v.begin();
	std::transform(v.begin(), v.end(), std::back_inserter(minmaxes), [&vals, vs, base](int idx) ->
		std::pair<double, double>
	{ std::pair<std::vector<double>::iterator, std::vector<double>::iterator> minmax =
		std::minmax_element(vals.begin() + std::max(0, idx - base - 25), vals.begin() + std::min(idx - base + 25, (int)vs - base));
	return std::pair<double, double>(*minmax.first, *minmax.second); });
	std::vector<double> pcts1;
	SmoothData(v, vals, minmaxes, pcts1);

	//std::vector<int> v(ft.size());
	//std::iota(v.begin(), v.end(), 0);
	//std::vector<std::pair<std::vector<double>::iterator, std::vector<double>::iterator>> minmaxes;
	minmaxes.clear();
	vs = ft.size();
	std::transform(v.begin(), v.end(), std::back_inserter(minmaxes), [&ft, vs, base](int idx) ->
		std::pair<double, double>
	{ std::pair<std::vector<double>::iterator, std::vector<double>::iterator> minmax =
		std::minmax_element(ft.begin() + std::max(0, idx - 25), ft.begin() + std::min(idx + 25, (int)vs));
	return std::pair<double, double>(*minmax.first, *minmax.second); });
	std::vector<double> pcts2;
	SmoothData(v, ft, minmaxes, pcts2);

	//std::vector<int> v(ms);
	//std::iota(v.begin(), v.end(), 0);
	//std::vector<std::pair<std::vector<double>::iterator, std::vector<double>::iterator>> minmaxes;
	minmaxes.clear();
	std::transform(v.begin(), v.end(), std::back_inserter(minmaxes), [&motionDetected, ms, base](int idx) ->
		std::pair<double, double>
	{ std::pair<std::vector<double>::iterator, std::vector<double>::iterator> minmax =
		std::minmax_element(motionDetected.begin() + std::max(0, idx - 25), motionDetected.begin() + std::min(idx + 25, (int)ms));
	return std::pair<double, double>(*minmax.first, *minmax.second); });
	std::vector<double> pcts3;
	SmoothData(v, motionDetected, minmaxes, pcts3);

	std::vector<double> pcts;
	std::transform(v.begin(), v.end(), std::back_inserter(pcts), [&pcts1, &pcts2, &pcts3, base](int idx) -> double { return pcts1[idx - base] + pcts2[idx - base] + pcts3[idx - base]; });

	std::pair<std::vector<double>::iterator, std::vector<double>::iterator> dMinMax = std::minmax_element(pcts.begin(), pcts.end());
	findFrequencyMinMax(25, pcts, maxindexes, minindexes);
	std::transform(v.begin(), v.end(), std::back_inserter(finalpcts), [&pcts, &dMinMax, base](int idx) -> double { return *dMinMax.second - *dMinMax.first == 0 ? 0 : (pcts[idx - base] - *dMinMax.first) / (*dMinMax.second - *dMinMax.first); });
}

cv::Mat DrawUI(cv::Mat frame, cv::Mat bkgnd, cv::Mat fgnd, std::vector<double> & motionDetected, std::vector<double> & oscillations, std::vector<double> & oscCount, double dMaxContourSize,
	int frameNum, int iBaseFrame, int framesPerSecond, time_t startTime, std::vector<double> & ft, double dScale)
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
	sprintf(buf, "Processing FPS: %lu", now == startTime ? 0 : (frameNum - iBaseFrame) / (int)(now - startTime));
	s = cv::getTextSize(cv::String(buf), cv::FONT_HERSHEY_PLAIN, 1, 1, &baseLine);
	cv::putText(canvas, cv::String(buf), cv::Point(480 + (160 - s.width) / 2, 210 + 10 + 30), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255, 255, 255));

	int ms = motionDetected.size();
	if (ms != 0) {
		std::pair<std::vector<double>::iterator, std::vector<double>::iterator> dMinMax = std::minmax_element(motionDetected.begin(), motionDetected.end());
		double d = *dMinMax.second;
		cv::Mat timeline = cv::Mat(30, std::min(640, (int)ms), CV_8UC3);
		int base = std::max(0, ms - 640);
		for (int i = base; i < ms; i++) {
			double pct = motionDetected[i] / dMaxContourSize;
			//timeline.at<cv::Vec3b>(cv::Point(i - base, 0)) = cv::Vec3b(pct > 0.5 ? 0 : 255 * (1 - pct * 2), 255 * (pct > 0.5 ? (pct * 2 - 1) : 1), 255);
			cv::line(timeline, cv::Point(i - base, 0), cv::Point(i - base, 29), cv::Vec3b(pct > 0.5 ? 0 : 255 * (1 - pct * 2), 255 * (pct > 0.5 ? (pct * 2 - 1) : 1), 255));
		}
		//cv::resize(timeline, timeline, cv::Size(std::min(640, (int)ms), 30), cv::INTER_MAX);
		timeline.copyTo(canvas(cv::Rect(0, 420, std::min(640, (int)ms), 30)));
	}

	/*if (oscillations.size() != 0) {
		//how to smooth the jagged data - sliding window average, Kalman filter
		cv::Mat timeline = cv::Mat::zeros(30, std::max(640, (int)(oscillations.size() * 1)), CV_8UC3);
		int breaths = 0;
		//normalizing and smoothing could be generalized to a transform lambda with mutable capture accumulating?
		//if (dMaxOsc != 0) {
			double lastpct = 0;
			std::vector<double> vals;
			std::vector<int> v(oscillations.size());
			std::iota(v.begin(), v.end(), 0);
			std::transform(v.begin(), v.end(), std::back_inserter(vals),
				[oscillations, oscCount](int idx) ->
				double { return oscCount[idx] == 0 ? 0: oscillations[idx] / oscCount[idx]; });
			std::vector<std::pair<std::vector<double>::iterator, std::vector<double>::iterator>> minmaxes;
			std::transform(v.begin(), v.end(), std::back_inserter(minmaxes), [&vals](int idx) ->
				std::pair<std::vector<double>::iterator, std::vector<double>::iterator>
			{ return std::minmax_element(vals.begin() + std::max(0, idx - 25), vals.begin() + std::min(idx + 25, (int)vals.size())); });
			std::vector<double> pcts;
			SmoothData(v, vals, minmaxes, pcts);
			std::pair<std::vector<double>::iterator, std::vector<double>::iterator> dMinMax = std::minmax_element(pcts.begin(), pcts.end());
			std::set<int> maxindexes, minindexes;
			findFrequencyMinMax(25, pcts, maxindexes, minindexes);
			for (int i = 0; i < vals.size(); i++) {
				double pct = *dMinMax.second - *dMinMax.first == 0 ? 0 : (pcts[i] - *dMinMax.first) / (*dMinMax.second - *dMinMax.first);
				if (maxindexes.find(i) != maxindexes.end()) {
					cv::line(timeline, cv::Point(i * 1, 0), cv::Point(i * 1, 30), cv::Vec3b(0, 0, 255));
					breaths++;
				} else if (minindexes.find(i) != minindexes.end()) {
					cv::line(timeline, cv::Point(i * 1, 0), cv::Point(i * 1, 30), cv::Vec3b(0, 255, 0));
					breaths++;
				}
				timeline.at<cv::Vec3b>(cv::Point(i * 1, 29 * pct)) = cv::Vec3b(255, 255, 255);
				lastpct = pct;
			}
		//}
		if (oscillations.size() * 1 > 640) cv::resize(timeline, timeline, cv::Size(640, 30), cv::INTER_MAX);
		timeline.copyTo(canvas(cv::Rect(0, 450, 640, 30)));

		char buf[256];
		sprintf(buf, "Breaths: %lu (feature points)", breaths / 2);
		int baseLine = 0;
		cv::Size s = cv::getTextSize(cv::String(buf), cv::FONT_HERSHEY_PLAIN, 1, 1, &baseLine);
		cv::putText(canvas, cv::String(buf), cv::Point((640 - s.width) / 2, 450 + 10), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255, 255, 255));
	}

	if (ft.size() != 0) {
		std::vector<int> v(ft.size());
		std::iota(v.begin(), v.end(), 0);
		std::vector<std::pair<std::vector<double>::iterator, std::vector<double>::iterator>> minmaxes;
		std::transform(v.begin(), v.end(), std::back_inserter(minmaxes), [&ft](int idx) ->
			std::pair<std::vector<double>::iterator, std::vector<double>::iterator>
		{ return std::minmax_element(ft.begin() + std::max(0, idx - 25), ft.begin() + std::min(idx + 25, (int)ft.size())); });
		std::vector<double> pcts;
		SmoothData(v, ft, minmaxes, pcts);
		std::pair<std::vector<double>::iterator, std::vector<double>::iterator> dMinMax = std::minmax_element(pcts.begin(), pcts.end());
		std::set<int> maxindexes, minindexes;
		findFrequencyMinMax(25, pcts, maxindexes, minindexes);
		cv::Mat timeline = cv::Mat::zeros(30, ft.size() * dScale, CV_8UC3);
		int breaths = 0;
		for (int i = 0; i < ft.size(); i++) {
			double pct = *dMinMax.second - *dMinMax.first == 0 ? 0 : (pcts[i] - *dMinMax.first) / (*dMinMax.second - *dMinMax.first);
			if (maxindexes.find(i) != maxindexes.end()) {
				cv::line(timeline, cv::Point(i * dScale, 0), cv::Point(i * dScale, 30), cv::Vec3b(0, 0, 255));
				breaths++;
			} else if (minindexes.find(i) != minindexes.end()) {
				cv::line(timeline, cv::Point(i * dScale, 0), cv::Point(i * dScale, 30), cv::Vec3b(0, 255, 0));
				breaths++;
			}
			timeline.at<cv::Vec3b>(cv::Point(i * dScale, 29 * pct)) = cv::Vec3b(255, 255, 255);
		}
		cv::resize(timeline, timeline, cv::Size(std::min(640, (int)(ft.size() * dScale)), 30), cv::INTER_MAX);
		timeline.copyTo(canvas(cv::Rect(0, 480, std::min(640, (int)(ft.size() * dScale)), 30)));

		char buf[256];
		sprintf(buf, "Breaths: %lu (light intensity)", breaths / 2);
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
		std::vector<int> v(motionDetected.size());
		std::iota(v.begin(), v.end(), 0);
		std::vector<std::pair<std::vector<double>::iterator, std::vector<double>::iterator>> minmaxes;
		std::transform(v.begin(), v.end(), std::back_inserter(minmaxes), [&motionDetected](int idx) ->
			std::pair<std::vector<double>::iterator, std::vector<double>::iterator>
		{ return std::minmax_element(motionDetected.begin() + std::max(0, idx - 25), motionDetected.begin() + std::min(idx + 25, (int)motionDetected.size())); });
		std::vector<double> pcts;
		SmoothData(v, motionDetected, minmaxes, pcts);
		std::pair<std::vector<double>::iterator, std::vector<double>::iterator> dMinMax = std::minmax_element(pcts.begin(), pcts.end());
		std::set<int> maxindexes, minindexes;
		findFrequencyMinMax(25, pcts, maxindexes, minindexes);
		int breaths = 0;
		double lastpct = 0;
		for (int i = 0; i < motionDetected.size(); i++) {
			double pct = *dMinMax.second - *dMinMax.first == 0 ? 0 : (pcts[i] - *dMinMax.first) / (*dMinMax.second - *dMinMax.first);
			//log(exp(N) * Value) / N brings normalized value to logarithmic scale with correctly chosen N factor which is big enough to bring all values above 1
			//log(1+Value) also works
			//double pct = d == 0 ? 0 : motionDetected[i] / d; //if (pct > 1.0) pct = 1.0; if (pct > .001) pct = .001; pct *= 1000;
			//pct = pct == 0 ? 0 : log(exp(10) * pct) / 10;
			//pct = log(1 + pct * (exp(1) - 1));
			if (maxindexes.find(i) != maxindexes.end()) {
				cv::line(timeline, cv::Point(i * dScale, 0), cv::Point(i * dScale, 30), cv::Vec3b(0, 0, 255));
				breaths++;
			} else if (minindexes.find(i) != minindexes.end()) {
				cv::line(timeline, cv::Point(i * dScale, 0), cv::Point(i * dScale, 30), cv::Vec3b(0, 255, 0));
				breaths++;
			}
			timeline.at<cv::Vec3b>(cv::Point(i * dScale, 29 * pct)) = cv::Vec3b(255, 255, 255);
			lastpct = pct;
		}
		cv::resize(timeline, timeline, cv::Size(std::min(640, (int)(motionDetected.size() * dScale)), 30), cv::INTER_MAX);
		timeline.copyTo(canvas(cv::Rect(0, 510, std::min(640, (int)(motionDetected.size() * dScale)), 30)));

		char buf[256];
		sprintf(buf, "Breaths: %lu (motion area)", breaths / 2);
		int baseLine = 0;
		cv::Size s = cv::getTextSize(cv::String(buf), cv::FONT_HERSHEY_PLAIN, 1, 1, &baseLine);
		cv::putText(canvas, cv::String(buf), cv::Point((640 - s.width) / 2, 510 + 10), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255, 255, 255));
	}*/

	if (ms != 0) {
		cv::Mat timeline = cv::Mat::zeros(90, std::min(640, (int)(ms * dScale)), CV_8UC3);

		std::vector<double> pcts;
		std::set<int> maxindexes, minindexes;
		calcResults(motionDetected, oscillations, oscCount, ft, pcts, maxindexes, minindexes, std::min(640, (int)(ms * dScale)));
		int breaths = 0;
		for (int i = 0; i < std::min(640, (int)(ms * dScale)); i++) {
			if (maxindexes.find(i) != maxindexes.end()) {
				cv::line(timeline, cv::Point(i * dScale, 0), cv::Point(i * dScale, 90), cv::Vec3b(0, 0, 255));
				breaths++;
			}
			else if (minindexes.find(i) != minindexes.end()) {
				cv::line(timeline, cv::Point(i * dScale, 0), cv::Point(i * dScale, 90), cv::Vec3b(0, 255, 0));
				breaths++;
			}
			if (i != 0) cv::line(timeline, cv::Point((i - 1) * dScale, 89 * pcts[i - 1]), cv::Point(i * dScale, 89 * pcts[i]), cv::Vec3b(255, 255, 255));
			//timeline.at<cv::Vec3b>(cv::Point(i * dScale, 89 * pct)) = cv::Vec3b(255, 255, 255);
		}
		//cv::resize(timeline, timeline, cv::Size(std::min(640, (int)(ms * dScale)), 90), cv::INTER_MAX);
		timeline.copyTo(canvas(cv::Rect(0, 450, std::min(640, (int)(ms * dScale)), 90)));

		char buf[256];
		sprintf(buf, "Breaths: %lu", breaths / 2);
		int baseLine = 0;
		cv::Size s = cv::getTextSize(cv::String(buf), cv::FONT_HERSHEY_PLAIN, 1, 1, &baseLine);
		cv::putText(canvas, cv::String(buf), cv::Point((640 - s.width) / 2, 510 + 10), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255, 255, 255));
	}

	return canvas;
}

struct DrawItem
{
	cv::Mat detFrame;
	cv::Mat bkg;
	cv::Mat fgnd;
	std::vector<double> motionDetected;
	std::vector<double> oscillations;
	std::vector<double> oscCount;
	double dMaxContourSize;
	int FrameCount;
	int iBaseFrame;
	time_t start;
	std::vector<double> ft;
};

struct DrawStartParams
{
	std::function<ProgressFunc> pf;
	int* pCancel;
	std::queue<DrawItem> drawQueue;
	mutable std::mutex m;
	ProcessParams pp;
	VidInfo vi;
};

void DrawProcessor(DrawStartParams& p)
{
	DrawItem di;
	while (!*p.pCancel) {
		while (p.drawQueue.size() == 0 && !*p.pCancel) {
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		if (*p.pCancel) break;
		{
			std::lock_guard<std::mutex> lock(p.m);
			di = p.drawQueue.front();
			p.drawQueue.pop();
			p.pf(DrawUI(di.detFrame, di.bkg, di.fgnd, di.motionDetected, di.oscillations, di.oscCount, di.dMaxContourSize, di.FrameCount, di.iBaseFrame, p.vi.dFPS, di.start, di.ft, p.vi.dFPS / p.pp.dDesiredFPS), di.FrameCount);
		}
	}
}

struct ResizeStartParams
{
	cv::VideoCapture* pvc;
	bool *bPaused;
	int* pCancel;
	int FrameCount;
	int* iJumpFrame;
	std::queue<cv::Mat> imgQueue;
	std::queue<cv::Mat> procImgQueue;
	mutable std::mutex m;
	ProcessParams pp;
};


void ResizeProcessor(ResizeStartParams& p)
{
	cv::Mat frame;
	while (p.pvc->isOpened() && !*p.pCancel) {
		while (p.imgQueue.size() > 10 && !*p.pCancel && *p.iJumpFrame == -1) {
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		if (*p.bPaused && !*p.pCancel) {
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
			if (*p.bPaused && !*p.pCancel) {
				continue;
			}
		}
		if (*p.pCancel) break;
		*p.pvc >> frame; //calls read which is the same as grab/retrieve combined
		if (!*p.pCancel && frame.dims != 0 && p.FrameCount != p.pp.iMaxFrames) {
			if (!p.pp.noProcessing) {
				cv::Mat detFrame = frame.clone();
				if (p.pp.iMinHeight != 0 || p.pp.iMinWidth != 0) cv::resize(frame, frame, cv::Size(p.pp.iMinWidth, p.pp.iMinHeight), 0, 0, cv::INTER_AREA); //cv::INTER_LANCZOS4, cv:INTER_LINEAR for zooming, cv::INTER_CUBIC slow for zooming, cv::INTER_AREA for shrinking
				if (p.pp.bGrayScale) cvtColor(frame, frame, CV_BGR2GRAY);
				{
					std::lock_guard<std::mutex> lock(p.m);
					p.imgQueue.push(detFrame);
					p.procImgQueue.push(frame);
				}
			} else {
				std::lock_guard<std::mutex> lock(p.m);
				p.imgQueue.push(frame);
			}
		}
	}
}

void ProcessVideo(VidInfo vi, ProcessParams pp, cv::VideoCapture & pvc, std::vector<double> & motionDetected, std::vector<double> & oscillations, std::vector<double> & oscCount, std::vector<double> & ft, double & dMaxContourSize, double & dMaxOscillation, std::function<ProgressFunc> pf, int* pCancel, int* iJumpFrame, bool* bPaused)
{
	cv::Mat frame, fgimg;
	double dSumContourSize;
	cv::Ptr<cv::BackgroundSubtractor> bg;
	std::vector<std::vector<cv::Point>> contours;
	std::vector<double> areas;
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
	int iBaseFrame = 0;
	//cv::BFMatcher::create();
	matcher = cv::FlannBasedMatcher::create();
	//matcher = cv::FlannBasedMatcher::FlannBasedMatcher();
	ResizeStartParams params = {};
	params.bPaused = bPaused;
	params.pCancel = pCancel;
	params.FrameCount = FrameCount;
	params.pp = pp;
	params.pvc = &pvc;
	params.iJumpFrame = iJumpFrame;
	std::thread resizeProc(&ResizeProcessor, std::ref(params));
	DrawStartParams drawParams = {};
	drawParams.pp = pp;
	drawParams.pCancel = pCancel;
	drawParams.vi = vi;
	drawParams.pf = pf;
	std::thread drawProc(&DrawProcessor, std::ref(drawParams));
	while (pvc.isOpened()) {
		if (*bPaused && !*pCancel) {
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
			if (*bPaused && !*pCancel) {
				continue;
			}
		}
		int iJump;
		if ((iJump = *iJumpFrame) != -1) {
			iBaseFrame = iJump;
			*iJumpFrame = -1;
			pvc.set(CV_CAP_PROP_POS_FRAMES, pp.noProcessing ? iJump : ((iJump > frameHistory) ? iJump - frameHistory : 0)); //CV_CAP_PROP_POS_MSEC
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
			{
				std::lock_guard<std::mutex> lock(drawParams.m);
				while (!drawParams.drawQueue.empty()) drawParams.drawQueue.pop();
			}
			int doCancel = 1;
			params.pCancel = &doCancel;
			resizeProc.join(); //to avoid race conditions and properly synchronize, restart option utilized for jump
			params.pCancel = pCancel;
			params.FrameCount = iJump;
			while (!params.imgQueue.empty()) params.imgQueue.pop();
			while (!params.procImgQueue.empty()) params.procImgQueue.pop();
			resizeProc = std::thread(&ResizeProcessor, std::ref(params));
		}

		/*while (vc.grab()) {
			cv::OutputArray img();
			vc.retrieve(img);
		}*/
		//pvc >> frame; //calls read which is the same as grab/retrieve combined
		while (params.imgQueue.size() == 0) {
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		{
			std::lock_guard<std::mutex> lock(params.m);
			frame = params.imgQueue.front();
			params.imgQueue.pop();
		}
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
				cv::Mat detFrame = frame;// frame.clone();
				//conversion for processing
				//if (pp.iMinHeight != 0 || pp.iMinWidth != 0) cv::resize(frame, frame, cv::Size(pp.iMinWidth, pp.iMinHeight), 0, 0, cv::INTER_AREA); //cv::INTER_LANCZOS4, cv:INTER_LINEAR for zooming, cv::INTER_CUBIC slow for zooming, cv::INTER_AREA for shrinking
				//if (pp.bGrayScale) cvtColor(frame, frame, CV_BGR2GRAY);
				while (params.procImgQueue.size() == 0) {
					std::this_thread::sleep_for(std::chrono::milliseconds(10));
				}
				{
					std::lock_guard<std::mutex> lock(params.m);
					frame = params.procImgQueue.front();
					params.procImgQueue.pop();
				}
				//reference frame is the lowest in callibration window
				if (oscillations.size() == 0) {
					cv::goodFeaturesToTrack(frame, lastpts, 100, 0.3, 7);
					cv::cornerSubPix(frame, lastpts, cv::Size(10, 10), cv::Size(-1, -1), termcrit);
					cv::buildOpticalFlowPyramid(frame, pyramid, cv::Size(21, 21), 3);
				}
				cv::buildOpticalFlowPyramid(frame, nextpyr, cv::Size(21, 21), 3);
				//best features are the ones closest to the motion window
				cv::calcOpticalFlowPyrLK(pyramid, nextpyr, lastpts, pts, status, err);
				if (oscillations.size() == 0) { nextstatus = status; nextpts = pts; }
				//cv::calcOpticalFlowFarneback(bkgnd, frame, pyramid, 0.5, 3, 15, 3, 5, 1.2, 0);
				double totaldist = 0, totalneg = 0;
				int totalosc = 0, totalnegosc = 0;
				int sz = pts.size();
				for (int i = 0; i < sz; i++) {
					if (!status[i] || !nextstatus[i]) continue;
					//if (err.at<float>(cv::Point(0, i)) > 2) continue;;
					double dist = cv::norm(pts[i] - nextpts[i]);
					//based on reference point - either fixed in image preferably by gravity vector but hard to determine
					//or based on point in background image, must determine which are positive and negative distance contributors
					//choose most negative point in calibration window
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
				std::vector<int> v(std::min((int)oscillations.size(), frameHistory));
				std::iota(v.begin(), v.end(), 0);
				int minidx = *std::min_element(v.begin(), v.end(), [oscillations, oscCount](int idx1, int idx2) ->
					bool { return oscillations[idx1] / oscCount[idx1] < oscillations[idx2] / oscCount[idx2]; });
				if (minidx == v.back()) {
					cv::goodFeaturesToTrack(frame, lastpts, 100, 0.3, 7);
					cv::cornerSubPix(frame, lastpts, cv::Size(10, 10), cv::Size(-1, -1), termcrit);
					cv::buildOpticalFlowPyramid(frame, pyramid, cv::Size(21, 21), 3);
					cv::buildOpticalFlowPyramid(frame, nextpyr, cv::Size(21, 21), 3);
					//best features are the ones closest to the motion window
					cv::calcOpticalFlowPyrLK(pyramid, nextpyr, lastpts, pts, status, err);
					nextstatus = status; nextpts = pts;
				}

				if (FrameCount % UseEvery == 0) {

					//motion detection
					bg->apply(frame, fgimg);
					//cv::erode(fgimg, fgimg, cv::Mat(), cv::Point(-1, -1), 2);
					//cv::dilate(fgimg, fgimg, cv::Mat(), cv::Point(-1, -1), 2);
					//if (FrameCount == 0) { FrameCount++; continue; }
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

						//std::vector<std::vector<cv::Point>> convexHulls;
						//std::transform(contours.begin(), contours.end(), std::back_inserter(convexHulls), [](std::vector<cv::Point>& v) { std::vector<cv::Point> hull; cv::convexHull(v, hull); return hull; });
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
					if (ft.size() == 0) nextellipse = ellipse.clone();
					else cv::bitwise_and(lastframe, mask, nextellipse);
					if (ft.size() == 0) {
						bkgnd = nextellipse;
						cv::bitwise_and(bkgnd, mask, refellipse);
					}

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

					std::vector<int> v(std::min((int)ft.size(), frameHistory));
					std::iota(v.begin(), v.end(), 0);
					int minidx = *std::min_element(v.begin(), v.end(), [ft](int idx1, int idx2) ->
						bool { return ft[idx1] < ft[idx2]; });
					if (minidx == v.back()) {
						bkgnd = nextellipse;
						cv::bitwise_and(bkgnd, mask, refellipse);
					}
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
						//bg.get()->getBackgroundImage(bkgnd);
						cv::Mat bkg;
						cv::cvtColor(bkgnd, bkg, pp.bGrayScale ? CV_GRAY2RGB : CV_BGR2RGB);

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
							std::transform(lastkps.begin(), lastkps.end(), std::back_inserter(kpoints), [&pp, &vi](cv::KeyPoint& v) { return cv::KeyPoint(cv::Point2f(v.pt.x * vi.dwWidthInPixels / pp.iMinWidth, v.pt.y * vi.dwHeightInPixels / pp.iMinHeight), v.size * vi.dwWidthInPixels / pp.iMinWidth, v.angle, v.response, v.octave, v.class_id); });
							cv::drawKeypoints(detFrame, kpoints, detFrame);
						}
						else cv::drawKeypoints(detFrame, lastkps, detFrame);
						sz = pts.size();
						for (int i = 0; i < sz; i++) {
							if (!status[i]) continue;
							if (pp.iMinHeight != 0 || pp.iMinWidth != 0) {
								cv::circle(detFrame, cv::Point2f(pts[i].x * vi.dwWidthInPixels / pp.iMinWidth, pts[i].y * vi.dwHeightInPixels / pp.iMinHeight), 5, cv::Scalar(255, 0, 0), 2);
							}
							else cv::circle(detFrame, pts[i], 5, cv::Scalar(255, 0, 0), 2);
						}

						//cv::drawMatches(frame, kps, frame, kps, matches, frame);

						//cvtColor(detFrame, detFrame, CV_BGR2RGB);

						{
							std::lock_guard<std::mutex> lock(drawParams.m);
							drawParams.drawQueue.push(DrawItem{detFrame.clone(), bkg.clone(), fgnd.clone(), motionDetected, oscillations, oscCount, dMaxContourSize, FrameCount, iBaseFrame, start, ft});
						}

						//pf(DrawUI(detFrame, bkg, fgnd, motionDetected, oscillations, oscCount, dMaxContourSize, FrameCount, iBaseFrame, vi.dFPS, start, ft, vi.dFPS / pp.dDesiredFPS), FrameCount);
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
	resizeProc.join();
	drawProc.join();
}

struct StartParams
{
	cv::VideoCapture* pvc;
	VidInfo vi;
	std::vector<double> motionDetect;
	std::vector<double> osc;
	std::vector<double> oscCount;
	std::vector<double> ft;
	double dMaxContour;
	double dMaxOsc;
	int pc = 0;
	std::queue<cv::Mat> imgQueue;
	std::queue<int> imgFrameNum;
	mutable std::mutex m;
	int iCurPos;
	int iJumpFrame = -1;
	int iBaseFrame = 0;
	bool bPause;
	double playbackRate;
	std::chrono::time_point<std::chrono::high_resolution_clock> start;
	std::chrono::time_point<std::chrono::high_resolution_clock> pausedat;
	std::vector<int> breathPos;
	ProcessParams pp;
};

void VideoProcessor(StartParams& p)
{
	ProcessVideo(p.vi, p.pp, *p.pvc, p.motionDetect, p.osc, p.oscCount, p.ft, p.dMaxContour, p.dMaxOsc,
		std::move(std::function<ProgressFunc>([&p](cv::Mat mat, int Frame) -> void {
		while (p.imgQueue.size() > 10 && !p.pc && p.iJumpFrame == -1) {
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		if (p.iJumpFrame != -1) return;
		std::lock_guard<std::mutex> lock(p.m);
		p.imgFrameNum.push(Frame);
		p.imgQueue.push(mat);
	})), &p.pc, &p.iJumpFrame, &p.bPause);
}

void ChangeVideoPos(int Pos, void* p)
{
	if (((StartParams*)p)->iCurPos != Pos) {
		((StartParams*)p)->iBaseFrame = Pos;
		((StartParams*)p)->iJumpFrame = Pos;
		if (((StartParams*)p)->pp.noProcessing) {
			std::vector<int> fixBreath;
			std::copy_if(((StartParams*)p)->breathPos.begin(), ((StartParams*)p)->breathPos.end(), std::back_inserter(fixBreath), [Pos](int i) { return i <= Pos; });
			if (fixBreath.size() % 2 == 1) fixBreath.pop_back();
			((StartParams*)p)->breathPos = fixBreath;
		}
		((StartParams*)p)->start = std::chrono::high_resolution_clock::now() - std::chrono::milliseconds((int)(((StartParams*)p)->playbackRate * Pos / ((StartParams*)p)->vi.dFPS));
		std::lock_guard<std::mutex> lock(((StartParams*)p)->m);
		((StartParams*)p)->imgQueue = std::queue<cv::Mat>();
		((StartParams*)p)->imgFrameNum = std::queue<int>();
	}
}

void callbackPause(int, void* p)
{
	((StartParams*)p)->bPause = !((StartParams*)p)->bPause;
	if (((StartParams*)p)->bPause) ((StartParams*)p)->pausedat = std::chrono::high_resolution_clock::now();
	else ((StartParams*)p)->start += (std::chrono::high_resolution_clock::now() - ((StartParams*)p)->pausedat);
}

void VideoMouseEvent(int event, int x, int y, int flags, void* userdata)
{
	cv::Rect button(620, 10, 20, 60);
	if (event == cv::EVENT_LBUTTONDOWN) {
		if (!button.contains(cv::Point(x, y))) ((StartParams*)userdata)->breathPos.push_back(((StartParams*)userdata)->iCurPos);
	} else if (event == cv::EVENT_LBUTTONUP) {
		if (button.contains(cv::Point(x, y))) {
			if (y < 30) callbackPause(0, userdata);
			else if (y < 50) {
				((StartParams*)userdata)->playbackRate /= 2;
				((StartParams*)userdata)->start += (std::chrono::high_resolution_clock::now() - ((StartParams*)userdata)->start) / 2;
			} else {
				((StartParams*)userdata)->playbackRate *= 2;
				((StartParams*)userdata)->start -= (std::chrono::high_resolution_clock::now() - ((StartParams*)userdata)->start);
			}
		} else if (((StartParams*)userdata)->pp.noProcessing) ((StartParams*)userdata)->breathPos.push_back(((StartParams*)userdata)->iCurPos);
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
	params.playbackRate = 1000;
	params.pp = ProcessParams{ true, 200, 150, -1, 10, true, 10, true, 3, 25, 0, 0, true, true, true, false };
	if (params.pp.noProcessing) params.breathPos.clear();
	cv::setMouseCallback(WINDOWNAME, VideoMouseEvent, &params);
	cv::createTrackbar(TRACKBARNAME, WINDOWNAME, NULL, params.vi.dFrameCount, ChangeVideoPos, &params);
	//cv::createButton(PAUSEBTN, callbackPause, &params, CV_PUSH_BUTTON);
	std::thread vidProc(&VideoProcessor, std::ref(params));
	params.start = std::chrono::high_resolution_clock::now();
	while (true) {
		int idx = -1;
		cv::Mat mat;
		//locking scope, not a unique_lock because only used once
		{
			std::lock_guard<std::mutex> lock(params.m);
			if (params.imgQueue.size() != 0) {
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
			}
		}
		if (idx != -1) {
			char buf[256];
			if (params.breathPos.size() != 0) {
				double dScale = 1;// params.pp.dDesiredFPS / params.vi.dFPS;
				cv::Mat timeline = cv::Mat::zeros(30, std::min(640, (1 + (int)((idx - params.iBaseFrame) * dScale))), CV_8UC3);
				int i, iBase = -1, sz = params.breathPos.size(), basePos = std::max(0, (int)((idx - params.iBaseFrame) * dScale) - 640);
				for (i = 0; i < sz; i++) {
					if (params.breathPos[i] >= params.iBaseFrame && params.breathPos[i] <= idx) {
						if ((params.breathPos[i] - params.iBaseFrame) * dScale < basePos) continue;
						if (iBase == -1) iBase = i;
						//timeline.at<cv::Vec3b>(cv::Point((params.breathPos[i] - params.iBaseFrame) * dScale, 0)) = i % 2 == 0 ? cv::Vec3b(0, 255, 0) : cv::Vec3b(0, 0, 255);
						cv::line(timeline, cv::Point((params.breathPos[i] - params.iBaseFrame) * dScale - basePos, 0), cv::Point((params.breathPos[i] - params.iBaseFrame) * dScale - basePos, 29), i % 2 == 0 ? cv::Vec3b(0, 255, 0) : cv::Vec3b(0, 0, 255));
					} else if (params.breathPos[i] > idx) break;
				}
				//cv::resize(timeline, timeline, cv::Size(std::min(640, (1 + (int)((idx - params.iBaseFrame) * dScale))), 30), cv::INTER_MAX);

				timeline.copyTo(mat(cv::Rect(0, params.pp.noProcessing ? 480 : 540, std::min(640, (1 + (int)((idx - params.iBaseFrame) * dScale))), 30)));

				sprintf(buf, "Breaths: %lu", iBase == -1 ? 0 : (i - iBase) / 2);
				int baseLine = 0;
				cv::Size s = cv::getTextSize(cv::String(buf), cv::FONT_HERSHEY_PLAIN, 1, 1, &baseLine);
				cv::putText(mat, cv::String(buf), cv::Point((640 - s.width) / 2, (params.pp.noProcessing ? 480 : 540) + 10), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255, 255, 255));
			}
			cv::Rect button(620, 10, 20, 20);
			mat(button) = cv::Vec3b(200, 200, 200);
			cv::putText(mat(button), cv::String("\""), cv::Point(2, 27), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 0), 3);
			button = cv::Rect(620, 30, 20, 20);
			mat(button) = cv::Vec3b(200, 200, 200);
			cv::putText(mat(button), cv::String("+"), cv::Point(4, 18), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(0, 0, 0), 3);
			button = cv::Rect(620, 50, 20, 20);
			mat(button) = cv::Vec3b(200, 200, 200);
			cv::putText(mat(button), cv::String("-"), cv::Point(4, 18), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(0, 0, 0), 3);
			button = cv::Rect(580, 70, 60, 20);
			sprintf(buf, "x%.2f", 1000 / params.playbackRate);
			cv::putText(mat(button), cv::String(buf), cv::Point(0, 18), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255, 255, 255), 1);
				
			cvSetTrackbarPos(TRACKBARNAME, WINDOWNAME, params.iCurPos = idx);
			cv::imshow(WINDOWNAME, mat);
		}
		std::chrono::duration<double, std::milli> elapsed = std::chrono::high_resolution_clock::now() - params.start;
		int c = cvWaitKey(idx == -1 ? 1 : std::max(1, (int)(params.playbackRate * params.iCurPos / params.vi.dFPS - elapsed.count()))); if (c == 27) break;
	}
	params.pc = 1;
	vidProc.join();
	if (params.pp.noProcessing && params.breathPos.size() != 0) {
		std::ofstream breathfile;
		breathfile.open(metaFile);
		breathfile << params.breathPos.size() << std::endl;
		for (int i = 0; i < params.breathPos.size(); i++) {
			breathfile << params.breathPos[i] << std::endl;
		}
		breathfile.close();
	}
	if (params.motionDetect.size() != 0) {
		std::string metaSignalFile = (argc == FIRST_ARG) ? "cap0.sig.md" : (std::string(argv[FIRST_ARG]).substr(std::string(argv[FIRST_ARG]).find_last_of("/\\") + 1) + ".sig.md");
		std::string metaResultFile = (argc == FIRST_ARG) ? "cap0.res.md" : (std::string(argv[FIRST_ARG]).substr(std::string(argv[FIRST_ARG]).find_last_of("/\\") + 1) + ".res.md");
		std::vector<double> finalpcts;
		std::set<int> maxindexes, minindexes;
		calcResults(params.motionDetect, params.osc, params.oscCount, params.ft, finalpcts, maxindexes, minindexes, -1);
		std::ofstream signalfile;
		signalfile.open(metaSignalFile);
		signalfile << finalpcts.size() << std::endl;
		for (int i = 0; i < finalpcts.size(); i++) {
			signalfile << finalpcts[i] << std::endl;
		}
		signalfile.close();
		std::vector<int> calcBreath;
		std::copy(maxindexes.begin(), maxindexes.end(), std::back_inserter(calcBreath));
		std::copy(minindexes.begin(), minindexes.end(), std::back_inserter(calcBreath));
		std::sort(calcBreath.begin(), calcBreath.end());
		std::ofstream breathfile;
		breathfile.open(metaResultFile);
		breathfile << calcBreath.size() << std::endl;
		for (int i = 0; i < calcBreath.size(); i++) {
			breathfile << calcBreath[i] << std::endl;
		}
		breathfile.close();
	}
	cvDestroyWindow(WINDOWNAME);
	VideoCleanup((void*)params.pvc);
	return 0;
}