cmake_minimum_required (VERSION 3.8)

set(OPENSSL_ROOT_DIR "D:\\dev\\openssl-3.3.2\\x64")

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)

set(BOOST_ROOT "D:\\dev\\boost_1_85")

find_package(Boost REQUIRED COMPONENTS system thread locale)

if(NOT Boost_FOUND)
    message(FATAL_ERROR "Could not find Boost")
endif()

find_package(OpenSSL REQUIRED)

project(tpool)

set(TPOOL thread_pool.h thread_pool.cpp safe_queue.cpp)

add_library(tpool STATIC ${TPOOL})

target_link_libraries(tpool PUBLIC ${Boost_LIBRARIES} OpenSSL::SSL libpqxx::pqxx)
target_include_directories(tpool PRIVATE ${Boost_INCLUDE_DIRS})