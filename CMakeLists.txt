cmake_minimum_required(VERSION 3.10)
project(hxd VERSION 0.1)
add_executable(hxd src/hxd.c)
install(TARGETS hxd DESTINATION bin)
