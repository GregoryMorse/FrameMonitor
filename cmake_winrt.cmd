IF "%1"=="" (SET VSVER=11) ELSE (SET VSVER=%1)
SET msvc_path=C:\Program Files (x86)\Microsoft Visual Studio %VSVER%.0
CALL "%msvc_path%\Common7\Tools\VsDevCmd.bat"
IF "%VSVER%"=="11" (SET OSVER=8.0) ELSE (IF "%VSVER%"=="12" (SET OSVER=8.1) ELSE (SET OSVER=10))
IF "%VSVER%"=="14" (SET "LIBPATH=%LIBPATH%;C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\lib\store\references")

REM CMakeLists.txt must comment out the CMAKE_USE_RELATIVE_PATHS ON line due to a bug leaving trailing slashes in makefiles in CMake 3.3.1
REM all static linked yet using shared Visual C libraries, Debug/Release, x86/x64/ARM, WinRT Native C++/WinRT C++ CLI (not strictly necessary as Native C++ will suffice)/Win32

RMDIR /S /Q build\arm\vc%VSVER%cli
RMDIR /S /Q build\x86\vc%VSVER%cli
RMDIR /S /Q build\x64\vc%VSVER%cli
MKDIR build\ARM\vc%VSVER%cli
MKDIR build\x86\vc%VSVER%cli
MKDIR build\x64\vc%VSVER%cli

CD build\ARM\vc%VSVER%cli
DEL CMakeCache.txt
%COMSPEC% /S /C "CALL "%msvc_path%\VC\bin\x86_arm\vcvarsx86_arm.bat" && "C:\Program Files (x86)\CMake\bin\cmake.exe" -G"NMake Makefiles" -DCMAKE_BUILD_TYPE=Debug -DBUILD_SHARED_LIBS=OFF -DBUILD_WITH_STATIC_CRT=OFF -DCMAKE_SYSTEM_NAME=WindowsPhone -DCMAKE_SYSTEM_VERSION=%OSVER% -DWITH_FFMPEG=OFF -DWITH_MSMF=OFF -DWITH_DSHOW=OFF -DWITH_VFW=OFF -DWITH_OPENEXR=OFF -DWITH_CUDA=OFF -DBUILD_opencv_gpu=OFF -DBUILD_opencv_python=OFF -DBUILD_opencv_java=OFF -DCMAKE_TOOLCHAIN_FILE=../../../sources/platforms/winrt/arm.winrt.toolchain.cmake ../../../sources"
%COMSPEC% /S /C "CALL "%msvc_path%\VC\bin\x86_arm\vcvarsx86_arm.bat" && nmake.exe opencv_video"
DEL CMakeCache.txt
%COMSPEC% /S /C "CALL "%msvc_path%\VC\bin\x86_arm\vcvarsx86_arm.bat" && "C:\Program Files (x86)\CMake\bin\cmake.exe" -G"NMake Makefiles" -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_SHARED_LIBS=OFF -DBUILD_WITH_STATIC_CRT=OFF -DCMAKE_SYSTEM_NAME=WindowsPhone -DCMAKE_SYSTEM_VERSION=%OSVER% -DWITH_FFMPEG=OFF -DWITH_MSMF=OFF -DWITH_DSHOW=OFF -DWITH_VFW=OFF -DWITH_OPENEXR=OFF -DWITH_CUDA=OFF -DBUILD_opencv_gpu=OFF -DBUILD_opencv_python=OFF -DBUILD_opencv_java=OFF -DCMAKE_TOOLCHAIN_FILE=../../../sources/platforms/winrt/arm.winrt.toolchain.cmake ../../../sources"
%COMSPEC% /S /C "CALL "%msvc_path%\VC\bin\x86_arm\vcvarsx86_arm.bat" && nmake.exe opencv_video"
CD ..\..\..

