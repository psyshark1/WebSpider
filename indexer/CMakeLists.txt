cmake_minimum_required (VERSION 3.8)

set(libpqxx_DIR "D:\\dev\\libpqxx-7.9.2")
set(OPENSSL_ROOT_DIR "D:\\dev\\openssl-3.3.2\\x64")

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)

set(BOOST_ROOT "D:\\dev\\boost_1_85")

find_package(Boost REQUIRED COMPONENTS system thread locale)

if(NOT Boost_FOUND)
    message(FATAL_ERROR "Could not find Boost")
endif()

find_package(OpenSSL REQUIRED)

project(indexer)

set(INDEXER indexer.h indexer.cpp)

add_subdirectory(${libpqxx_DIR} "libpqxx-build") 
set(PostgreSQL_ADDITIONAL_VERSIONS "11")

add_library(indexer STATIC ${INDEXER})

target_link_libraries(indexer PUBLIC ${Boost_LIBRARIES} libpqxx::pqxx OpenSSL::SSL)
target_include_directories(indexer PRIVATE ${Boost_INCLUDE_DIRS})