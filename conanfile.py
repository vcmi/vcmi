from conan import ConanFile
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
        "boost/[^1.69]",
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
        "with_ffmpeg": [True, False],
    }
    default_options = {
        "with_ffmpeg": True,

        "boost/*:shared": True,
        "bzip2/*:shared": True,
        "libiconv/*:shared": True,
        "libpng/*:shared": True,
        "minizip/*:shared": True,
        "ogg/*:shared": True,
        "opus/*:shared": True,
        "xz_utils/*:shared": True,
        "zlib/*:shared": True,
    }

    def configure(self):
        self.options["ffmpeg"].shared = is_msvc(self) # MSVC static build requires static runtime, but VCMI uses dynamic runtime
        # self.options["freetype"].shared = self.settings.os == "Android" # TODO https://github.com/conan-io/conan-center-index/issues/26020

        # static Qt for iOS is the only viable option at the moment
        self.options["qt"].shared = self.settings.os != "iOS"

        if self.settings.os == "Android":
            self.options["qt"].android_sdk = os.getenv("ANDROID_HOME") # , default=""

        # we need only the following Boost parts:
        # date_time filesystem iostreams locale program_options system
        # some other parts are also enabled because they're dependents
        # see e.g. conan-center-index/recipes/boost/all/dependencies
        self.options["boost"].without_context = True
        self.options["boost"].without_contract = True
        self.options["boost"].without_coroutine = True
        self.options["boost"].without_fiber = True
        self.options["boost"].without_graph = True
        self.options["boost"].without_graph_parallel = True
        self.options["boost"].without_json = True
        self.options["boost"].without_log = True
        self.options["boost"].without_math = True
        self.options["boost"].without_mpi = True
        self.options["boost"].without_nowide = True
        self.options["boost"].without_process = True
        self.options["boost"].without_python = True
        self.options["boost"].without_serialization = True
        self.options["boost"].without_stacktrace = True
        self.options["boost"].without_test = True
        self.options["boost"].without_timer = True
        self.options["boost"].without_type_erasure = True
        self.options["boost"].without_wave = True
        self.options["boost"].without_url = True

        self.options["ffmpeg"].disable_all_bitstream_filters = True
        self.options["ffmpeg"].disable_all_decoders = True
        self.options["ffmpeg"].disable_all_demuxers = True
        self.options["ffmpeg"].disable_all_encoders = True
        self.options["ffmpeg"].disable_all_filters = True
        self.options["ffmpeg"].disable_all_hardware_accelerators = True
        self.options["ffmpeg"].disable_all_muxers = True
        self.options["ffmpeg"].disable_all_parsers = True
        self.options["ffmpeg"].disable_all_protocols = True

        self.options["ffmpeg"].with_asm = False
        self.options["ffmpeg"].with_freetype = False
        self.options["ffmpeg"].with_libaom = False
        self.options["ffmpeg"].with_libdav1d = False
        self.options["ffmpeg"].with_libiconv = False
        self.options["ffmpeg"].with_libmp3lame = False
        self.options["ffmpeg"].with_libsvtav1 = False
        self.options["ffmpeg"].with_libvpx = False
        self.options["ffmpeg"].with_libwebp = False
        self.options["ffmpeg"].with_libx264 = False
        self.options["ffmpeg"].with_libx265 = False
        self.options["ffmpeg"].with_lzma = True
        self.options["ffmpeg"].with_openh264 = False
        self.options["ffmpeg"].with_openjpeg = False
        self.options["ffmpeg"].with_programs = False
        self.options["ffmpeg"].with_sdl = False
        self.options["ffmpeg"].with_ssl = False
        self.options["ffmpeg"].with_vorbis = False
        # option not available on Android
        if self.settings.os != "Android":
            self.options["ffmpeg"].with_libfdk_aac = False

        self.options["ffmpeg"].avcodec = True
        self.options["ffmpeg"].avdevice = False
        self.options["ffmpeg"].avfilter = False
        self.options["ffmpeg"].avformat = True
        self.options["ffmpeg"].postproc = False
        self.options["ffmpeg"].swresample = True # For resampling of audio in 'planar' formats
        self.options["ffmpeg"].swscale = True    # For video scaling

        # We want following options supported:
        # H3:SoD - .bik and .smk
        # H3:HD  -  ogg container / theora video / vorbis sound (not supported by vcmi at the moment, but might be supported in future)
        # and for mods - webm container / vp8 or vp9 video / opus sound
        # TODO: add av1 support for mods (requires enabling libdav1d which currently fails to build via Conan)
        self.options["ffmpeg"].enable_protocols = "file"
        self.options["ffmpeg"].enable_demuxers = ",".join([
            "bink",
            "binka",
            "ogg",
            "smacker",
            "webm_dash_manifest",
        ])
        self.options["ffmpeg"].enable_parsers = ",".join([
            "opus",
            "vorbis",
            "vp8",
            "vp9",
            "webp",
        ])
        self.options["ffmpeg"].enable_decoders = ",".join([
            "bink",
            "binkaudio_dct",
            "binkaudio_rdft",
            "opus",
            "smackaud",
            "smacker",
            "theora",
            "vorbis",
            "vp8",
            "vp9",
        ])

        #optionally, for testing - enable ffplay/ffprobe binaries in conan package:
        #if self.settings.os == "Windows":
        #    self.options["ffmpeg"].with_programs = True
        #    self.options["ffmpeg"].avfilter = True
        #    self.options["ffmpeg"].with_sdl = True
        #    self.options["ffmpeg"].enable_filters = "aresample,scale"

        self.options["onetbb"].tbbbind = False
        self.options["onetbb"].tbbmalloc = False
        self.options["onetbb"].tbbproxy = False

        self.options["sdl"].iconv = True
        self.options["sdl"].sdl2main = self.settings.os != "iOS"
        self.options["sdl"].vulkan = False

        # bmp, png are the only ones that needs to be supported
        # dds support may be useful for HD edition, but not supported by sdl_image at the moment
        self.options["sdl_image"].gif = False
        self.options["sdl_image"].lbm = False
        self.options["sdl_image"].pnm = False
        self.options["sdl_image"].pcx = False
        self.options["sdl_image"].qoi = False
        self.options["sdl_image"].svg = False
        self.options["sdl_image"].tga = False
        self.options["sdl_image"].with_libjpeg = False
        self.options["sdl_image"].with_libtiff = False
        self.options["sdl_image"].with_libwebp = False
        self.options["sdl_image"].xcf = False
        self.options["sdl_image"].xpm = False
        self.options["sdl_image"].xv = False
        if is_apple_os(self):
            self.options["sdl_image"].imageio = True

        # mp3, ogg and wav are the only ones that needs to be supported, flac is a bonus
        self.options["sdl_mixer"].mad = False
        self.options["sdl_mixer"].mikmod = False
        self.options["sdl_mixer"].modplug = False
        self.options["sdl_mixer"].nativemidi = False
        self.options["sdl_mixer"].tinymidi = False

        # static on "single app" platforms
        isSdlShared = self.settings.os != "iOS" and self.settings.os != "Android"
        self.options["sdl"].shared = isSdlShared
        self.options["sdl_image"].shared = isSdlShared
        self.options["sdl_mixer"].shared = isSdlShared
        self.options["sdl_ttf"].shared = isSdlShared

        def _disableQtOptions(disableFlag, options):
            return " ".join([f"-{disableFlag}-{tool}" for tool in options])

        _qtOptions = [
            _disableQtOptions("no", [
                "gif",
                "ico",
            ]),
            _disableQtOptions("no-feature", [
                # xpm format is required for Drag'n'Drop support
                "imageformat_bmp",
                "imageformat_jpeg",
                "imageformat_ppm",
                "imageformat_xbm",

                # we need only win/macdeployqt
                # TODO: disabling these doesn't disable generation of CMake targets
                # TODO: in Qt 6.3 it's a part of qtbase
                # "assistant",
                # "designer",
                # "distancefieldgenerator",
                # "kmap2qmap",
                # "linguist",
                # "makeqpf",
                # "pixeltool",
                # "qdbus",
                # "qev",
                # "qtattributionsscanner",
                # "qtdiag",
                # "qtpaths",
                # "qtplugininfo",
            ]),
        ]
        self.options["qt"].config = " ".join(_qtOptions)
        self.options["qt"].essential_modules = False
        self.options["qt"].qttools = True
        self.options["qt"].qtandroidextras = self.settings.os == "Android" # TODO: in Qt 6 it's part of Core
        self.options["qt"].with_freetype = self.settings.os == "Android"
        self.options["qt"].with_libjpeg = False
        self.options["qt"].with_mysql = False
        self.options["qt"].with_odbc = False
        self.options["qt"].with_openal = False
        self.options["qt"].with_pq = False
        self.options["qt"].openssl = not is_apple_os(self)
        if self.settings.os == "iOS" or self.settings.os == "Android":
            self.options["qt"].opengl = "es2"

        # transitive deps
        # doesn't link to overridden bzip2 & zlib, the tool isn't needed anyway
        self.options["pcre2"].build_pcre2grep = False
        # executable not needed
        self.options["sqlite3"].build_executable = False
        # prevents pulling openssl in and isn't needed anyway
        self.options["opusfile"].http = False
        # programs not needed
        self.options["zstd"].build_programs = False

    def requirements(self):
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
