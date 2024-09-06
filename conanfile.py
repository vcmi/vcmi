from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.apple import is_apple_os
from conan.tools.build import cross_building
from conan.tools.cmake import CMakeDeps, CMakeToolchain
from conans import tools

required_conan_version = ">=1.51.3"

class VCMI(ConanFile):
    settings = "os", "compiler", "build_type", "arch"

    _libRequires = [
        "boost/[^1.69]",
        "minizip/[~1.2.12]",
    ]
    _clientRequires = [
        # Versions between 2.5-2.8 have broken loading of palette sdl images which a lot of mods use
        # there is workaround that require disabling cmake flag which is not available in conan recipes. 
        # Bug is fixed in version 2.8, however it is not available in conan at the moment
        "sdl_image/2.0.5", 
        "sdl_ttf/[>=2.0.18]",
        "onetbb/[^2021.7 <2021.10]",  # 2021.10+ breaks mobile builds due to added hwloc dependency
        "xz_utils/[>=5.2.5]", # Required for innoextract
    ]

    requires = _libRequires + _clientRequires

    options = {
        "default_options_of_requirements": [True, False],
        "with_apple_system_libs": [True, False],
        "with_ffmpeg": [True, False],
        "with_luajit": [True, False],
    }
    default_options = {
        "default_options_of_requirements": False,
        "with_apple_system_libs": False,
        "with_ffmpeg": True,
        "with_luajit": False,

        "boost/*:shared": True,
        "minizip/*:shared": True,
    }

    def configure(self):
        self.options["ffmpeg"].shared = self.settings.os == "Android" # using shared version results in less total project size on Android
        self.options["freetype"].shared = self.settings.os == "Android"

        # SDL_image and Qt depend on it, in iOS both are static
        self.options["libpng"].shared = not self.settings.os != "iOS"
        # static Qt for iOS is the only viable option at the moment
        self.options["qt"].shared = self.settings.os != "iOS"

        # TODO: enable for all platforms
        if self.settings.os == "Android":
            self.options["bzip2"].shared = True
            self.options["libiconv"].shared = True
            self.options["zlib"].shared = True

        # TODO: enable for all platforms?
        if self.settings.os == "Windows":
            self.options["sdl"].shared = True
            self.options["sdl_image"].shared = True
            self.options["sdl_mixer"].shared = True
            self.options["sdl_ttf"].shared = True

        if self.settings.os == "iOS": 
            # TODO: ios - newer sdl fails to link
            self.requires("sdl/2.26.1")
            self.requires("sdl_mixer/2.0.4")
        elif self.settings.os == "Android":
            # On Android SDL version must be same as version of Java wrapper for SDL in VCMI source code
            self.requires("sdl/2.26.5")
            self.requires("sdl_mixer/2.0.4")
        else:
            # upcoming SDL version 3.0+ is not supported at the moment due to API breakage
            # SDL versions between 2.22-2.26.1 have broken sound
            self.requires("sdl/[^2.26 || >=2.0.20 <=2.22.0]")
            self.requires("sdl_mixer/[>=2.0.4]")

        if self.settings.os == "Android":
            self.options["qt"].android_sdk = tools.get_env("ANDROID_HOME", default="")

        if self.options.default_options_of_requirements:
            return

        # we need only the following Boost parts:
        # date_time filesystem iostreams locale program_options system thread
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
        self.options["ffmpeg"].with_bzip2 = False
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
        self.options["ffmpeg"].with_opus = False
        self.options["ffmpeg"].with_programs = False
        self.options["ffmpeg"].with_sdl = False
        self.options["ffmpeg"].with_ssl = False
        self.options["ffmpeg"].with_vorbis = False
        self.options["ffmpeg"].with_zlib = False
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
        self.options["ffmpeg"].enable_demuxers = "bink,binka,ogg,smacker,webm_dash_manifest"
        self.options["ffmpeg"].enable_parsers = "opus,vorbis,vp8,vp9,webp"
        self.options["ffmpeg"].enable_decoders = "bink,binkaudio_dct,binkaudio_rdft,smackaud,smacker,theora,vorbis,vp8,vp9,opus"

        #optionally, for testing - enable ffplay/ffprobe binaries in conan package:
        #if self.settings.os == "Windows":
        #    self.options["ffmpeg"].with_programs = True
        #    self.options["ffmpeg"].avfilter = True
        #    self.options["ffmpeg"].with_sdl = True
        #    self.options["ffmpeg"].enable_filters = "aresample,scale"

        self.options["sdl"].sdl2main = self.settings.os != "iOS"
        self.options["sdl"].vulkan = False

        # bmp, png are the only ones that needs to be supported
        # dds support may be useful for HD edition, but not supported by sdl_image at the moment
        self.options["sdl_image"].gif = False
        self.options["sdl_image"].lbm = False
        self.options["sdl_image"].pnm = False
        self.options["sdl_image"].pcx = False
        #self.options["sdl_image"].qoi = False # sdl_image >=2.6
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

        # mp3, ogg and wav are the only ones that needs to be supported
        # opus is nice to have, but fails to build in CI
        # flac can be considered, but generally unnecessary
        self.options["sdl_mixer"].flac = False
        self.options["sdl_mixer"].modplug = False
        self.options["sdl_mixer"].opus = False
        if self.settings.os == "iOS" or self.settings.os == "Android":
            # only available in older sdl_mixer version, removed in newer version
            self.options["sdl_mixer"].mad = False
            self.options["sdl_mixer"].mikmod = False
            self.options["sdl_mixer"].nativemidi = False

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

                # we need only macdeployqt
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
        self.options["qt"].qttools = True
        self.options["qt"].qtandroidextras = self.settings.os == "Android" # TODO: in Qt 6 it's part of Core
        self.options["qt"].with_freetype = self.settings.os == "Android"
        self.options["qt"].with_libjpeg = False
        self.options["qt"].with_md4c = False
        self.options["qt"].with_mysql = False
        self.options["qt"].with_odbc = False
        self.options["qt"].with_openal = False
        self.options["qt"].with_pq = False
        self.options["qt"].openssl = not is_apple_os(self)
        if self.settings.os == "iOS" or self.settings.os == "Android":
            self.options["qt"].opengl = "es2"
        if not is_apple_os(self) and self.settings.os != "Android" and cross_building(self):
            self.options["qt"].cross_compile = self.env["CONAN_CROSS_COMPILE"]
        # TODO: add for all platforms after updating recipe
        if self.settings.os == "Android":
            self.options["qt"].essential_modules = False
        # No Qt OpenGL for cross-compiling for Windows, Conan does not support it
        if self.settings.os == "Windows" and cross_building(self):
            self.options["qt"].opengl = "no"

        # transitive deps
        # doesn't link to overridden bzip2 & zlib, the tool isn't needed anyway
        self.options["pcre2"].build_pcre2grep = False
        # executable not needed
        if self.settings.os == "Android":
            self.options["sqlite3"].build_executable = False

    def requirements(self):
        self.requires("freetype/[~2.12.1]", override=True) # sdl_ttf / Qt
        self.requires("libpng/[~1.6.39]", override=True) # freetype / qt / sdl_image

        # client
        if self.options.with_ffmpeg:
            self.requires("ffmpeg/[>=4.4]")

        # launcher
        if self.settings.os == "Android":
            self.requires("qt/[~5.15.14]")
        else:
            self.requires("qt/[~5.15.2]")
        # TODO: version range doesn't work in Conan v1
        if self.options["qt"].openssl:
            self.requires("openssl/1.1.1s")

        # use Apple system libraries instead of external ones
        if self.options.with_apple_system_libs and is_apple_os(self):
            systemLibsOverrides = [
                "bzip2/1.0.8",
                "libiconv/1.17",
                "sqlite3/3.39.2",
                "zlib/1.2.12",
            ]
            for lib in systemLibsOverrides:
                self.requires(f"{lib}@vcmi/apple", override=True)
        elif self.settings.os == "Android":
            self.requires("zlib/1.2.12@vcmi/android", override=True)
        else:
            self.requires("zlib/[~1.2.13]", override=True) # minizip / Qt
            self.requires("libiconv/[~1.17]", override=True) # ffmpeg / sdl

        # TODO: the latest official release of LuaJIT (which is quite old) can't be built for arm
        if self.options.with_luajit and not str(self.settings.arch).startswith("arm"):
            self.requires("luajit/[~2.0.5]")

    def validate(self):
        if self.options.with_apple_system_libs and not is_apple_os(self):
            raise ConanInvalidConfiguration("with_apple_system_libs is only for Apple platforms")
        if self.options.with_apple_system_libs and self.options.default_options_of_requirements:
            raise ConanInvalidConfiguration("with_apple_system_libs and default_options_of_requirements can't be True at the same time")

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["USING_CONAN"] = True
        tc.variables["CONAN_INSTALL_FOLDER"] = self.install_folder
        if self.settings.os == "Android":
            tc.variables["CMAKE_ANDROID_API"] = str(self.settings.os.api_level)
            tc.variables["ANDROID_SYSROOT_LIB_SUBDIR"] = {
                'armv7': 'arm-linux-androideabi',
                'armv8': 'aarch64-linux-android',
                'x86': 'i686-linux-android',
                'x86_64': 'x86_64-linux-android',
            }.get(str(self.settings.arch))
        if cross_building(self) and self.settings.os == "Windows":
            tc.variables["CONAN_SYSTEM_LIBRARY_LOCATION"] = self.env["CONAN_SYSTEM_LIBRARY_LOCATION"]
        tc.generate()

        deps = CMakeDeps(self)
        if tools.get_env("GENERATE_ONLY_BUILT_CONFIG", default=False):
            deps.generate()
            return

        # allow using prebuilt deps with all configs
        # credits to https://github.com/conan-io/conan/issues/11607#issuecomment-1188500937 for the workaround
        configs = [
            "Debug",
            "MinSizeRel",
            "Release",
            "RelWithDebInfo",
        ]
        for config in configs:
            print(f"generating CMakeDeps for {config}")
            deps.configuration = config
            deps.generate()

    def imports(self):
        if is_apple_os(self):
            self.copy("*.dylib", "Frameworks", "lib")
        elif self.settings.os == "Windows":
            self.copy("*.dll", src="bin/archdatadir/plugins/platforms", dst="platforms")
            self.copy("*.dll", src="bin/archdatadir/plugins/styles", dst="styles")
            self.copy("*.dll", src="@bindirs", dst="", excludes="archdatadir/*")
        elif self.settings.os == "Android":
            self.copy("*.so", ".", "lib")
