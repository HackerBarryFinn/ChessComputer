cmake_minimum_required(VERSION 3.31)
project(ChessComputer)

set(CMAKE_CXX_STANDARD 20)

add_executable(ChessComputer src/main.cpp
        src/board.h
        src/board.cpp
        src/movegen.cpp
        src/movegen.h
        src/utils.h
        src/figures/pawns.cpp
        src/figures/knights.cpp
        src/figures/sliding.cpp
        src/figures/king.cpp
        src/attacks.cpp
        src/attacks.h
)
