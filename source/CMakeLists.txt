# 添加头文件路径

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/utils)

# 把源码编译成一个可执行文件

add_executable(GateServer ./main.cc)

# 添加库
target_link_libraries(main utils)