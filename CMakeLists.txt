cmake_minimum_required(VERSION 3.31)
project(ChessComputer)

set(CMAKE_CXX_STANDARD 20)

add_executable(ChessComputer src/main.cpp
        src/board.h
        src/board.cpp
)
