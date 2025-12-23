from dependencies.conanfile import VCMI

from conan.tools.cmake import CMakeToolchain
from conan.tools.files import save

from glob import glob
import os

class VCMIApp(VCMI):
    generators = "CMakeDeps"

    def _pathForCmake(self, path: str) -> str:
        # CMake doesn't like \ in strings
        return path.replace(os.path.sep, os.path.altsep) if os.path.altsep else path

    def _generateRuntimeLibsFile(self) -> str:
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
                runtimeLibs += map(self._pathForCmake, glob(os.path.join(runtimeLibDir, f"*.{runtimeLibExtension}")))
        save(self, runtimeLibsFile, "\n".join(runtimeLibs))

        return runtimeLibsFile

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["USING_CONAN"] = True
        tc.variables["CONAN_RUNTIME_LIBS_FILE"] = self._generateRuntimeLibsFile()
        if self.settings.os == "Android":
            tc.variables["CMAKE_ANDROID_API"] = str(self.settings.os.api_level)
            tc.variables["SDL_JAVA_SRC_DIR"] = os.path.join(self.dependencies.host["sdl"].package_folder, "share", "java", "SDL2")
        elif self.settings.os == "Windows":
            tc.variables["CONAN_RUNENV_SCRIPT"] = self._pathForCmake(os.path.join(self.build_folder, "conanrun"))
        tc.generate()