CD build\x86\vc%VSVER%cli
DEL CMakeCache.txt
%COMSPEC% /S /C "CALL "%msvc_path%\VC\bin\vcvars32.bat" && "C:\Program Files (x86)\CMake\bin\cmake.exe" -G"NMake Makefiles" -DCMAKE_BUILD_TYPE=Debug -DBUILD_SHARED_LIBS=OFF -DBUILD_WITH_STATIC_CRT=OFF -DCMAKE_SYSTEM_NAME=WindowsStore -DCMAKE_SYSTEM_VERSION=%OSVER% -DWITH_FFMPEG=OFF -DWITH_MSMF=OFF -DWITH_DSHOW=OFF -DWITH_VFW=OFF -DWITH_OPENEXR=OFF -DWITH_CUDA=OFF -DBUILD_opencv_gpu=OFF -DBUILD_opencv_python=OFF -DBUILD_opencv_java=OFF ../../../sources"
%COMSPEC% /S /C "CALL "%msvc_path%\VC\bin\vcvars32.bat" && nmake.exe opencv_video"
DEL CMakeCache.txt
%COMSPEC% /S /C "CALL "%msvc_path%\VC\bin\vcvars32.bat" && "C:\Program Files (x86)\CMake\bin\cmake.exe" -G"NMake Makefiles" -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_SHARED_LIBS=OFF -DBUILD_WITH_STATIC_CRT=OFF -DCMAKE_SYSTEM_NAME=WindowsStore -DCMAKE_SYSTEM_VERSION=%OSVER% -DWITH_FFMPEG=OFF -DWITH_MSMF=OFF -DWITH_DSHOW=OFF -DWITH_VFW=OFF -DWITH_OPENEXR=OFF -DWITH_CUDA=OFF -DBUILD_opencv_gpu=OFF -DBUILD_opencv_python=OFF -DBUILD_opencv_java=OFF ../../../sources"
%COMSPEC% /S /C "CALL "%msvc_path%\VC\bin\vcvars32.bat" && nmake.exe opencv_video"
CD ..\..\..

CD build\x64\vc%VSVER%cli
DEL CMakeCache.txt
%COMSPEC% /S /C "CALL "%msvc_path%\VC\bin\amd64\vcvars64.bat" && "C:\Program Files (x86)\CMake\bin\cmake.exe" -G"NMake Makefiles" -DCMAKE_BUILD_TYPE=Debug -DBUILD_SHARED_LIBS=OFF -DBUILD_WITH_STATIC_CRT=OFF -DCMAKE_SYSTEM_NAME=WindowsStore -DCMAKE_SYSTEM_VERSION=%OSVER% -DWITH_FFMPEG=OFF -DWITH_MSMF=OFF -DWITH_DSHOW=OFF -DWITH_VFW=OFF -DWITH_OPENEXR=OFF -DWITH_CUDA=OFF -DBUILD_opencv_gpu=OFF -DBUILD_opencv_python=OFF -DBUILD_opencv_java=OFF ../../../sources"
%COMSPEC% /S /C "CALL "%msvc_path%\VC\bin\amd64\vcvars64.bat" && nmake.exe opencv_video"
DEL CMakeCache.txt
%COMSPEC% /S /C "CALL "%msvc_path%\VC\bin\amd64\vcvars64.bat" && "C:\Program Files (x86)\CMake\bin\cmake.exe" -G"NMake Makefiles" -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_SHARED_LIBS=OFF -DBUILD_WITH_STATIC_CRT=OFF -DCMAKE_SYSTEM_NAME=WindowsStore -DCMAKE_SYSTEM_VERSION=%OSVER% -DWITH_FFMPEG=OFF -DWITH_MSMF=OFF -DWITH_DSHOW=OFF -DWITH_VFW=OFF -DWITH_OPENEXR=OFF -DWITH_CUDA=OFF -DBUILD_opencv_gpu=OFF -DBUILD_opencv_python=OFF -DBUILD_opencv_java=OFF ../../../sources"
%COMSPEC% /S /C "CALL "%msvc_path%\VC\bin\amd64\vcvars64.bat" && nmake.exe opencv_video"
CD ..\..\..

