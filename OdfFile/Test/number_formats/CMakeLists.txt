cmake_minimum_required(VERSION 3.10)
project(odf_number_formats_test LANGUAGES CXX)

enable_testing()
find_package(Boost CONFIG REQUIRED)

add_executable(odf_number_formats_test
    main.cpp
    ../../Reader/Converter/xlsx_numFmts.cpp
)
target_compile_features(odf_number_formats_test PRIVATE cxx_std_17)
target_include_directories(odf_number_formats_test PRIVATE ../../Common ${Boost_INCLUDE_DIRS})
add_test(NAME odf_number_formats_test COMMAND odf_number_formats_test)
