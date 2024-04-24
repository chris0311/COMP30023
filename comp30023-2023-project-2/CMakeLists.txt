cmake_minimum_required(VERSION 3.25)
project(comp30023_2023_project_2 C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(comp30023_2023_project_2
        client.c
        rpc.c
        rpc.h
        server.c
        server.h)
