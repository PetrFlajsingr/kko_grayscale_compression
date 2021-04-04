cmake_minimum_required(VERSION 3.17)
project(huff_codec)

set(CMAKE_CXX_STANDARD 20)
list(APPEND flags "-fconcepts" "-fconcepts-diagnostics-depth=10" "-fcoroutines" "-Werror=return-type"
        "-Wall" "-Wextra" "-Werror" "-Wpedantic" "-Wno-unknown-pragmas" "-Wno-unused-function"
        "-Wpointer-arith" "-Wno-cast-qual" "-Wno-type-limits" "-Wno-strict-aliasing" "-g")

if (CMAKE_BUILD_TYPE MATCHES Debug)
    message(" ")
    message("CMAKE IN DEBUG MODE")
    message(" ")
elseif (CMAKE_BUILD_TYPE MATCHES Release)
    message(" ")
    message("CMAKE IN RELEASE MODE")
    message(" ")
    list(APPEND flags "-O3")
elseif (CMAKE_BUILD_TYPE MATCHES ASAN)
    message(" ")
    message("CMAKE IN ADDRESS SANITISE MODE")
    message(" ")
    list(APPEND flags "-fno-omit-frame-pointer" "-fsanitize=address")
elseif (CMAKE_BUILD_TYPE MATCHES LSAN)
    message(" ")
    message("CMAKE IN LEAK SANITISE MODE")
    message(" ")
    list(APPEND flags "-fno-omit-frame-pointer" "-fsanitize=leak")
elseif (CMAKE_BUILD_TYPE MATCHES UBSAN)
    message(" ")
    message("CMAKE IN UNDEFINED SANITISE MODE")
    message(" ")
    list(APPEND flags "-fsanitize=undefined")
elseif (CMAKE_BUILD_TYPE MATCHES TSAN)
    message(" ")
    message("CMAKE IN THREAD SANITISE MODE")
    message(" ")
    list(APPEND flags "-fsanitize=thread")
elseif (CMAKE_BUILD_TYPE MATCHES MSAN)
    message(" ")
    message("CMAKE IN MEMORY SANITISE MODE")
    message(" ")
    list(APPEND flags "-fsanitize=memory")
endif ()

set(SOURCES
        main.cpp
        )

set(INCLUDES
        args/ValidPathCheckAction.h
        RawGrayscaleImageDataReader.h
        static_encoding.h
        static_decoding.h
        constants.h
        BinaryEncoder.h
        EncodingTreeData.h
        models.h
        utils.h
        Tree.h
        )

if (CMAKE_BUILD_TYPE MATCHES ASAN)
    set(SAN_LIB -lasan)
elseif (CMAKE_BUILD_TYPE MATCHES UBSAN)
    set(SAN_LIB -lubsan)
else ()
    set(SAN_LIB)
endif ()

include_directories(libs)

add_executable(huff_codec ${SOURCES})
add_executable(test test.cpp)
add_executable(bench bench.cpp)

target_compile_options(huff_codec PRIVATE ${flags})
target_compile_options(test PRIVATE ${flags})
target_compile_options(bench PRIVATE ${flags})
target_link_libraries(huff_codec ${SAN_LIB})
target_link_libraries(test ${SAN_LIB})

if (MEASURE_BUILD_TIME)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CMAKE_COMMAND} -E time")
endif ()