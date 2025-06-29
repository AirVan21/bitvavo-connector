from conan import ConanFile

class BitvavoConnectorConan(ConanFile):
    name = "bitvavo_connector"
    version = "1.0"
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake", "cmake_find_package"

    def requirements(self):
        self.requires("boost/1.82.0")
        self.requires("openssl/1.1.1t")