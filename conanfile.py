from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.apple import is_apple_os
from conan.tools.cmake import CMakeToolchain
from conan.tools.files import save
from conan.tools.microsoft import is_msvc

from glob import glob
import os

required_conan_version = ">=2.13.0"

class VCMI(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps"

    _libRequires = [
        "luajit/2.1.0-beta3",
        "minizip/[^1.2.12]",
        "zlib/[^1.2.12]",
    ]
    _clientRequires = [
        "onetbb/[^2021.7]",
        "sdl_image/[^2.8.2]",
        "sdl_mixer/[^2.8.0]",
        "sdl_ttf/[^2.0.18]",
    ]
    _launcherRequires = [
        "xz_utils/[^5.2.5]", # innoextract
    ]
    requires = _libRequires + _clientRequires + _launcherRequires

    options = {
        "target_pre_windows10": [True, False],
        "with_ffmpeg": [True, False],
    }
    default_options = {
        "target_pre_windows10": False,
        "with_ffmpeg": True,
    }

    @property
    def _isMobilePlatform(self):
        return self.settings.os == "iOS" or self.settings.os == "Android"

    def config_options(self):
        # static on "single app" platforms
        isSdlShared = not self._isMobilePlatform
        self.options["sdl"].shared = isSdlShared
        self.options["sdl_image"].shared = isSdlShared
        self.options["sdl_mixer"].shared = isSdlShared
        self.options["sdl_ttf"].shared = isSdlShared

        if self.settings.os == "Android":
            self.options["qt"].android_sdk = os.getenv("ANDROID_HOME")

        if self.settings.os != "Windows":
            del self.options.target_pre_windows10

    def requirements(self):
        # lib
        # boost::filesystem removed support for Windows < 10 in v1.87
        boostMinVersion = "1.69"
        if self.options.get_safe("target_pre_windows10", False):
            self.requires(f"boost/[>={boostMinVersion} <1.87]")
        else:
            self.requires(f"boost/[^{boostMinVersion}]")

        # client
        if self.options.with_ffmpeg:
            self.requires("ffmpeg/[>=4.4]")

        # On Android SDL version must be the same as the version of Java wrapper for SDL in VCMI source code
        # Wrapper can be found in the following directory: android/vcmi-app/src/main/java/org/libsdl/app
        # TODO: try enabling version range once there's no conflict
        # sdl_image & sdl_ttf depend on earlier version
        # ERROR: Version conflict: Conflict between sdl/2.28.5 and sdl/2.28.3 in the graph.
        # Conflict originates from sdl_mixer/2.8.0
        # upcoming SDL version 3.0+ is not supported at the moment due to API breakage
        # SDL versions between 2.22-2.26.1 have broken sound
        # self.requires("sdl/[^2.26.1 || >=2.0.20 <=2.22.0]")
        # versions before 2.30.7 don't build for Android with NDK 27: https://github.com/libsdl-org/SDL/issues/9792
        self.requires("sdl/2.30.9", override=True)

        # launcher
        if self.settings.os == "Android":
            self.requires("qt/[~5.15.14]") # earlier versions have serious bugs
        else:
            self.requires("qt/[~5.15.2]")

    def validate(self):
        # FFmpeg
        if is_msvc(self) and self.options.with_ffmpeg and self.dependencies["ffmpeg"].options.shared != True:
            raise ConanInvalidConfiguration("MSVC FFmpeg static build requires static runtime, but VCMI uses dynamic runtime. You must build FFmpeg as shared.")

        # SDL
        sdl2mainValue = self.settings.os != "iOS"
        if self.dependencies["sdl"].options.sdl2main != sdl2mainValue:
            raise ConanInvalidConfiguration(f"sdl:sdl2main option for {self.settings.os} must be set to {sdl2mainValue}")

        # Qt
        qtDep = self.dependencies["qt"]
        if qtDep.options.qttools != True:
            raise ConanInvalidConfiguration("qt:qttools option must be set to True")
        if self.settings.os == "Android" and qtDep.options.qtandroidextras != True:
            # TODO: in Qt 6 this option doesn't exist
            raise ConanInvalidConfiguration("qt:qtandroidextras option for Android must be set to True")
        if not is_apple_os(self) and qtDep.options.openssl != True:
            raise ConanInvalidConfiguration("qt:openssl option for non-Apple OS must be set to True, otherwise mods can't be downloaded")

    def _pathForCmake(self, path):
        # CMake doesn't like \ in strings
        return path.replace(os.path.sep, os.path.altsep) if os.path.altsep else path

    def _generateRuntimeLibsFile(self):
        # create file with list of libs to copy to the package for distribution
        runtimeLibsFile = self._pathForCmake(os.path.join(self.build_folder, "_runtime_libs.txt"))

        runtimeLibExtension = {
            "Android": "so",
            "iOS":     "dylib",
            "Macos":   "dylib",
            "Windows": "dll",
        }.get(str(self.settings.os))

        runtimeLibs = []
        for _, dep in self.dependencies.host.items():
            # Qt libs are copied using *deployqt
            if dep.ref.name == "qt":
                continue

            runtimeLibDir = ''
            if self.settings.os == "Windows":
                if len(dep.cpp_info.bindirs) > 0:
                    runtimeLibDir = dep.cpp_info.bindir
            elif len(dep.cpp_info.libdirs) > 0:
                runtimeLibDir = dep.cpp_info.libdir
            if len(runtimeLibDir) > 0:
                runtimeLibs += glob(os.path.join(runtimeLibDir, f"*.{runtimeLibExtension}"))
        save(self, runtimeLibsFile, "\n".join(runtimeLibs))

        return runtimeLibsFile

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["USING_CONAN"] = True
        tc.variables["CONAN_RUNTIME_LIBS_FILE"] = self._generateRuntimeLibsFile()
        if self.settings.os == "Android":
            tc.variables["CMAKE_ANDROID_API"] = str(self.settings.os.api_level)
        elif self.settings.os == "Windows":
            tc.variables["CONAN_RUNENV_SCRIPT"] = self._pathForCmake(os.path.join(self.build_folder, "conanrun.bat"))
        tc.generate()
