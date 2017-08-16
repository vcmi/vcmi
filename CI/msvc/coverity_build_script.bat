cd %APPVEYOR_BUILD_FOLDER%
cd build_%VCMI_BUILD_PLATFORM%

echo Building with coverity...
cov-build.exe --dir cov-int cmake --build . --config %VCMI_BUILD_CONFIGURATION% -- /maxcpucount:2
