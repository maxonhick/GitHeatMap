cmake_minimum_required(VERSION 3.10)
project(GitHeatMap)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(LIBGIT2 libgit2)
endif()

if(LIBGIT2_FOUND AND LIBGIT2_LIBRARIES)
    if(NOT LIBGIT2_LIBRARIES MATCHES "/")
        message(STATUS "pkg-config returned short name: ${LIBGIT2_LIBRARIES}")
        find_library(LIBGIT2_LIBRARY_FULL
            NAMES ${LIBGIT2_LIBRARIES} git2 libgit2
            PATHS
                ${LIBGIT2_LIBRARY_DIRS}
                /opt/homebrew/lib
                /usr/local/lib
                /usr/lib
                /usr/lib/x86_64-linux-gnu
                /usr/lib/aarch64-linux-gnu
            NO_DEFAULT_PATH
        )
        if(LIBGIT2_LIBRARY_FULL)
            set(LIBGIT2_LIBRARIES ${LIBGIT2_LIBRARY_FULL})
            message(STATUS "Found full path: ${LIBGIT2_LIBRARIES}")
        endif()
    endif()
endif()

if(NOT LIBGIT2_FOUND OR NOT LIBGIT2_LIBRARIES MATCHES "/")
    # Ищем библиотеку вручную
    find_library(LIBGIT2_LIBRARY
        NAMES git2 libgit2
        PATHS
            # macOS
            /opt/homebrew/lib
            /usr/local/lib
            # Linux
            /usr/lib
            /usr/lib/x86_64-linux-gnu
            /usr/lib/aarch64-linux-gnu
            # Windows (vcpkg)
            ${VCPKG_ROOT}/installed/x64-windows/lib
            ${VCPKG_ROOT}/installed/x86-windows/lib
            C:/vcpkg/installed/x64-windows/lib
            C:/vcpkg/installed/x86-windows/lib
            # Windows (MSYS2)
            C:/msys64/mingw64/lib
            C:/msys64/mingw32/lib
            ${CMAKE_SYSTEM_LIBRARY_PATH}
    )
    
    find_path(LIBGIT2_INCLUDE_DIR
        NAMES git2.h
        PATHS
            # macOS
            /opt/homebrew/include
            /usr/local/include
            # Linux
            /usr/include
            /usr/local/include
            # Windows (vcpkg)
            ${VCPKG_ROOT}/installed/x64-windows/include
            ${VCPKG_ROOT}/installed/x86-windows/include
            C:/vcpkg/installed/x64-windows/include
            C:/vcpkg/installed/x86-windows/include
            # Windows (MSYS2)
            C:/msys64/mingw64/include
            C:/msys64/mingw32/include
            ${CMAKE_SYSTEM_INCLUDE_PATH}
    )
    
    if(LIBGIT2_LIBRARY AND LIBGIT2_INCLUDE_DIR)
        set(LIBGIT2_FOUND TRUE)
        set(LIBGIT2_INCLUDE_DIRS ${LIBGIT2_INCLUDE_DIR})
        set(LIBGIT2_LIBRARIES ${LIBGIT2_LIBRARY})
    endif()
endif()

if(NOT LIBGIT2_FOUND)
    message(FATAL_ERROR 
        "libgit2 not found!\n"
        "  Install it:\n"
        "    Linux:  sudo apt install libgit2-dev\n"
        "    macOS:  brew install libgit2\n"
        "    Windows: vcpkg install libgit2:x64-windows\n"
    )
endif()

message(STATUS "✓ libgit2 found: ${LIBGIT2_LIBRARIES}")

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/analyzer/repo_analyzer.cpp
    src/analyzer/output_printer.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_include_directories(GitHeatMap PRIVATE ${LIBGIT2_INCLUDE_DIRS})
target_link_libraries(GitHeatMap PRIVATE ${LIBGIT2_LIBRARIES})

if(APPLE)
    target_link_libraries(GitHeatMap PRIVATE
        "-framework CoreFoundation"
        "-framework Security"
    )
endif()

if(UNIX AND NOT APPLE)
    find_package(Threads REQUIRED)
    target_link_libraries(GitHeatMap PRIVATE Threads::Threads)
endif()

if(WIN32)
    target_link_libraries(GitHeatMap PRIVATE
        winhttp
        crypt32
        rpcrt4
        ole32
    )
endif()

message(STATUS "Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")

option(BUILD_TESTS "Build tests" ON)

if(BUILD_TESTS)
    enable_testing()

    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    add_executable(githeatmap_tests
        tests/test_filters.cpp
        tests/test_output_printer.cpp
        tests/test_repo_analyzer.cpp
        src/analyzer/repo_analyzer.cpp
        src/analyzer/output_printer.cpp
    )

    target_include_directories(githeatmap_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${LIBGIT2_INCLUDE_DIRS}
    )

    target_link_libraries(githeatmap_tests PRIVATE
        GTest::gtest_main
        ${LIBGIT2_LIBRARIES}
    )

    include(GoogleTest)
    gtest_discover_tests(githeatmap_tests)
endif()