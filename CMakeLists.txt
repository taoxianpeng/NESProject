cmake_minimum_required (VERSION 3.14)

# 如果支持，请为 MSVC 编译器启用热重载。
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("nes")

# 启用 FetchContent 模块
include(FetchContent)

# 设置选项
option(USE_SYSTEM_SPDLOG "Use system-installed spdlog" OFF)
option(FETCH_SPDLOG "Fetch spdlog from GitHub if not found" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(${CMAKE_SOURCE_DIR}/include)

# 查找或获取 spdlog
if(USE_SYSTEM_SPDLOG)
    find_package(spdlog 1.14 REQUIRED)
    message(STATUS "Using system spdlog")
else()
    if(FETCH_SPDLOG)
        message(STATUS "Fetching spdlog from GitHub...")
        
        include(FetchContent)
        
        FetchContent_Declare(
            spdlog
            GIT_REPOSITORY https://github.com/gabime/spdlog.git
            GIT_TAG v1.14.1
            GIT_SHALLOW TRUE
            OVERRIDE_FIND_PACKAGE  # 优先使用下载的版本
        )
        
        # 配置 spdlog 选项
        set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
        set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(SPDLOG_BUILD_BENCH OFF CACHE BOOL "" FORCE)
        set(SPDLOG_FMT_EXTERNAL OFF CACHE BOOL "" FORCE)  # 使用内置 fmt
        
        FetchContent_MakeAvailable(spdlog)
        
        message(STATUS "spdlog fetched successfully")
    else()
        message(STATUS "spdlog not found and FETCH_SPDLOG is OFF")
    endif()
endif()

set(SOURCES
    
    ${CMAKE_SOURCE_DIR}/src/bus.cpp
    ${CMAKE_SOURCE_DIR}/src/olc6502.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES} "src/olcPixelGameEngine.h" "src/olcNes_Video1_6502.cpp")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

target_link_libraries(${PROJECT_NAME} PRIVATE spdlog::spdlog)
