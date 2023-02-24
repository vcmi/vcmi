from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.apple import is_apple_os
from conan.tools.build import cross_building
from conan.tools.cmake import CMakeDeps, CMakeToolchain
from conans import tools

def vcmi_deploy(graph, output_folder):
    conanfile = graph.root.conanfile
    conanfile.output.info(f"Deployer of VCMI. It deploys all required dependencies into {output_folder}/vcmi_deps")

    if is_apple_os(conanfile):
        conanfile.copy("*.dylib", "Frameworks", "lib")
    elif conanfile.settings.os == "Windows":
        conanfile.copy("*.dll", src="bin/archdatadir/plugins/platforms", dst="platforms")
        conanfile.copy("*.dll", src="bin/archdatadir/plugins/styles", dst="styles")
        conanfile.copy("*.dll", src="@bindirs", dst="", excludes="archdatadir/*")