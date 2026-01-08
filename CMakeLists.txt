cmake_minimum_required(VERSION 3.20)

project(ChessComputer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Chess-Library (Engine/Logik) ---
add_library(chess
        src/chess/core/board.cpp
        src/chess/core/movegen.cpp

        src/chess/figures/pawns.cpp
        src/chess/figures/knights.cpp
        src/chess/figures/sliding.cpp
        src/chess/figures/king.cpp

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

# --- App/Tests Executable ---
add_executable(ChessComputer
        app/main.cpp
        tests/test_runner.cpp
)

target_link_libraries(ChessComputer PRIVATE chess)

# Optional: Warnungen (Compiler-abhängig)
if (MSVC)
    target_compile_options(chess PRIVATE /W4)
    target_compile_options(ChessComputer PRIVATE /W4)
else()
    target_compile_options(chess PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(ChessComputer PRIVATE -Wall -Wextra -Wpedantic)
endif()