RMDIR /S /Q build\arm\vc%VSVER%rt
RMDIR /S /Q build\x86\vc%VSVER%rt
RMDIR /S /Q build\x64\vc%VSVER%rt
MKDIR build\ARM\vc%VSVER%rt
MKDIR build\x86\vc%VSVER%rt
MKDIR build\x64\vc%VSVER%rt

CD build\ARM\vc%VSVER%rt
DEL CMakeCache.txt
%COMSPEC% /S /C "CALL "%msvc_path%\VC\bin\x86_arm\vcvarsx86_arm.bat" && "C:\Program Files (x86)\CMake\bin\cmake.exe" -G"NMake Makefiles" -DCMAKE_BUILD_TYPE=Debug -DBUILD_SHARED_LIBS=OFF -DBUILD_WITH_STATIC_CRT=OFF -DCMAKE_SYSTEM_NAME=WindowsPhone -DCMAKE_SYSTEM_VERSION=%OSVER% -DENABLE_WINRT_MODE_NATIVE=ON -DWITH_FFMPEG=OFF -DWITH_MSMF=OFF -DWITH_DSHOW=OFF -DWITH_VFW=OFF -DWITH_OPENEXR=OFF -DWITH_CUDA=OFF -DBUILD_opencv_gpu=OFF -DBUILD_opencv_python=OFF -DBUILD_opencv_java=OFF -DCMAKE_TOOLCHAIN_FILE=../../../sources/platforms/winrt/arm.winrt.toolchain.cmake ../../../sources"
%COMSPEC% /S /C "CALL "%msvc_path%\VC\bin\x86_arm\vcvarsx86_arm.bat" && nmake.exe opencv_video"
DEL CMakeCache.txt
%COMSPEC% /S /C "CALL "%msvc_path%\VC\bin\x86_arm\vcvarsx86_arm.bat" && "C:\Program Files (x86)\CMake\bin\cmake.exe" -G"NMake Makefiles" -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_SHARED_LIBS=OFF -DBUILD_WITH_STATIC_CRT=OFF -DCMAKE_SYSTEM_NAME=WindowsPhone -DCMAKE_SYSTEM_VERSION=%OSVER% -DENABLE_WINRT_MODE_NATIVE=ON -DWITH_FFMPEG=OFF -DWITH_MSMF=OFF -DWITH_DSHOW=OFF -DWITH_VFW=OFF -DWITH_OPENEXR=OFF -DWITH_CUDA=OFF -DBUILD_opencv_gpu=OFF -DBUILD_opencv_python=OFF -DBUILD_opencv_java=OFF -DCMAKE_TOOLCHAIN_FILE=../../../sources/platforms/winrt/arm.winrt.toolchain.cmake ../../../sources"
%COMSPEC% /S /C "CALL "%msvc_path%\VC\bin\x86_arm\vcvarsx86_arm.bat" && nmake.exe opencv_video"
CD ..\..\..

