from conans import ConanFile, CMake


class VkcppConan(ConanFile):
    name = "vkcpp"
    version = "0.0.1"
    license = "MIT"
    author = "aneelatwork"
    url = "https://github.com/aneelatwork/vkcpp.git"
    description = "cpp wrapper of vulkan"
    topics = ( "vulkan" )
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}
    generators = "cmake"
    requires = "vkpkg/1.2.154.1@aneelatwork/stable"

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def source(self):
        self.run("git clone https://github.com/aneelatwork/vkcpp.git")

    def build(self):
        cmake = CMake(self)
        cmake.configure(source_folder="vkcpp")
        cmake.build()

        # Explicit way:
        # self.run('cmake %s/hello %s'
        #          % (self.source_folder, cmake.command_line))
        # self.run("cmake --build . %s" % cmake.build_config)

    def package(self):
        self.copy("*.hpp", src="vkcpp")
        self.copy("*vkcpp.lib", dst="lib", keep_path=False)
        self.copy("*.dll", dst="bin", keep_path=False)
        self.copy("*.so", dst="lib", keep_path=False)
        self.copy("*.dylib", dst="lib", keep_path=False)
        self.copy("*.a", dst="lib", keep_path=False)

    def package_info(self):
        self.cpp_info.libs = ["vkcpp"]

