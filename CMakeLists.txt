cmake_minimum_required(VERSION 3.17)  # 3.17+ required for CMAKE_MFC_FLAG support
project(FrameMonitor)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# vcpkg integration — set VCPKG_ROOT environment variable or pass -DCMAKE_TOOLCHAIN_FILE
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")
endif()

# MFC requires Windows; use dynamic MFC (flag 2 = dynamic, 1 = static)
if(NOT WIN32)
    message(FATAL_ERROR "FrameMonitor is a Windows MFC application and can only be built on Windows.")
endif()
set(CMAKE_MFC_FLAG 2)

find_package(OpenCV REQUIRED
    COMPONENTS core imgproc imgcodecs highgui video videoio features2d flann xfeatures2d)

set(SOURCES
    DeviceEnumerator.cpp
    FrameMonitor.cpp
    FrameMonitorDlg.cpp
    stdafx.cpp
    TimelineStatic.cpp
    Video.cpp
    FrameMonitor.rc
)

set(HEADERS
    DeviceEnumerator.h
    FrameMonitor.h
    FrameMonitorDlg.h
    Resource.h
    stdafx.h
    targetver.h
    TimelineStatic.h
)

# WIN32 suppresses the console window for a Windows GUI application
add_executable(FrameMonitor WIN32 ${SOURCES} ${HEADERS})

target_compile_definitions(FrameMonitor PRIVATE
    UNICODE
    _UNICODE
    _WINDOWS
    _CRT_SECURE_NO_WARNINGS
)

# Precompiled header (stdafx.cpp creates it; Video.cpp explicitly opts out)
target_precompile_headers(FrameMonitor PRIVATE stdafx.h)
set_source_files_properties(stdafx.cpp  PROPERTIES COMPILE_FLAGS "/Ycstdafx.h")
set_source_files_properties(Video.cpp   PROPERTIES COMPILE_FLAGS "/Y-")

target_link_libraries(FrameMonitor PRIVATE ${OpenCV_LIBS})
