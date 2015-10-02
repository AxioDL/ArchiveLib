cmake_minimum_required(VERSION 3.0)
project(ArchiveLib)

include_directories(${CMAKE_SOURCE_DIR})

add_library(Archive
    pak.h pak.c)

add_subdirectory(mkpak)