CD build\x86\vc%VSVER%rt
DEL CMakeCache.txt
%COMSPEC% /S /C "CALL "%msvc_path%\VC\bin\vcvars32.bat" && "C:\Program Files (x86)\CMake\bin\cmake.exe" -G"NMake Makefiles" -DCMAKE_BUILD_TYPE=Debug -DBUILD_SHARED_LIBS=OFF -DBUILD_WITH_STATIC_CRT=OFF -DCMAKE_SYSTEM_NAME=WindowsStore -DCMAKE_SYSTEM_VERSION=%OSVER% -DENABLE_WINRT_MODE_NATIVE=ON -DWITH_FFMPEG=OFF -DWITH_MSMF=OFF -DWITH_DSHOW=OFF -DWITH_VFW=OFF -DWITH_OPENEXR=OFF -DWITH_CUDA=OFF -DBUILD_opencv_gpu=OFF -DBUILD_opencv_python=OFF -DBUILD_opencv_java=OFF ../../../sources"
%COMSPEC% /S /C "CALL "%msvc_path%\VC\bin\vcvars32.bat" && nmake.exe opencv_video"
DEL CMakeCache.txt
%COMSPEC% /S /C "CALL "%msvc_path%\VC\bin\vcvars32.bat" && "C:\Program Files (x86)\CMake\bin\cmake.exe" -G"NMake Makefiles" -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_SHARED_LIBS=OFF -DBUILD_WITH_STATIC_CRT=OFF -DCMAKE_SYSTEM_NAME=WindowsStore -DCMAKE_SYSTEM_VERSION=%OSVER% -DENABLE_WINRT_MODE_NATIVE=ON -DWITH_FFMPEG=OFF -DWITH_MSMF=OFF -DWITH_DSHOW=OFF -DWITH_VFW=OFF -DWITH_OPENEXR=OFF -DWITH_CUDA=OFF -DBUILD_opencv_gpu=OFF -DBUILD_opencv_python=OFF -DBUILD_opencv_java=OFF ../../../sources"
%COMSPEC% /S /C "CALL "%msvc_path%\VC\bin\vcvars32.bat" && nmake.exe opencv_video"
CD ..\..\..

CD build\x64\vc%VSVER%rt
DEL CMakeCache.txt
%COMSPEC% /S /C "CALL "%msvc_path%\VC\bin\amd64\vcvars64.bat" && "C:\Program Files (x86)\CMake\bin\cmake.exe" -G"NMake Makefiles" -DCMAKE_BUILD_TYPE=Debug -DBUILD_SHARED_LIBS=OFF -DBUILD_WITH_STATIC_CRT=OFF -DCMAKE_SYSTEM_NAME=WindowsStore -DCMAKE_SYSTEM_VERSION=%OSVER% -DENABLE_WINRT_MODE_NATIVE=ON -DWITH_FFMPEG=OFF -DWITH_MSMF=OFF -DWITH_DSHOW=OFF -DWITH_VFW=OFF -DWITH_OPENEXR=OFF -DWITH_CUDA=OFF -DBUILD_opencv_gpu=OFF -DBUILD_opencv_python=OFF -DBUILD_opencv_java=OFF ../../../sources"
%COMSPEC% /S /C "CALL "%msvc_path%\VC\bin\amd64\vcvars64.bat" && nmake.exe opencv_video"
DEL CMakeCache.txt
%COMSPEC% /S /C "CALL "%msvc_path%\VC\bin\amd64\vcvars64.bat" && "C:\Program Files (x86)\CMake\bin\cmake.exe" -G"NMake Makefiles" -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_SHARED_LIBS=OFF -DBUILD_WITH_STATIC_CRT=OFF -DCMAKE_SYSTEM_NAME=WindowsStore -DCMAKE_SYSTEM_VERSION=%OSVER% -DENABLE_WINRT_MODE_NATIVE=ON -DWITH_FFMPEG=OFF -DWITH_MSMF=OFF -DWITH_DSHOW=OFF -DWITH_VFW=OFF -DWITH_OPENEXR=OFF -DWITH_CUDA=OFF -DBUILD_opencv_gpu=OFF -DBUILD_opencv_python=OFF -DBUILD_opencv_java=OFF ../../../sources"
%COMSPEC% /S /C "CALL "%msvc_path%\VC\bin\amd64\vcvars64.bat" && nmake.exe opencv_video"
CD ..\..\..

RMDIR /S /Q build\arm\vc%VSVER%st
RMDIR /S /Q build\x86\vc%VSVER%st
RMDIR /S /Q build\x64\vc%VSVER%st
MKDIR build\ARM\vc%VSVER%st
MKDIR build\x86\vc%VSVER%st
MKDIR build\x64\vc%VSVER%st

