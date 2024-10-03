cmake_minimum_required(VERSION 3.8)

set(OPENSSL_ROOT_DIR "D:\\dev\\openssl-3.3.2\\x64")

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)

set(BOOST_ROOT "D:\\dev\\boost_1_85")

find_package(Boost REQUIRED COMPONENTS system thread locale)

if(NOT Boost_FOUND)
    message(FATAL_ERROR "Could not find Boost")
endif()

project(httputils)

find_package(OpenSSL REQUIRED)
set(HTTP lnk.h http_utils.h http_utils.cpp)

add_library(httputils STATIC ${HTTP})
target_link_libraries(httputils PUBLIC OpenSSL::SSL ${Boost_LIBRARIES})
target_include_directories(httputils PRIVATE ${Boost_INCLUDE_DIRS})