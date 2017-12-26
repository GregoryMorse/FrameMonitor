REM start /b /wait E:\downloads\programming\opencv-3.4.0-vc14_vc15.exe -o"%userprofile%\Documents\My Libraries\opencv340" -y
REM "%programfiles%\7-Zip\7z.exe" x -o"%userprofile%\Documents\My Libraries" E:\downloads\programming\opencv_contrib-3.4.0.zip
%comspec% /c cmakevio_winrt.cmd 11
%comspec% /c cmakevio_winrt.cmd 12
%comspec% /c cmakevio_winrt.cmd 14
%comspec% /c cmakevio_winrt.cmd 15 x64 st OFF
%comspec% /c cmakevio_winrt.cmd 15 x86 st OFF
%comspec% /c cmakevio_winrt.cmd 15 x64 st ON
%comspec% /c cmakevio_winrt.cmd 15 x86 st ON