CD build\ARM\vc%VSVER%st
DEL CMakeCache.txt
%COMSPEC% /S /C "CALL "%msvc_path%\VC\bin\x86_arm\vcvarsx86_arm.bat" && "C:\Program Files (x86)\CMake\bin\cmake.exe" -G"NMake Makefiles" -DCMAKE_BUILD_TYPE=Debug -DBUILD_SHARED_LIBS=OFF -DBUILD_WITH_STATIC_CRT=OFF -DWITH_FFMPEG=OFF -DWITH_MSMF=OFF -DWITH_DSHOW=OFF -DWITH_VFW=OFF -DWITH_OPENEXR=OFF -DWITH_CUDA=OFF -DBUILD_opencv_gpu=OFF -DBUILD_opencv_python=OFF -DBUILD_opencv_java=OFF -DCMAKE_TOOLCHAIN_FILE=../../../sources/platforms/winrt/arm.winrt.toolchain.cmake ../../../sources"
%COMSPEC% /S /C "CALL "%msvc_path%\VC\bin\x86_arm\vcvarsx86_arm.bat" && nmake.exe opencv_video"
DEL CMakeCache.txt
%COMSPEC% /S /C "CALL "%msvc_path%\VC\bin\x86_arm\vcvarsx86_arm.bat" && "C:\Program Files (x86)\CMake\bin\cmake.exe" -G"NMake Makefiles" -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_SHARED_LIBS=OFF -DBUILD_WITH_STATIC_CRT=OFF -DWITH_FFMPEG=OFF -DWITH_MSMF=OFF -DWITH_DSHOW=OFF -DWITH_VFW=OFF -DWITH_OPENEXR=OFF -DWITH_CUDA=OFF -DBUILD_opencv_gpu=OFF -DBUILD_opencv_python=OFF -DBUILD_opencv_java=OFF -DCMAKE_TOOLCHAIN_FILE=../../../sources/platforms/winrt/arm.winrt.toolchain.cmake ../../../sources"
%COMSPEC% /S /C "CALL "%msvc_path%\VC\bin\x86_arm\vcvarsx86_arm.bat" && nmake.exe opencv_video"
CD ..\..\..

CD build\x86\vc%VSVER%st
DEL CMakeCache.txt
%COMSPEC% /S /C "CALL "%msvc_path%\VC\bin\vcvars32.bat" && "C:\Program Files (x86)\CMake\bin\cmake.exe" -G"NMake Makefiles" -DCMAKE_BUILD_TYPE=Debug -DBUILD_SHARED_LIBS=OFF -DBUILD_WITH_STATIC_CRT=OFF -DWITH_FFMPEG=OFF -DWITH_MSMF=OFF -DWITH_DSHOW=OFF -DWITH_VFW=OFF -DWITH_OPENEXR=OFF -DWITH_CUDA=OFF -DBUILD_opencv_gpu=OFF -DBUILD_opencv_python=OFF -DBUILD_opencv_java=OFF ../../../sources"
%COMSPEC% /S /C "CALL "%msvc_path%\VC\bin\vcvars32.bat" && nmake.exe opencv_video"
DEL CMakeCache.txt
%COMSPEC% /S /C "CALL "%msvc_path%\VC\bin\vcvars32.bat" && "C:\Program Files (x86)\CMake\bin\cmake.exe" -G"NMake Makefiles" -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_SHARED_LIBS=OFF -DBUILD_WITH_STATIC_CRT=OFF -DWITH_FFMPEG=OFF -DWITH_MSMF=OFF -DWITH_DSHOW=OFF -DWITH_VFW=OFF -DWITH_OPENEXR=OFF -DWITH_CUDA=OFF -DBUILD_opencv_gpu=OFF -DBUILD_opencv_python=OFF -DBUILD_opencv_java=OFF ../../../sources"
%COMSPEC% /S /C "CALL "%msvc_path%\VC\bin\vcvars32.bat" && nmake.exe opencv_video"
CD ..\..\..

