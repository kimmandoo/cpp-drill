cmake_minimum_required(VERSION 3.16)

project(toy_llvm LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_program(LLVM_CONFIG llvm-config REQUIRED)

execute_process(
    COMMAND ${LLVM_CONFIG} --cxxflags
    OUTPUT_VARIABLE LLVM_CXXFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
separate_arguments(LLVM_CXXFLAGS NATIVE_COMMAND ${LLVM_CXXFLAGS})

execute_process(
    COMMAND ${LLVM_CONFIG} --ldflags --system-libs --libs all
    OUTPUT_VARIABLE LLVM_LDFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
separate_arguments(LLVM_LDFLAGS NATIVE_COMMAND ${LLVM_LDFLAGS})

add_executable(toy
    my_llvm.cpp
)
target_compile_options(toy PRIVATE ${LLVM_CXXFLAGS})
target_link_libraries(toy PRIVATE ${LLVM_LDFLAGS})

set(GENERATED_OBJECT ${CMAKE_BINARY_DIR}/output.o)
set(KALEIDOSCOPE_INPUT ${CMAKE_CURRENT_SOURCE_DIR}/kaleidoscope_input.txt)

add_custom_command(
    OUTPUT ${GENERATED_OBJECT}
    COMMAND /bin/bash -c "$<TARGET_FILE:toy> < ${KALEIDOSCOPE_INPUT}"
    DEPENDS toy ${KALEIDOSCOPE_INPUT}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Generating output.o from Kaleidoscope source"
    VERBATIM
)

add_custom_target(generate_output DEPENDS ${GENERATED_OBJECT})

add_executable(main
    main.cpp
    ${GENERATED_OBJECT}
)
add_dependencies(main generate_output)
