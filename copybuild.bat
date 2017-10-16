RMDIR /S /Q ..\..\opencv.new
XCOPY /E /Y build\include ..\..\opencv.new\build\include\
%comspec% /c cmake_winrt.cmd 11
%comspec% /c cmake_winrt.cmd 12
%comspec% /c cmake_winrt.cmd 14