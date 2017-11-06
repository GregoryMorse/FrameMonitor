#!/bin/sh
#sudo apt-get install build-essential
#sudo apt-get install cmake git libgtk2.0-dev pkg-config libavcodec-dev libavformat-dev libswscale-dev
#sudo apt-get install python-dev python-numpy libtbb2 libtbb-dev libjpeg-dev libpng-dev libtiff-dev libjasper-dev libdc1394-22-dev

wget -c -O opencv_contrib-3.3.1.zip http://github.com/opencv/opencv_contrib/archive/3.3.1.zip
unzip opencv_contrib-3.3.1.zip
rm opencv_contrib-3.3.1.zip

downloadFile=opencv-3.3.1.zip
wget -c -O $downloadFile http://sourceforge.net/projects/opencvlibrary/files/opencv-unix/3.3.1/$downloadFile
unzip $downloadFile
rm $downloadFile
cd opencv-3.3.1
mkdir build
cd build

cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX=/usr/local -DBUILD_SHARED_LIBS=ON -DBUILD_WITH_STATIC_CRT=OFF -DOPENCV_EXTRA_MODULES_PATH=../../opencv_contrib-3.3.1/modules -DWITH_FFMPEG=ON -DWITH_MSMF=ON -DWITH_DSHOW=OFF -DWITH_VFW=OFF -DWITH_OPENEXR=OFF -DWITH_CUDA=OFF -DBUILD_opencv_gpu=OFF -DBUILD_opencv_python=OFF -DBUILD_opencv_java=OFF ..
make -j 4 opencv_videoio opencv_video opencv_highgui opencv_xfeatures2d

sudo make install
sudo sh -c 'echo "/usr/local/lib" > /etc/ld.so.conf.d/opencv.conf'
sudo ldconfig
cd ..
cd ..

g++ Video.cpp -std=c++11 `pkg-config --cflags --libs opencv` -lpthread
