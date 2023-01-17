from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.apple import is_apple_os
from conan.tools.cmake import CMakeDeps, CMakeToolchain
from conans import tools

required_conan_version = ">=1.51.3"

class VCMI(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    requires = [
        "boost/[^1.69]",
        "minizip/[~1.2.12]",
        "onetbb/[^2021.3]", # Nullkiller AI
        "qt/[~5.15.2]", # launcher
        "sdl/[~2.26.1 || >=2.0.20 <=2.22.0]", # versions in between have broken sound
        "sdl_image/[~2.0.5]",
        "sdl_mixer/[~2.0.4]",
        "sdl_ttf/[~2.0.18]",
    ]
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
        "onetbb/*:shared": True,
    }

    def configure(self):
        # SDL_image and Qt depend on it, in iOS both are static
        self.options["libpng"].shared = self.settings.os != "iOS"
        # static Qt for iOS is the only viable option at the moment
        self.options["qt"].shared = self.settings.os != "iOS"

        if self.options.default_options_of_requirements:
            return

        # we need only the following Boost parts:
        # date_time filesystem locale program_options system thread
        # some other parts are also enabled because they're dependents
        # see e.g. conan-center-index/recipes/boost/all/dependencies
        self.options["boost"].without_context = True
        self.options["boost"].without_contract = True
        self.options["boost"].without_coroutine = True
        self.options["boost"].without_fiber = True
        self.options["boost"].without_graph = True
        self.options["boost"].without_graph_parallel = True
        self.options["boost"].without_iostreams = True
        self.options["boost"].without_json = True
        self.options["boost"].without_log = True
        self.options["boost"].without_math = True
        self.options["boost"].without_mpi = True
        self.options["boost"].without_nowide = True
        self.options["boost"].without_python = True
        self.options["boost"].without_random = True
        self.options["boost"].without_regex = True
        self.options["boost"].without_serialization = True
        self.options["boost"].without_stacktrace = True
        self.options["boost"].without_test = True
        self.options["boost"].without_timer = True
        self.options["boost"].without_type_erasure = True
        self.options["boost"].without_wave = True

        self.options["ffmpeg"].avdevice = False
        self.options["ffmpeg"].avfilter = False
        self.options["ffmpeg"].postproc = False
        self.options["ffmpeg"].swresample = False
        self.options["ffmpeg"].with_freetype = False
        self.options["ffmpeg"].with_libfdk_aac = False
        self.options["ffmpeg"].with_libmp3lame = False
        self.options["ffmpeg"].with_libvpx = False
        self.options["ffmpeg"].with_libwebp = False
        self.options["ffmpeg"].with_libx264 = False
        self.options["ffmpeg"].with_libx265 = False
        self.options["ffmpeg"].with_openh264 = False
        self.options["ffmpeg"].with_openjpeg = False
        self.options["ffmpeg"].with_opus = False
        self.options["ffmpeg"].with_programs = False
        self.options["ffmpeg"].with_ssl = False
        self.options["ffmpeg"].with_vorbis = False

        self.options["sdl"].sdl2main = self.settings.os != "iOS"
        self.options["sdl"].vulkan = False

        self.options["sdl_image"].lbm = False
        self.options["sdl_image"].pnm = False
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

        self.options["sdl_mixer"].flac = False
        self.options["sdl_mixer"].mad = False
        self.options["sdl_mixer"].mikmod = False
        self.options["sdl_mixer"].modplug = False
        self.options["sdl_mixer"].nativemidi = False
        self.options["sdl_mixer"].opus = False
        self.options["sdl_mixer"].wav = False

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
        self.options["qt"].with_freetype = False
        self.options["qt"].with_libjpeg = False
        self.options["qt"].with_md4c = False
        self.options["qt"].with_mysql = False
        self.options["qt"].with_odbc = False
        self.options["qt"].with_openal = False
        self.options["qt"].with_pq = False
        self.options["qt"].openssl = not is_apple_os(self)
        if self.settings.os == "iOS":
            self.options["qt"].opengl = "es2"

        # transitive deps
        # doesn't link to overridden bzip2 & zlib, the tool isn't needed anyway
        self.options["pcre2"].build_pcre2grep = False

    def requirements(self):
        # TODO: will no longer be needed after merging https://github.com/conan-io/conan-center-index/pull/13399
        self.requires("libpng/[~1.6.38]", override=True) # freetype / Qt
        if self.options.default_options_of_requirements:
            self.requires("libjpeg/9e", override=True) # libtiff / Qt
            self.requires("freetype/[~2.12.1]", override=True) # sdl_ttf / Qt
            if self.options.with_ffmpeg:
                self.requires("libwebp/[~1.2.4]", override=True) # sdl_image / ffmpeg

        if self.options.with_ffmpeg:
            self.requires("ffmpeg/[^4.4]")

        # use Apple system libraries instead of external ones
        if self.options.with_apple_system_libs and not self.options.default_options_of_requirements and is_apple_os(self):
            systemLibsOverrides = [
                "bzip2/1.0.8",
                "libiconv/1.17",
                "sqlite3/3.39.2",
                "zlib/1.2.12",
            ]
            for lib in systemLibsOverrides:
                self.requires(f"{lib}@vcmi/apple", override=True)

        # TODO: the latest official release of LuaJIT (which is quite old) can't be built for arm
        if self.options.with_luajit and not str(self.settings.arch).startswith("arm"):
            self.requires("luajit/[~2.0.5]")

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["USING_CONAN"] = True
        tc.variables["CONAN_INSTALL_FOLDER"] = self.install_folder
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
       self.copy("*.dylib", "Frameworks", "lib")
