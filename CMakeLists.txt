cmake_minimum_required(VERSION 3.17)
project(simplefs C)

set(CMAKE_C_STANDARD 11)

add_executable(simplefs main.c fs.c terminal.c fs)