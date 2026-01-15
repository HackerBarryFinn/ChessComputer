cmake_minimum_required(VERSION 3.20)

project(ChessComputer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(chess
        src/chess/core/board.cpp
        src/chess/core/movegen.cpp

        src/chess/pieces/pawns.cpp
        src/chess/pieces/knights.cpp
        src/chess/pieces/sliding.cpp
        src/chess/pieces/king.cpp

        src/chess/rules/attacks.cpp
        src/chess/rules/fen.cpp
        src/chess/rules/makemove.cpp
        src/chess/rules/perft.cpp

        src/chess/search/eval.cpp
        src/chess/search/search.cpp
        src/chess/search/tt.cpp
)

target_include_directories(chess
        PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

# Spiel (CLI)
add_executable(ChessPlay
        app/play_main.cpp
        app/game.cpp
)
target_link_libraries(ChessPlay PRIVATE chess)

# Tests
add_executable(ChessTests
        app/main.cpp
        tests/test_runner.cpp
)
target_link_libraries(ChessTests PRIVATE chess)