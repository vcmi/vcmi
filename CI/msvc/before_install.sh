curl -LfsS -o "vcpkg-export-${VCMI_BUILD_PLATFORM}-windows-v140.7z" \
	"https://github.com/vcmi/vcmi-deps-windows/releases/download/v1.4/vcpkg-export-${VCMI_BUILD_PLATFORM}-windows-v140.7z"
7z x "vcpkg-export-${VCMI_BUILD_PLATFORM}-windows-v140.7z"

rm -r -f vcpkg/installed/${VCMI_BUILD_PLATFORM}-windows/debug
mkdir -p vcpkg/installed/${VCMI_BUILD_PLATFORM}-windows/debug/bin
cp vcpkg/installed/${VCMI_BUILD_PLATFORM}-windows/bin/* vcpkg/installed/${VCMI_BUILD_PLATFORM}-windows/debug/bin
