cmake_minimum_required(VERSION 3.10)  # cmake版本最低要求

project(GateServer)
set(CMAKE_CXX_STANDARD 17)

find_package(Boost REQUIRED)
find_package(fmt REQUIRED)
# aux_source_directory(./utils/ DIR_SRC)

include_directories(${Boost_INCLUDE_DIRS})


SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)  # 设置可执行文件的输出目录
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)	   # 设置库文件的输出目录

target_link_libraries(
    fmt::fmt
)
ADD_SUBDIRECTORY(${PROJECT_SOURCE_DIR}/source/utils/log)