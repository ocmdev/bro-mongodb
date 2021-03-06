
cmake_minimum_required(VERSION 3.1)

project(BroPluginRITAWriter)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(BroPlugin)

find_package(PkgConfig)

pkg_search_module(LIBMONGOCXX REQUIRED libmongocxx)
pkg_search_module(LIBBSONCXX REQUIRED libbsoncxx)

include_directories("/usr/local/include/mongocxx/v_noabi")
include_directories("/usr/local/include/bsoncxx/v_noabi")
include_directories("/usr/local/include/libmongoc-1.0")
include_directories("/usr/local/include/libbson-1.0")
include_directories("/usr/local/lib")

link_directories(${LIBMONGOCXX_LIBRARY_DIRS})
link_directories(${LIBBSONCXX_LIBRARY_DIRS})

bro_plugin_begin(ActiveCM RITAWriter)
bro_plugin_cc(src/writers/MongoDBBuffer.cc)
bro_plugin_cc(src/writers/AbstractMongoDBWriter.cc)
bro_plugin_cc(src/writers/BufferedMongoDBWriter.cc)
bro_plugin_cc(src/writers/RotatedBufferedMongoDBWriter.cc)
bro_plugin_cc(src/Plugin.cc)
bro_plugin_cc(src/MongoDBWriterBackend.cc)
bro_plugin_cc(src/DocBuilder.cc)
bro_plugin_dist_files(README VERSION)
bro_plugin_link_library(${LIBMONGOCXX_LIBRARIES})
bro_plugin_link_library(${LIBBSONCXX_LIBRARIES})
bro_plugin_end()

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" VERSION LIMIT_COUNT 1)

if ("${PROJECT_SOURCE_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}")
    # Allows building rpm/deb packages via "make package" in build dir.
    include(ConfigurePackaging)
    ConfigurePackaging(${VERSION})
endif ()
