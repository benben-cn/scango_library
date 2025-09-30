cmake_minimum_required(VERSION 3.20)
project(Scango_Dll)

# 自动检测Xcode工具链
if(NOT CMAKE_OSX_SYSROOT)
    execute_process(COMMAND xcodebuild -version -sdk iphoneos Path
        OUTPUT_VARIABLE IOS_DEVICE_SDK
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(CMAKE_OSX_SYSROOT ${IOS_DEVICE_SDK})
endif()

if(CMAKE_OSX_SYSROOT MATCHES "iphonesimulator")
    set(CONFIG_SDK Release-iphonesimulator)
elseif(CMAKE_OSX_SYSROOT MATCHES "iphoneos")
    set(CONFIG_SDK Release-iphoneos)
endif()

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

#set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/output)
#link_directories(${PROJECT_SOURCE_DIR}/lib)
#生成FRAMEWORK格式的动态库，并且把公共头文件scango_dll.h放入Headers文件夹下
add_library(SCANGO_Dll SHARED ${SOURCES} scango_dll.h)
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${CONFIG_SDK}/SCANGO_Dll.framework/Modules/module.modulemap"
 CONTENT "framework module \"SCANGO_Dll\" {\n header \"scango_dll.h\"\n link \"z\"\n export *\n}"
)
set_target_properties(SCANGO_Dll PROPERTIES 
    FRAMEWORK TRUE
    FRAMEWORK_VERSION A
    MACOSX_FRAMEWORK_IDENTIFIER com.example.SCANGO_Dll
    PUBLIC_HEADER scango_dll.h
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.example.SCANGO_Dll"
    C_STANDARD 99
    XCODE_ATTRIBUTE_MODULEMAP_FILE "${CMAKE_CURRENT_BINARY_DIR}/${CONFIG_SDK}/SCANGO_Dll.framework/Modules/module.modulemap"
)
target_compile_definitions(SCANGO_Dll PUBLIC IS_BUILDING_SHARED)
message(STATUS "Building shared version...")

find_package(ZLIB REQUIRED)
# 包含 DLL 的头文件目录
include_directories(${ZLIB_INCLUDE_DIRS})

# 链接到 DLL 的导入库
target_link_libraries(SCANGO_Dll PRIVATE ${ZLIB_LIBRARIES})


