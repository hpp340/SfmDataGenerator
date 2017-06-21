/*
	Copyright (c) 2016 Oleg Yakovlev
	This file is a part of SfmDataGenerator software
*/
#include "stdafx.h"
#include "GenHelper.h"

using namespace std;
using namespace cv;

GenHelper::GenHelper(viz::Viz3d &viz, Mat &cloud, SfmData &sfmData, const string& imgFolder, Mode mode) :
	viz(viz), cloud(cloud), sfmData(sfmData), n(cloud.cols), counter(0), imgFolder(imgFolder), camParams("Lens distortion"), mode(mode),
	camParamsDisplay(0), progressDisplay(0), cache(){
}

Point3d GenHelper::getPoint(int idx) const {
	return cloud.at<Vec3f>(idx);
}

void GenHelper::changeCameraParams() {
	showCameraParams();
	camParams.change([this]() -> void {
		this -> showCameraParams();
		auto ws = this->viz.getCamera().getWindowSize();
		auto p = this->viz.getCamera().getPrincipalPoint();
	});
	hideCameraParams();
}

void GenHelper::showCameraParams() {
	camParamsDisplay = 1;
	viz.showWidget("title", viz::WText("Camera parameters: ", Point(10, 120), 15, viz::Color::white()));
	viz.showWidget("focalx", viz::WText("Focal length (X): " + to_string(viz.getCamera().getFocalLength()[0]), Point(10, 100), 15, viz::Color::white()));
	viz.showWidget("focaly", viz::WText("Focal length (Y): " + to_string(viz.getCamera().getFocalLength()[1]), Point(10, 80), 15, viz::Color::white()));
	viz.showWidget("k1", viz::WText("Lens distrotion (K1): " + to_string(camParams.k1()), Point(10, 60), 15, viz::Color::white()));
	viz.showWidget("k2", viz::WText("Lens distortion (K2): " + to_string(camParams.k2()), Point(10, 40), 15, viz::Color::white()));
	auto ws = viz.getCamera().getWindowSize();
	viz.showWidget("imgsize", viz::WText("Image size: " + to_string(ws.width) + "x" + to_string(ws.height), Point(10, 20), 15, viz::Color::white()));
}

void GenHelper::hideCameraParams() {
	if (!camParamsDisplay) return;
	viz.removeWidget("title");
	viz.removeWidget("focalx");
	viz.removeWidget("focaly");
	viz.removeWidget("k1");
	viz.removeWidget("k2");
	viz.removeWidget("imgsize");
	camParamsDisplay = 0;
}

void GenHelper::takePhoto() {
	if (mode == Mode::SILHOUETTE) {
		takeSilhouette();
		return;
	}
	if (mode == Mode::DEPTH) {
		takeDepth();
		return;
	}
	takeUsualPhoto();
}

void GenHelper::takeUsualPhoto() {
	Size ws = viz.getCamera().getWindowSize();
	if (cache.size() != ws) {
		cache.create(ws);
	}
	cache.setTo(-1.0f);
	Camera cam(viz, camParams.k1(), camParams.k2());
	vector<Observation> view;
	Point3d winCoords;
	for (int i = 0; i < n; ++i) {
		const Point3d& pnt = getPoint(i);
		const Point3d& pntCam = cam.toCameraCoords(pnt);
		const Point2d& pntImg = cam.projectPointCamCoords(pntCam);
		
		if (pntCam.z <= 0.0 || pntImg.x < 0.0 || pntImg.x > ws.width ||
			pntImg.y < 0.0 || pntImg.y > ws.height) {
			continue;
		}

		viz.convertToWindowCoordinates(pnt, winCoords);

		//Depth testing
		Point proj((int)round(winCoords.x), (int)round(winCoords.y));
		float &d = cache.at<float>(proj);
		if (d < 0.0f) {
			d = viz.getDepth(proj);
		}
		if (d != 1.0 && winCoords.z  > d + 1e-3) continue;
		view.emplace_back(i, pntImg);
	}
	Mat img = viz.getScreenshot();
	ostringstream os;
	counter++;
	os << imgFolder << "/" << setw(6) << setfill('0') << counter << ".png";
	Mat undist;
	vector<Vec2f> dist;
	for (int i = 0; i < img.rows; ++i) {
		for (int j = 0; j < img.cols; ++j) {
			dist.emplace_back((float)j, (float)i);
		}
	}
	undistortPoints(dist, undist, cam.K, vector<double>{ cam.k1, cam.k2, 0.0, 0.0 }, noArray(), cam.K);
	undist = undist.reshape(0, img.rows);
	Mat newImg;
	remap(img, newImg, undist, noArray(), INTER_LINEAR, BORDER_REPLICATE);
	imwrite(os.str(), newImg);
	sfmData.addView(cam, view, os.str());
}

void GenHelper::takeSilhouette() {
	Size ws = viz.getCamera().getWindowSize();
	Camera cam(viz, 0.0, 0.0);
	vector<Observation> view;
	Mat screen(ws, CV_8UC3);
	for (int i = 0; i < ws.height; ++i) {
		for (int j = 0; j < ws.width; ++j) {
			if (viz.getDepth(Point(j, ws.height - i - 1)) == 1.0) {
				screen.at<Vec3b>(i, j) = Vec3b(0, 0, 0);
			}
			else {
				screen.at<Vec3b>(i, j) = Vec3b(255, 255, 255);
			}
		}
	}
	ostringstream os;
	counter++;
	os << imgFolder << "/" << setw(6) << setfill('0') << counter << ".png";
	imwrite(os.str(), screen);
	sfmData.addView(cam, view, os.str());
}

void GenHelper::showProgress() {
	progressDisplay = 1;
	viz.showWidget("progress", viz::WText("Processing...", Point(10, 20), 15, viz::Color::white()));
}

void GenHelper::hideProgress() {
	if (!progressDisplay) return;
	viz.removeWidget("progress");
}

void GenHelper::takeDepth() {
	Size ws = viz.getCamera().getWindowSize();
	Camera cam(viz, 0.0, 0.0);
	Vec2d clip = viz.getCamera().getClip();
	Mat1f depth = Mat1f::zeros(ws);
	View view;
	//showProgress();
	//viz.spinOnce(1, false);
	for (int i = 0; i < ws.height; ++i) {
		for (int j = 0; j < ws.width; ++j) {
			double d = viz.getDepth(Point(j, i));
			if (d == 1.0) continue;
			double depthSample = 2.0 * d - 1.0;
			d = 2.0*d - 1.0;
			//convert OpenGL depth to metric depth
			depth(ws.height - i - 1, j) = float((2.0 * clip[0] * clip[1]) / (clip[1] + clip[0] - depthSample * (clip[1] - clip[0])));
		}
	}
	ostringstream os;
	++counter;
	os << imgFolder << "/" << setw(6) << setfill('0') << counter << ".exr";
	imwrite(os.str(), depth);
	sfmData.addView(cam, view, os.str());
	//hideProgress();
}