# SfmDataGenerator
## Overview
SfmDataGenerator is a software for generating 3D computer vision datasets:
 * Structure from Motion (SfM) datasets
 * RGB-D datasets
 * Silhouette reconstruction datasets

The dataset is being created by moving virtual camera around a 3D-model and taking photos from different viewpoints. SfmDataGenerator supports [PLY](http://paulbourke.net/dataformats/ply/) and [OBJ](http://paulbourke.net/dataformats/obj/) models. Virtual camera is fully consistent with the *pinhole camera* model.

Generated dataset (camera parameters for each viewpoint & 3d point cloud if provided) may be stored in *txt*, *yml* or *xml* files. Images & silhouettes are saved to *png* files, depth images are saved to floating point *exr* image.

[Demo]()

## Pre-requisite
SfmDataGenerator is written using OpenCV library with modified Viz module, which you can build from [repository](https://github.com/helgui/opencv). If you already have an OpenCV library installed, you only need to rebuild Viz module.
## Use
See [manual](MANUAL.md)
## Author
Oleg Yakovlev