CD build\x64\vc%VSVER%st
DEL CMakeCache.txt
%COMSPEC% /S /C "CALL "%msvc_path%\VC\bin\amd64\vcvars64.bat" && "C:\Program Files (x86)\CMake\bin\cmake.exe" -G"NMake Makefiles" -DCMAKE_BUILD_TYPE=Debug -DBUILD_SHARED_LIBS=OFF -DBUILD_WITH_STATIC_CRT=OFF -DWITH_FFMPEG=OFF -DWITH_MSMF=OFF -DWITH_DSHOW=OFF -DWITH_VFW=OFF -DWITH_OPENEXR=OFF -DWITH_CUDA=OFF -DBUILD_opencv_gpu=OFF -DBUILD_opencv_python=OFF -DBUILD_opencv_java=OFF ../../../sources"
%COMSPEC% /S /C "CALL "%msvc_path%\VC\bin\amd64\vcvars64.bat" && nmake.exe opencv_video"
DEL CMakeCache.txt
%COMSPEC% /S /C "CALL "%msvc_path%\VC\bin\amd64\vcvars64.bat" && "C:\Program Files (x86)\CMake\bin\cmake.exe" -G"NMake Makefiles" -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_SHARED_LIBS=OFF -DBUILD_WITH_STATIC_CRT=OFF -DWITH_FFMPEG=OFF -DWITH_MSMF=OFF -DWITH_DSHOW=OFF -DWITH_VFW=OFF -DWITH_OPENEXR=OFF -DWITH_CUDA=OFF -DBUILD_opencv_gpu=OFF -DBUILD_opencv_python=OFF -DBUILD_opencv_java=OFF ../../../sources"
%COMSPEC% /S /C "CALL "%msvc_path%\VC\bin\amd64\vcvars64.bat" && nmake.exe opencv_video"
CD ..\..\..

MKDIR ..\..\opencv.new\build\x86\vc%VSVER%cli\lib 2>nul
MKDIR ..\..\opencv.new\build\x86\vc%VSVER%cli\bin 2>nul
COPY /Y build\x86\vc%VSVER%cli\3rdparty\lib\zlibd.lib ..\..\opencv.new\build\x86\vc%VSVER%cli\lib
COPY /Y build\x86\vc%VSVER%cli\3rdparty\lib\zlib.lib ..\..\opencv.new\build\x86\vc%VSVER%cli\lib
COPY /Y build\x86\vc%VSVER%cli\lib\*.lib ..\..\opencv.new\build\x86\vc%VSVER%cli\lib
COPY /Y build\x86\vc%VSVER%cli\lib\*.pdb ..\..\opencv.new\build\x86\vc%VSVER%cli\lib
COPY /Y build\x86\vc%VSVER%cli\bin\*.dll ..\..\opencv.new\build\x86\vc%VSVER%cli\bin
COPY /Y build\x86\vc%VSVER%cli\bin\*.pdb ..\..\opencv.new\build\x86\vc%VSVER%cli\bin

MKDIR ..\..\opencv.new\build\x64\vc%VSVER%cli\lib 2>nul
MKDIR ..\..\opencv.new\build\x64\vc%VSVER%cli\bin 2>nul
COPY /Y build\x64\vc%VSVER%cli\3rdparty\lib\zlibd.lib ..\..\opencv.new\build\x64\vc%VSVER%cli\lib
COPY /Y build\x64\vc%VSVER%cli\3rdparty\lib\zlib.lib ..\..\opencv.new\build\x64\vc%VSVER%cli\lib
COPY /Y build\x64\vc%VSVER%cli\lib\*.lib ..\..\opencv.new\build\x64\vc%VSVER%cli\lib
COPY /Y build\x64\vc%VSVER%cli\lib\*.pdb ..\..\opencv.new\build\x64\vc%VSVER%cli\lib
COPY /Y build\x64\vc%VSVER%cli\bin\*.dll ..\..\opencv.new\build\x64\vc%VSVER%cli\bin
COPY /Y build\x64\vc%VSVER%cli\bin\*.pdb ..\..\opencv.new\build\x64\vc%VSVER%cli\bin

