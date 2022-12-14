cmake_minimum_required(VERSION 3.13)
project(falcon9_decoder)

file(GLOB_RECURSE SRC "src/*.cpp")

add_library(falcon9_decoder SHARED ${SRC})
target_link_libraries(falcon9_decoder PRIVATE sdrpp_core)
set_target_properties(falcon9_decoder PROPERTIES PREFIX "")

target_include_directories(falcon9_decoder PRIVATE "src/")

if (MSVC)
    target_compile_options(falcon9_decoder PRIVATE /O2 /Ob2 /std:c++17 /EHsc)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(falcon9_decoder PRIVATE -O3 -std=c++17 -Wno-unused-command-line-argument -undefined dynamic_lookup)
else ()
    target_compile_options(falcon9_decoder PRIVATE -O3 -std=c++17)
endif ()

# Install directives
install(TARGETS falcon9_decoder DESTINATION lib/sdrpp/plugins)