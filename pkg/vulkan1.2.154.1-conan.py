import os
from conans import ConanFile, tools


class VkpkgConan(ConanFile):
    name = "vkpkg"
    version = "1.2.154.1"
    settings = "os", "compiler", "build_type", "arch"
    description = "make conan package of first vulkan version in VulkanSDK"
    url = "None"
    license = "None"
    author = "None"
    topics = None

    build_dir = 'C:/VulkanSDK/1.2.154.1'

    def package(self):
        inc_folder = os.path.join( self.build_dir, 'Include' )
        bin_folder = lib_folder = ''

        if self.settings.arch == 'x86':
            bin_folder = os.path.join( self.build_dir, 'Bin32' )
            lib_folder = os.path.join( self.build_dir, 'Lib32' )

        elif self.settings.arch == 'x86_64':
            bin_folder = os.path.join( self.build_dir, 'Bin' )
            lib_folder = os.path.join( self.build_dir, 'Lib' )

        self.copy(pattern='*.exe', dst='bin', src=bin_folder, excludes='*cube*')
        self.copy(pattern='*', dst='include', src=inc_folder)
        self.copy(pattern='*', dst='lib', src=lib_folder)

    def package_info(self):
        self.cpp_info.libs = [ 'vulkan-1' ]
