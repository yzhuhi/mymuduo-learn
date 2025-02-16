cmake_minimum_required(VERSION 2.5)
project(mymuduo)

# cmake ==> makefile ==>make 如果新添加文件后make并没有编译，因为makefile没有更改，需要将cmake生成的makefile文件全部清除再cmake
# mymuduo 最终编译成so动态库， 设置动态库路径, 放在根目录的lib目录下
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
# 设置调试信息 以及启动c++11语言标准
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11")

# 定义参与编译的的源文件
aux_source_directory(. SRC_LIST)
# 编译动态库
add_library(mymuduo SHARED ${SRC_LIST})

