cmake_minimum_required(VERSION 3.20)
project(MyNeuroTool VERSION 1.0 LANGUAGES CXX)

# ---------------------------------------------------------
# 1. 基础设置
# ---------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    add_compile_definitions(_SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING)
endif()

# 查找 Qt
find_package(Qt6 COMPONENTS Core Widgets REQUIRED)

# ---------------------------------------------------------
# 2. 配置 dcm2niix (预编译工具)
# ---------------------------------------------------------
set(DCM2NIIX_BIN_DIR "${CMAKE_SOURCE_DIR}/external/dcm2niix/bin")

if(EXISTS "${DCM2NIIX_BIN_DIR}/dcm2niix.exe")
    message(STATUS "dcm2niix found at: ${DCM2NIIX_BIN_DIR}")
    add_compile_definitions(DCM2NIIX_EXE="${DCM2NIIX_BIN_DIR}/dcm2niix.exe")
else()
    message(FATAL_ERROR "dcm2niix.exe not found! Please download from GitHub Releases and put it in external/dcm2niix/bin/")
endif()

# ---------------------------------------------------------
# 3. 配置 ANTs (预编译工具)
# ---------------------------------------------------------
set(ANTS_BIN_DIR "${CMAKE_SOURCE_DIR}/external/ANTs/bin")

if(EXISTS "${ANTS_BIN_DIR}/antsRegistration.exe")
    message(STATUS "ANTs found at: ${ANTS_BIN_DIR}")
    add_compile_definitions(ANTS_BIN_PATH="${ANTS_BIN_DIR}")
else()
    message(WARNING "ANTs not found at ${ANTS_BIN_DIR}")
    add_compile_definitions(ANTS_BIN_PATH="")
endif()

# ---------------------------------------------------------
# 4. 创建核心库
# ---------------------------------------------------------
# 确保 src/core.cpp 文件存在，如果还没有，先创建一个空文件
add_library(CoreLib STATIC
    src/core/core.cpp
 "include/dicom_series.h" "include/dicom_manager.h" "include/dicom_converter.h" "src/dicom_manager.cpp" "src/dicom_converter.cpp" "src/main.cpp" "include/nifti_manager.h" "include/core.h" "src/core/core.cpp" "src/nifti_manager.cpp" "src/image_average.cpp" "include/image_average.h")
target_include_directories(CoreLib PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(CoreLib PUBLIC Qt6::Core)

# 设置调试环境变量 (让程序运行时能找到 DLL)
if(WIN32)
    set(DEBUG_ENV "PATH=${ANTS_BIN_DIR};${DCM2NIIX_BIN_DIR};$ENV{PATH}")
    set_property(TARGET CoreLib PROPERTY VS_DEBUGGER_ENVIRONMENT "${DEBUG_ENV}")
endif()

# ---------------------------------------------------------
# 5. CLI 可执行文件 (优先实现)
# ---------------------------------------------------------
add_executable(AppCLI "src/cli/main_cli.cpp.bak" "include/dicom_series.h" "include/dicom_manager.h" "include/dicom_converter.h" "src/dicom_manager.cpp" "src/dicom_converter.cpp" "src/main.cpp" "include/nifti_manager.h" "include/core.h" "src/core/core.cpp" "src/nifti_manager.cpp" "src/image_average.cpp" "include/image_average.h")
target_link_libraries(AppCLI PRIVATE CoreLib)
if(WIN32)
    set_property(TARGET AppCLI PROPERTY VS_DEBUGGER_ENVIRONMENT "${DEBUG_ENV}")
endif()

# ---------------------------------------------------------
# 6. Qt GUI 可执行文件
# ---------------------------------------------------------
add_executable(AppGUI WIN32 "src/gui/main_gui.cpp.bak" "include/dicom_series.h" "include/dicom_manager.h" "include/dicom_converter.h" "src/dicom_manager.cpp" "src/dicom_converter.cpp" "src/main.cpp" "include/nifti_manager.h" "include/core.h" "src/core/core.cpp" "src/nifti_manager.cpp" "src/image_average.cpp" "include/image_average.h")
target_link_libraries(AppGUI PRIVATE CoreLib Qt6::Widgets)
if(WIN32)
    set_property(TARGET AppGUI PROPERTY VS_DEBUGGER_ENVIRONMENT "${DEBUG_ENV}")
endif()

# 设置默认启动项为 CLI
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT AppCLI)