MKDIR ..\..\opencv.new\build\ARM\vc%VSVER%cli\lib 2>nul
MKDIR ..\..\opencv.new\build\ARM\vc%VSVER%cli\bin 2>nul
COPY /Y build\ARM\vc%VSVER%cli\3rdparty\lib\zlibd.lib ..\..\opencv.new\build\ARM\vc%VSVER%cli\lib
COPY /Y build\ARM\vc%VSVER%cli\3rdparty\lib\zlib.lib ..\..\opencv.new\build\ARM\vc%VSVER%cli\lib
COPY /Y build\ARM\vc%VSVER%cli\lib\*.lib ..\..\opencv.new\build\ARM\vc%VSVER%cli\lib
COPY /Y build\ARM\vc%VSVER%cli\lib\*.pdb ..\..\opencv.new\build\ARM\vc%VSVER%cli\lib
COPY /Y build\ARM\vc%VSVER%cli\bin\*.dll ..\..\opencv.new\build\ARM\vc%VSVER%cli\bin
COPY /Y build\ARM\vc%VSVER%cli\bin\*.pdb ..\..\opencv.new\build\ARM\vc%VSVER%cli\bin

MKDIR ..\..\opencv.new\build\x86\vc%VSVER%rt\lib 2>nul
MKDIR ..\..\opencv.new\build\x86\vc%VSVER%rt\bin 2>nul
COPY /Y build\x86\vc%VSVER%rt\3rdparty\lib\zlibd.lib ..\..\opencv.new\build\x86\vc%VSVER%rt\lib
COPY /Y build\x86\vc%VSVER%rt\3rdparty\lib\zlib.lib ..\..\opencv.new\build\x86\vc%VSVER%rt\lib
COPY /Y build\x86\vc%VSVER%rt\lib\*.lib ..\..\opencv.new\build\x86\vc%VSVER%rt\lib
COPY /Y build\x86\vc%VSVER%rt\lib\*.pdb ..\..\opencv.new\build\x86\vc%VSVER%rt\lib
COPY /Y build\x86\vc%VSVER%rt\bin\*.dll ..\..\opencv.new\build\x86\vc%VSVER%rt\bin
COPY /Y build\x86\vc%VSVER%rt\bin\*.pdb ..\..\opencv.new\build\x86\vc%VSVER%rt\bin

MKDIR ..\..\opencv.new\build\x64\vc%VSVER%rt\lib 2>nul
MKDIR ..\..\opencv.new\build\x64\vc%VSVER%rt\bin 2>nul
COPY /Y build\x64\vc%VSVER%rt\3rdparty\lib\zlibd.lib ..\..\opencv.new\build\x64\vc%VSVER%rt\lib
COPY /Y build\x64\vc%VSVER%rt\3rdparty\lib\zlib.lib ..\..\opencv.new\build\x64\vc%VSVER%rt\lib
COPY /Y build\x64\vc%VSVER%rt\lib\*.lib ..\..\opencv.new\build\x64\vc%VSVER%rt\lib
COPY /Y build\x64\vc%VSVER%rt\lib\*.pdb ..\..\opencv.new\build\x64\vc%VSVER%rt\lib
COPY /Y build\x64\vc%VSVER%rt\bin\*.dll ..\..\opencv.new\build\x64\vc%VSVER%rt\bin
COPY /Y build\x64\vc%VSVER%rt\bin\*.pdb ..\..\opencv.new\build\x64\vc%VSVER%rt\bin

MKDIR ..\..\opencv.new\build\ARM\vc%VSVER%rt\lib 2>nul
MKDIR ..\..\opencv.new\build\ARM\vc%VSVER%rt\bin 2>nul
COPY /Y build\ARM\vc%VSVER%rt\3rdparty\lib\zlibd.lib ..\..\opencv.new\build\ARM\vc%VSVER%rt\lib
COPY /Y build\ARM\vc%VSVER%rt\3rdparty\lib\zlib.lib ..\..\opencv.new\build\ARM\vc%VSVER%rt\lib
COPY /Y build\ARM\vc%VSVER%rt\lib\*.lib ..\..\opencv.new\build\ARM\vc%VSVER%rt\lib
COPY /Y build\ARM\vc%VSVER%rt\lib\*.pdb ..\..\opencv.new\build\ARM\vc%VSVER%rt\lib
COPY /Y build\ARM\vc%VSVER%rt\bin\*.dll ..\..\opencv.new\build\ARM\vc%VSVER%rt\bin
COPY /Y build\ARM\vc%VSVER%rt\bin\*.pdb ..\..\opencv.new\build\ARM\vc%VSVER%rt\bin

