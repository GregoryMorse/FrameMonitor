REM merge pull request, plus sources\modules\imgproc\src\smooth.cpp change line: C:\Users\Gregory\Documents\My Libraries\opencv330\opencv\sources\modules\imgproc\src\smooth.cpp(3961): error C2664: '__n128 __neon_QdQnQm(unsigned int,__n128,__n128)': cannot convert argument 3 from 'volatile const float32x4_t' to '__n128'
REM C:\Users\Gregory\Documents\My Libraries\opencv330\opencv\sources\modules\imgproc\src\smooth.cpp(3961): note: Cannot copy construct union '__n128' due to ambiguous copy constructors or no available copy constructor
REM add: *(float32x4_t*)& thereby: float32x4_t _alpha = vsubq_f32(_val, *(float32x4_t*)&_val0);
IF "%1"=="" (SET VSVER=11) ELSE (SET VSVER=%1)
IF "%VSVER%"=="15" (SET "msvc_path=%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Enterprise\VC\Auxiliary\Build") ELSE (SET "msvc_path=%ProgramFiles(x86)%\Microsoft Visual Studio %VSVER%.0")
IF "%VSVER%"=="15" (SET "msvc_path_arm=%msvc_path%") ELSE (SET "msvc_path_x86=%msvc_path%\VC\bin\x86_arm")
IF "%VSVER%"=="15" (SET "msvc_path_x86=%msvc_path%") ELSE (SET "msvc_path_x86=%msvc_path%\VC\bin")
IF "%VSVER%"=="15" (SET "msvc_path_x64=%msvc_path%") ELSE (SET "msvc_path_x86=%msvc_path%\VC\bin\amd64")
IF "%VSVER%"=="15" (CALL "%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Enterprise\Common7\Tools\VsDevCmd.bat" && CD "%CD%") ELSE (CALL "%msvc_path%\Common7\Tools\VsDevCmd.bat")
IF "%VSVER%"=="11" (SET OSVER=8.0) ELSE (IF "%VSVER%"=="12" (SET OSVER=8.1) ELSE (SET OSVER=10))
IF "%VSVER%"=="14" (SET "LIBPATH=%LIBPATH%;%ProgramFiles(x86)%\Microsoft Visual Studio 14.0\VC\lib\store\references")
SET sources=../../../sources
SET armtoolchain=%sources%/platforms/winrt/arm.winrt.toolchain.cmake
IF "%VSVER%"=="15" (SET cmakedir="%ProgramFiles%\CMake\bin\cmake.exe") ELSE (SET cmakedir="%ProgramFiles(x86)%\CMake\bin\cmake.exe")
IF "%2"=="x64" (SET "msvc_vars=%msvc_path_x64%\vcvars64.bat") else (IF "%2"=="x86" (SET "msvc_vars=%msvc_path_arm%\vcvars32.bat") ELSE (SET "msvc_vars=%msvc_path_arm%\vcvarsx86_arm.bat"))
IF "%3"=="cli" (SET "cmdopts=-DCMAKE_SYSTEM_VERSION=%OSVER%") ELSE (IF "%3"=="rt" (SET "cmdopts=-DCMAKE_SYSTEM_VERSION=%OSVER% -DENABLE_WINRT_MODE_NATIVE=ON") ELSE (SET "cmdopts="))
IF "%2"=="arm" (SET "cmdopts=%cmdopts% -DCMAKE_TOOLCHAIN_FILE=%sources%/platforms/winrt/arm.winrt.toolchain.cmake -DCMAKE_SYSTEM_NAME=WindowsPhone") ELSE (IF NOT "%3"=="st" (SET "cmdopts=%cmdopts% -DCMAKE_SYSTEM_NAME=WindowsStore"))
IF "%2"=="x64" (SET intdir=intel64) ELSE (SET intdir=ia32)
IF "%4"=="ON" (SET pfix=%3d) ELSE (SET pfix=%3)
REM CMakeLists.txt must comment out the CMAKE_USE_RELATIVE_PATHS ON line due to a bug leaving trailing slashes in makefiles in CMake 3.3.1
REM all static linked yet using shared Visual C libraries, Debug/Release, x86/x64/ARM, WinRT Native C++/WinRT C++ CLI (not strictly necessary as Native C++ will suffice)/Win32

