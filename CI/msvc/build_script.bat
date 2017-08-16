cd %APPVEYOR_BUILD_FOLDER%
cd build_%VCMI_BUILD_PLATFORM%

cmake --build . --config %VCMI_BUILD_CONFIGURATION% -- /maxcpucount:2

cpack
