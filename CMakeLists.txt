cmake_minimum_required(VERSION 3.0)

project(mHDLC C)

add_library(mhdlc STATIC
    mhdlc.c
    mhdlc.h
)

add_executable(main main.c)
target_link_libraries(main mhdlc)
