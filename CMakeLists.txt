cmake_minimum_required(VERSION 3.25)
project(dean LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---- library --------------------------------------------------------------

add_subdirectory(src)

# ---- doctest (fetched automatically) --------------------------------------

include(FetchContent)
FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG        v2.4.11
)
FetchContent_MakeAvailable(doctest)

# ---- tests ----------------------------------------------------------------

enable_testing()
add_subdirectory(tests)

# ---- examples -------------------------------------------------------------

add_subdirectory(example)
