from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, cmake_layout


class LibdxfrwConan(ConanFile):
    name = "libdxfrw"
    version = "2.0.0"
    package_type = "library"
    license = "GPL-2.0-or-later"
    url = "https://github.com/LibreCAD/libdxfrw"
    description = (
        "C++17 library for reading and writing ASCII and binary DXF and "
        "reading DWG files"
    )
    topics = ("dxf", "dwg", "cad")

    settings = "os", "arch", "compiler", "build_type"
    options = {"shared": [True, False]}
    default_options = {"shared": False}

    # Keep the recipe self-contained for `conan create .`: no network clone is
    # needed and the canonical CMake source manifest remains the single source
    # of truth for the library translation units.
    exports_sources = (
        "CMakeLists.txt",
        "cmake/**",
        "src/**",
        "dwg2dxf/**",
        "libdxfrw_sources.cmake",
        "libdxfrw.pc.in",
        "COPYING",
    )

    def layout(self):
        cmake_layout(self)

    def generate(self):
        toolchain = CMakeToolchain(self)
        toolchain.variables["BUILD_SHARED_LIBS"] = bool(self.options.shared)
        toolchain.variables["LIBDXFRW_BUILD_DOC"] = False
        toolchain.variables["LIBDXFRW_BUILD_DWG2DXF"] = False
        toolchain.variables["LIBDXFRW_BUILD_TESTS"] = False
        toolchain.variables["LIBDXFRW_INSTALL_DEV"] = True
        toolchain.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["dxfrw"]
        self.cpp_info.set_property("cmake_file_name", "libdxfrw")
        self.cpp_info.set_property("cmake_target_name", "libdxfrw::libdxfrw")
