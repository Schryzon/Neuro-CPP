cmake_minimum_required(VERSION 3.50)
project(Neuro-CPP)

# → Use C++17 (or newer)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(CURL REQUIRED)
include(FetchContent)

# — Fetch curlpp ——————————————————
FetchContent_Declare(
  curlpp
  GIT_REPOSITORY https://github.com/jpbarrette/curlpp.git
  GIT_TAG        master
)
# — Fetch nlohmann_json ———————————————
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG        v3.11.2         # use the latest stable tag
)
# — Make them available ———————————————
FetchContent_MakeAvailable(curlpp nlohmann_json)


# --- Define executable ---
add_executable(Neuro-CPP
    src/main.cpp
)

# --- Include dirs for header-only stuff ---
target_include_directories(Neuro-CPP PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# --- Link libraries ---
target_link_libraries(Neuro-CPP PRIVATE
    CURL::libcurl
    curlpp::curlpp
    nlohmann_json::nlohmann_json
    ws2_32        # Windows socket library
    wldap32       # Windows LDAP dependency for curl
)

# (If you ever switch to MinGW or a different platform, adjust those system libs as needed.)