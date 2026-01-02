cmake_minimum_required(VERSION 3.31)
project(ChessComputer)

set(CMAKE_CXX_STANDARD 20)

add_executable(ChessComputer src/main.cpp
        include/board.h
        src/board.cpp
        src/movegen.cpp
        include/movegen.h
        include/utils.h
        src/figures/pawns.cpp
        src/figures/knights.cpp
        src/figures/sliding.cpp
        src/figures/king.cpp
        src/attacks.cpp
        include/attacks.h
        src/fen.cpp
        include/fen.h
        src/makemove.cpp
        include/makemove.h
        include/figures/king.h
        include/figures/knights.h
        include/figures/pawns.h
        include/figures/sliding.h
)
