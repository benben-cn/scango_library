cmake_minimum_required(VERSION 3.20)
project(Scango_Dll)

if(MSVC)
    # 对于MSVC编译器
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/execution-charset:utf-8>")
else()
    # 对于GCC/Clang等编译器
    add_compile_options(-finput-charset=UTF-8 -fexec-charset=UTF-8)
endif()

# 定义用于 DLL 导出的宏
add_definitions(-DSCANGODLL__)

include_directories(${PROJECT_SOURCE_DIR}/dcframe/kernel/comm)
include_directories(${PROJECT_SOURCE_DIR}/dcframe/public)
include_directories(${PROJECT_SOURCE_DIR}/dcframe/tools/array)
include_directories(${PROJECT_SOURCE_DIR}/dcframe/tools/cjson)
include_directories(${PROJECT_SOURCE_DIR}/dcframe/tools/comm)
include_directories(${PROJECT_SOURCE_DIR}/dcframe/tools/dtframe)
include_directories(${PROJECT_SOURCE_DIR}/dcframe/tools/encrypt)
include_directories(${PROJECT_SOURCE_DIR}/dcframe/tools/list)
include_directories(${PROJECT_SOURCE_DIR}/dcframe/tools/lock)
include_directories(${PROJECT_SOURCE_DIR}/dcframe/tools/pkghelper)
include_directories(${PROJECT_SOURCE_DIR}/dcframe/tools/socket)
include_directories(${PROJECT_SOURCE_DIR}/dcframe/tools/sort)
include_directories(${PROJECT_SOURCE_DIR}/dcframe/tools/vector)
include_directories(${PROJECT_SOURCE_DIR}/dcframe/tools/zlib)

file(GLOB CPP_SOURCES ${PROJECT_SOURCE_DIR}/*.cpp)
file(GLOB CPP_d_k_comm ${PROJECT_SOURCE_DIR}/dcframe/kernel/comm/*.cpp)
file(GLOB CPP_d_k_vfs ${PROJECT_SOURCE_DIR}/dcframe/kernel/vfs/*.cpp)
file(GLOB CPP_d_t_array ${PROJECT_SOURCE_DIR}/dcframe/tools/array/*.cpp)
file(GLOB CPP_d_t_cjson ${PROJECT_SOURCE_DIR}/dcframe/tools/cjson/*.c)
file(GLOB CPP_d_t_comm ${PROJECT_SOURCE_DIR}/dcframe/tools/comm/*.cpp)
file(GLOB CPP_d_t_dtframe ${PROJECT_SOURCE_DIR}/dcframe/tools/dtframe/*.cpp)
file(GLOB CPP_d_t_encrypt ${PROJECT_SOURCE_DIR}/dcframe/tools/encrypt/*.cpp)
file(GLOB CPP_d_t_list ${PROJECT_SOURCE_DIR}/dcframe/tools/list/*.cpp)
file(GLOB CPP_d_t_lock ${PROJECT_SOURCE_DIR}/dcframe/tools/lock/*.cpp)
file(GLOB CPP_d_t_pkghelper ${PROJECT_SOURCE_DIR}/dcframe/tools/pkghelper/*.cpp)
file(GLOB CPP_d_t_socket ${PROJECT_SOURCE_DIR}/dcframe/tools/socket/*.cpp)
file(GLOB CPP_d_t_sort ${PROJECT_SOURCE_DIR}/dcframe/tools/sort/*.cpp)
file(GLOB CPP_d_t_vector ${PROJECT_SOURCE_DIR}/dcframe/tools/vector/*.cpp)
file(GLOB CPP_d_t_zlib ${PROJECT_SOURCE_DIR}/dcframe/tools/zlib/*.cpp)

set(SOURCES 
    ${CPP_SOURCES} ${CPP_d_k_comm} ${CPP_d_k_vfs} ${CPP_d_t_array} ${CPP_d_t_cjson} 
    ${CPP_d_t_comm} ${CPP_d_t_dtframe} ${CPP_d_t_encrypt} ${CPP_d_t_list} ${CPP_d_t_lock} 
    ${CPP_d_t_pkghelper} ${CPP_d_t_socket} ${CPP_d_t_sort} ${CPP_d_t_vector} ${CPP_d_t_zlib})

set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/output)
#link_directories(${PROJECT_SOURCE_DIR}/lib)
add_library(SCANGO_Dll SHARED ${SOURCES})
target_compile_definitions(SCANGO_Dll PUBLIC IS_BUILDING_SHARED)
message(STATUS "Building shared version...")

# 包含 DLL 的头文件目录
#include_directories(${PROJECT_SOURCE_DIR}/include)

# 链接到 DLL 的导入库
#target_link_libraries(SCANGO_Dll PUBLIC pthreadVC2.lib PUBLIC zlib.lib PUBLIC ws2_32.lib)
#target_link_libraries(SCANGO_Dll ${PROJECT_SOURCE_DIR}/lib/pthreadVC2.lib ${PROJECT_SOURCE_DIR}/lib/zlib.lib)