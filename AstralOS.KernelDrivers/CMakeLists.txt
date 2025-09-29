cmake_minimum_required(VERSION 3.10)
project(AstralOSKernelDrivers)

execute_process(COMMAND bash -c "if [[ \$(pwd) == *\"OS/AstralOS.KernelDrivers\" ]]; then exit 0; else echo 'Please go inside AstralOS.KernelDrivers'; exit 1; fi"
                RESULT_VARIABLE check_directory_result)
if(NOT check_directory_result EQUAL 0)
    message(FATAL_ERROR "Please go inside AstralOS.KernelDrivers")
endif()

add_custom_target(init
    COMMAND cd ${CMAKE_SOURCE_DIR} && mkdir -p bin && mkdir -p bin/Drivers
    COMMAND cd ${CMAKE_SOURCE_DIR}/AstralOS.AHCI && cmake . && cmake --build . --target init
    COMMAND cd ${CMAKE_SOURCE_DIR}/AstralOS.IDE && cmake . && cmake --build . --target init
    COMMAND cd ${CMAKE_SOURCE_DIR}/AstralOS.GPT && cmake . && cmake --build . --target init
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Initializing all drivers"
)

add_custom_target(build
    COMMAND cd ${CMAKE_SOURCE_DIR}/AstralOS.AHCI && cmake . && cmake --build . --target build
    COMMAND cd ${CMAKE_SOURCE_DIR}/AstralOS.IDE && cmake . && cmake --build . --target build
    COMMAND cd ${CMAKE_SOURCE_DIR}/AstralOS.GPT && cmake . && cmake --build . --target build
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building all drivers"
)

add_custom_target(_clean
    COMMAND cd ${CMAKE_SOURCE_DIR}/AstralOS.AHCI && cmake . && cmake --build . --target _clean
    COMMAND cd ${CMAKE_SOURCE_DIR}/AstralOS.IDE && cmake . && cmake --build . --target _clean
    COMMAND cd ${CMAKE_SOURCE_DIR}/AstralOS.GPT && cmake . && cmake --build . --target _clean
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Cleaning all drivers"
)