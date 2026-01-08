cmake_minimum_required(VERSION 3.31)
project(ChessComputer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(ChessComputer app/main.cpp
        include/chess/core/board.h
        src/chess/core/board.cpp
        src/chess/core/movegen.cpp
        include/chess/core/movegen.h
        include/app/utils.h
        src/chess/figures/pawns.cpp
        src/chess/figures/knights.cpp
        src/chess/figures/sliding.cpp
        src/chess/figures/king.cpp
        src/chess/rules/attacks.cpp
        include/chess/rules/attacks.h
        src/chess/rules/fen.cpp
        include/chess/rules/fen.h
        src/chess/rules/makemove.cpp
        include/chess/rules/makemove.h
        include/chess/figures/king.h
        include/chess/figures/knights.h
        include/chess/figures/pawns.h
        include/chess/figures/sliding.h
        include/chess/rules/perft.h
        src/chess/rules/perft.cpp
        include/chess/search/eval.h
        include/chess/search/search.h
        src/chess/search/eval.cpp
        src/chess/search/search.cpp
        include/chess/search/tt.h
        src/chess/search/tt.cpp
        include/app/test_runner.h
        app/test_runner.cpp
)

target_include_directories(ChessComputer PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)