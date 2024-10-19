cmake_minimum_required (VERSION 3.8)

project (WebSpider)

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)

set(BOOST_ROOT "D:\\dev\\boost_1_85")

find_package(Boost REQUIRED COMPONENTS system thread locale)

if(NOT Boost_FOUND)
    message(FATAL_ERROR "Could not find Boost")
endif()

set(CMAKE_CXX_STANDARD 17)
add_compile_definitions(_WIN32_WINNT=0x0A01)

include_directories(http_utils)
include_directories(indexer)
include_directories(thread_pool)
include_directories(INI_Parser)
include_directories(ExceptINI)

add_executable(WebSpider stringstructs.h WebSpider.cpp)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})

add_subdirectory(http_utils)
add_subdirectory(indexer)
add_subdirectory(thread_pool)
add_subdirectory(INI_Parser)
add_subdirectory(ExceptINI)

target_link_libraries(WebSpider PRIVATE httputils indexer tpool iniparser ${Boost_LIBRARIES})
target_include_directories(WebSpider PRIVATE ${Boost_INCLUDE_DIRS})
target_compile_features(WebSpider PRIVATE cxx_std_17) 