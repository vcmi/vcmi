curl -LfsS -o "vcpkg-export-${VCMI_BUILD_PLATFORM}-windows-v143.7z" \
	"https://github.com/vcmi/vcmi-deps-windows/releases/download/v1.6/vcpkg-export-${VCMI_BUILD_PLATFORM}-windows-v143.7z"
7z x "vcpkg-export-${VCMI_BUILD_PLATFORM}-windows-v143.7z"

rm -r -f vcpkg/installed/${VCMI_BUILD_PLATFORM}-windows/debug
mkdir -p vcpkg/installed/${VCMI_BUILD_PLATFORM}-windows/debug/bin
cp vcpkg/installed/${VCMI_BUILD_PLATFORM}-windows/bin/* vcpkg/installed/${VCMI_BUILD_PLATFORM}-windows/debug/bin

DUMPBIN_DIR=$(vswhere -latest -find **/dumpbin.exe | head -n 1)
dirname "$DUMPBIN_DIR" > $GITHUB_PATH
