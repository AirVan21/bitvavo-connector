from conans import ConanFile, CMake, tools

class BitvavoConnectorConan(ConanFile):
    name = "bitvavo_connector"
    version = "1.0"
    settings = "os", "compiler", "build_type"
    generators = "cmake", "cmake_find_package", "cmake_paths"

    def requirements(self):
        self.requires("openssl/1.1.1t")

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()