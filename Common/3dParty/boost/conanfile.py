from conan import ConanFile


class BoostCustomBuildConan(ConanFile):
    name = "boost_eo"
    version = "1.72.0"

    settings = "os", "compiler", "build_type", "arch"

    def requirements(self):
        self.requires("boost/1.72.0")

    def configure(self):
        # Disable all boost libraries first
        self.options["boost"].without_all = True

        # Enable only what you need
        self.options["boost"].without_system = False
        self.options["boost"].without_filesystem = False
        self.options["boost"].without_regex = False
        self.options["boost"].without_date_time = False

        # Static build
        self.options["boost"].shared = False

        if self.settings.compiler == "msvc":
            self.options["boost"].debug_level = 0

    def package_info(self):
        # Forward Boost as if this package *were* Boost
        boost = self.dependencies["boost"]

        self.cpp_info.includedirs = boost.cpp_info.includedirs
        self.cpp_info.libdirs = boost.cpp_info.libdirs
        self.cpp_info.libs = boost.cpp_info.libs
        self.cpp_info.system_libs = boost.cpp_info.system_libs
        self.cpp_info.defines = boost.cpp_info.defines

        # Optional: alias target
        #self.cpp_info.set_property("cmake_target_name", "boost_eo::boost")