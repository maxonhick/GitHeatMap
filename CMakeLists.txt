cmake_minimum_required(VERSION 3.14)
project(GitHeatMap)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_CLI OFF CACHE BOOL "" FORCE)
set(USE_SSH OFF CACHE BOOL "" FORCE)
set(USE_HTTPS OFF CACHE BOOL "" FORCE)
set(REGEX_BACKEND "builtin" CACHE STRING "" FORCE)

FetchContent_Declare(
    libgit2
    URL https://github.com/libgit2/libgit2/archive/refs/tags/v1.8.1.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(libgit2)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/analyzer/repo_analyzer.cpp
    src/analyzer/output_printer.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${libgit2_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE libgit2package)

if(APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        "-framework CoreFoundation"
        "-framework Security"
        iconv
    )
endif()

if(UNIX AND NOT APPLE)
    find_package(Threads REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE 
        Threads::Threads
        -static-libgcc
        -static-libstdc++
    )
endif()

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        winhttp
        crypt32
        rpcrt4
        ole32
    )
    set_property(TARGET ${PROJECT_NAME} PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

option(GITHEATMAP_BUILD_TESTS "Build tests" ON)

if(GITHEATMAP_BUILD_TESTS)
    enable_testing()

    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    set(gtest_force_shared_crt OFF CACHE BOOL "" FORCE)
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
        ${libgit2_SOURCE_DIR}/include
    )

    target_link_libraries(githeatmap_tests PRIVATE
        GTest::gtest_main
        libgit2package
    )

    if(APPLE)
        target_link_libraries(githeatmap_tests PRIVATE
            "-framework CoreFoundation"
            "-framework Security"
            iconv
        )
    endif()

    if(UNIX AND NOT APPLE)
        find_package(Threads REQUIRED)
        target_link_libraries(githeatmap_tests PRIVATE Threads::Threads)
    endif()

    if(WIN32)
        target_link_libraries(githeatmap_tests PRIVATE
            winhttp
            crypt32
            rpcrt4
            ole32
        )
        set_property(TARGET githeatmap_tests PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()

    include(GoogleTest)
    gtest_discover_tests(githeatmap_tests)
endif()