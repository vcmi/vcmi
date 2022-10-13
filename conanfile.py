from conan import ConanFile
from conan.tools.apple import is_apple_os
from conan.tools.cmake import CMakeDeps, CMakeToolchain
from conans import tools

import os

required_conan_version = ">=1.51.3"

class VCMI(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    requires = [
        "boost/1.80.0",
        "ffmpeg/4.4.3",
        "minizip/1.2.12",
        "onetbb/2021.3.0", # Nullkiller AI
        "qt/5.15.6", # launcher
        "sdl/2.24.1",
        "sdl_image/2.0.5",
        "sdl_mixer/2.0.4",
        "sdl_ttf/2.0.18",
    ]

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

    default_options = {
        # shared libs
        "boost/*:shared": True,
        "minizip/*:shared": True,
        "onetbb/*:shared": True,

        # we need only the following Boost parts:
        # date_time filesystem locale program_options system thread
        # some other parts are also enabled because they're dependents
        # see e.g. conan-center-index/recipes/boost/all/dependencies
        "boost/*:without_context": True,
        "boost/*:without_contract": True,
        "boost/*:without_coroutine": True,
        "boost/*:without_fiber": True,
        "boost/*:without_graph": True,
        "boost/*:without_graph_parallel": True,
        "boost/*:without_iostreams": True,
        "boost/*:without_json": True,
        "boost/*:without_log": True,
        "boost/*:without_math": True,
        "boost/*:without_mpi": True,
        "boost/*:without_nowide": True,
        "boost/*:without_python": True,
        "boost/*:without_random": True,
        "boost/*:without_regex": True,
        "boost/*:without_serialization": True,
        "boost/*:without_stacktrace": True,
        "boost/*:without_test": True,
        "boost/*:without_timer": True,
        "boost/*:without_type_erasure": True,
        "boost/*:without_wave": True,

        "ffmpeg/*:avdevice": False,
        "ffmpeg/*:avfilter": False,
        "ffmpeg/*:postproc": False,
        "ffmpeg/*:swresample": False,
        "ffmpeg/*:with_freetype": False,
        "ffmpeg/*:with_libfdk_aac": False,
        "ffmpeg/*:with_libmp3lame": False,
        "ffmpeg/*:with_libvpx": False,
        "ffmpeg/*:with_libwebp": False,
        "ffmpeg/*:with_libx264": False,
        "ffmpeg/*:with_libx265": False,
        "ffmpeg/*:with_openh264": False,
        "ffmpeg/*:with_openjpeg": False,
        "ffmpeg/*:with_opus": False,
        "ffmpeg/*:with_programs": False,
        "ffmpeg/*:with_ssl": False,
        "ffmpeg/*:with_vorbis": False,

        "sdl/*:vulkan": False,

        "sdl_image/*:lbm": False,
        "sdl_image/*:pnm": False,
        "sdl_image/*:svg": False,
        "sdl_image/*:tga": False,
        "sdl_image/*:with_libjpeg": False,
        "sdl_image/*:with_libtiff": False,
        "sdl_image/*:with_libwebp": False,
        "sdl_image/*:xcf": False,
        "sdl_image/*:xpm": False,
        "sdl_image/*:xv": False,

        "sdl_mixer/*:flac": False,
        "sdl_mixer/*:mad": False,
        "sdl_mixer/*:mikmod": False,
        "sdl_mixer/*:modplug": False,
        "sdl_mixer/*:nativemidi": False,
        "sdl_mixer/*:opus": False,
        "sdl_mixer/*:wav": False,

        "qt/*:config": " ".join(_qtOptions),
        "qt/*:qttools": True,
        "qt/*:with_freetype": False,
        "qt/*:with_libjpeg": False,
        "qt/*:with_md4c": False,
        "qt/*:with_mysql": False,
        "qt/*:with_odbc": False,
        "qt/*:with_openal": False,
        "qt/*:with_pq": False,

        # transitive deps
        "pcre2/*:build_pcre2grep": False, # doesn't link to overridden bzip2 & zlib, the tool isn't needed anyway
    }

    def configure(self):
        # SDL_image and Qt depend on it, in iOS both are static
        self.options["libpng"].shared = self.settings.os != "iOS"

        self.options["qt"].openssl = not is_apple_os(self)
        self.options["qt"].shared = self.settings.os != "iOS"
        if self.settings.os == "iOS":
            self.options["qt"].opengl = "es2"

        self.options["sdl"].sdl2main = self.settings.os != "iOS"

        if is_apple_os(self):
            self.options["sdl_image"].imageio = True

    def requirements(self):
        # TODO: will no longer be needed after merging https://github.com/conan-io/conan-center-index/pull/13399
        self.requires("libpng/1.6.38", override=True) # freetype / Qt

        # use Apple system libraries instead of external ones
        if is_apple_os(self):
            systemLibsOverrides = [
                "bzip2/1.0.8",
                "libiconv/1.17",
                "sqlite3/3.39.2",
                "zlib/1.2.12",
            ]
            for lib in systemLibsOverrides:
                self.requires(f"{lib}@vcmi/apple", override=True)

        # TODO: the latest official release of LuaJIT (which is quite old) can't be built for arm
        if not str(self.settings.arch).startswith("arm"):
            self.requires("luajit/2.0.5")

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["USING_CONAN"] = True
        tc.generate()

        deps = CMakeDeps(self)
        if os.getenv("USE_CONAN_WITH_ALL_CONFIGS", "0") == "0":
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
