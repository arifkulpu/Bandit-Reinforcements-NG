cmake_minimum_required(VERSION 3.23)

project(BanditReinforcementsNG VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for intellisense
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find dependencies
find_package(CommonLibSSE REQUIRED)
find_package(spdlog REQUIRED)
find_package(SimpleIni REQUIRED)

# Collect source files
file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "src/*.h")

# Define the SKSE plugin
add_commonlibsse_plugin(${PROJECT_NAME}
    SOURCES ${SOURCES} ${HEADERS}
)

# Set include directories
target_include_directories(${PROJECT_NAME} PRIVATE src)

# Set Precompiled Headers
target_precompile_headers(${PROJECT_NAME} PRIVATE src/PCH.h)

# Link dependencies
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        spdlog::spdlog
        SimpleIni::SimpleIni
)

# Compiler warnings
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX- /Zc:__cplusplus)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()