MKDIR ..\..\opencv.new\build\x86\vc%VSVER%st\lib 2>nul
MKDIR ..\..\opencv.new\build\x86\vc%VSVER%st\bin 2>nul
COPY /Y build\x86\vc%VSVER%st\3rdparty\lib\zlibd.lib ..\..\opencv.new\build\x86\vc%VSVER%st\lib
COPY /Y build\x86\vc%VSVER%st\3rdparty\lib\zlib.lib ..\..\opencv.new\build\x86\vc%VSVER%st\lib
COPY /Y build\x86\vc%VSVER%st\lib\*.lib ..\..\opencv.new\build\x86\vc%VSVER%st\lib
COPY /Y build\x86\vc%VSVER%st\lib\*.pdb ..\..\opencv.new\build\x86\vc%VSVER%st\lib
COPY /Y build\x86\vc%VSVER%st\bin\*.dll ..\..\opencv.new\build\x86\vc%VSVER%st\bin
COPY /Y build\x86\vc%VSVER%st\bin\*.pdb ..\..\opencv.new\build\x86\vc%VSVER%st\bin

MKDIR ..\..\opencv.new\build\x64\vc%VSVER%st\lib 2>nul
MKDIR ..\..\opencv.new\build\x64\vc%VSVER%st\bin 2>nul
COPY /Y build\x64\vc%VSVER%st\3rdparty\lib\zlibd.lib ..\..\opencv.new\build\x64\vc%VSVER%st\lib
COPY /Y build\x64\vc%VSVER%st\3rdparty\lib\zlib.lib ..\..\opencv.new\build\x64\vc%VSVER%st\lib
COPY /Y build\x64\vc%VSVER%st\lib\*.lib ..\..\opencv.new\build\x64\vc%VSVER%st\lib
COPY /Y build\x64\vc%VSVER%st\lib\*.pdb ..\..\opencv.new\build\x64\vc%VSVER%st\lib
COPY /Y build\x64\vc%VSVER%st\bin\*.dll ..\..\opencv.new\build\x64\vc%VSVER%st\bin
COPY /Y build\x64\vc%VSVER%st\bin\*.pdb ..\..\opencv.new\build\x64\vc%VSVER%st\bin

MKDIR ..\..\opencv.new\build\ARM\vc%VSVER%st\lib 2>nul
MKDIR ..\..\opencv.new\build\ARM\vc%VSVER%st\bin 2>nul
COPY /Y build\ARM\vc%VSVER%st\3rdparty\lib\zlibd.lib ..\..\opencv.new\build\ARM\vc%VSVER%st\lib
COPY /Y build\ARM\vc%VSVER%st\3rdparty\lib\zlib.lib ..\..\opencv.new\build\ARM\vc%VSVER%st\lib
COPY /Y build\ARM\vc%VSVER%st\lib\*.lib ..\..\opencv.new\build\ARM\vc%VSVER%st\lib
COPY /Y build\ARM\vc%VSVER%st\lib\*.pdb ..\..\opencv.new\build\ARM\vc%VSVER%st\lib
COPY /Y build\ARM\vc%VSVER%st\bin\*.dll ..\..\opencv.new\build\ARM\vc%VSVER%st\bin
COPY /Y build\ARM\vc%VSVER%st\bin\*.pdb ..\..\opencv.new\build\ARM\vc%VSVER%st\bin
SET VSVER=
SET OSVER=