RMDIR /S /Q build\%2\vc%VSVER%%pfix%
MKDIR build\%2\vc%VSVER%%pfix%

CD build\%2\vc%VSVER%%pfix%
DEL CMakeCache.txt
%COMSPEC% /S /C "CALL "%msvc_vars%" && CD %CD% && %cmakedir% -G"NMake Makefiles" -DCMAKE_BUILD_TYPE=Debug %cmdopts% -DBUILD_SHARED_LIBS=%4 -DBUILD_WITH_STATIC_CRT=%4 -DOPENCV_EXTRA_MODULES_PATH=../../../../../opencv_contrib-3.4.0/modules -DWITH_FFMPEG=ON -DWITH_MSMF=ON -DWITH_DSHOW=OFF -DWITH_VFW=OFF -DWITH_OPENEXR=OFF -DWITH_CUDA=OFF -DBUILD_opencv_gpu=OFF -DBUILD_opencv_python=OFF -DBUILD_opencv_java=OFF %sources%"
%COMSPEC% /S /C "CALL "%msvc_vars%" && CD %CD% && nmake.exe opencv_videoio && nmake.exe opencv_video && nmake.exe opencv_highgui && nmake.exe opencv_xfeatures2d"
DEL CMakeCache.txt
%COMSPEC% /S /C "CALL "%msvc_vars%" && CD %CD% && %cmakedir% -G"NMake Makefiles" -DCMAKE_BUILD_TYPE=RelWithDebInfo %cmdopts% -DBUILD_SHARED_LIBS=%4 -DBUILD_WITH_STATIC_CRT=%4 -DOPENCV_EXTRA_MODULES_PATH=../../../../../opencv_contrib-3.4.0/modules -DWITH_FFMPEG=ON -DWITH_MSMF=ON -DWITH_DSHOW=OFF -DWITH_VFW=OFF -DWITH_OPENEXR=OFF -DWITH_CUDA=OFF -DBUILD_opencv_gpu=OFF -DBUILD_opencv_python=OFF -DBUILD_opencv_java=OFF %sources%"
%COMSPEC% /S /C "CALL "%msvc_vars%" && CD %CD% && nmake.exe opencv_videoio && nmake.exe opencv_video && nmake.exe opencv_highgui && nmake.exe opencv_xfeatures2d"
CD ..\..\..

MKDIR ..\..\opencv.new\build\%2\vc%VSVER%%pfix%\lib 2>nul
MKDIR ..\..\opencv.new\build\%2\vc%VSVER%%pfix%\bin 2>nul
COPY /Y build\%2\vc%VSVER%%pfix%\3rdparty\ippicv\ippicv_win\lib\%intdir%\ippicvmt.lib ..\..\opencv.new\build\%2\vc%VSVER%%pfix%\lib
COPY /Y build\%2\vc%VSVER%%pfix%\3rdparty\ippicv\ippiw_win\lib\%intdir%\ipp_iw.lib ..\..\opencv.new\build\%2\vc%VSVER%%pfix%\lib
COPY /Y build\%2\vc%VSVER%%pfix%\3rdparty\lib\*.lib ..\..\opencv.new\build\%2\vc%VSVER%%pfix%\lib
COPY /Y build\%2\vc%VSVER%%pfix%\3rdparty\lib\*.pdb ..\..\opencv.new\build\%2\vc%VSVER%%pfix%\lib
COPY /Y build\%2\vc%VSVER%%pfix%\lib\*.lib ..\..\opencv.new\build\%2\vc%VSVER%%pfix%\lib
COPY /Y build\%2\vc%VSVER%%pfix%\lib\*.pdb ..\..\opencv.new\build\%2\vc%VSVER%%pfix%\lib
COPY /Y build\%2\vc%VSVER%%pfix%\bin\*.dll ..\..\opencv.new\build\%2\vc%VSVER%%pfix%\bin
COPY /Y build\%2\vc%VSVER%%pfix%\bin\*.pdb ..\..\opencv.new\build\%2\vc%VSVER%%pfix%\bin

RMDIR /S /Q build\%2\vc%VSVER%%pfix%
SET VSVER=
SET OSVER=