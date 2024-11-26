# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

add_compile_options(-fdiagnostics-color=always)

message("ADF_PATH: $ENV{ADF_PATH}")
message("IDF_PATH: $ENV{IDF_PATH}")
set(EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/examples/common_components/qrcode)
set(EXTRA_COMPONENT_DIRS devices)
include($ENV{ADF_PATH}/CMakeLists.txt)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(test-skainet)


#Copy these line in CMakeLists.txt in root project

# Specify the output folder where binaries will be copied
set(CUSTOM_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/Binaries")

# Ensure the output folder exists
add_custom_command(
    OUTPUT ${CUSTOM_OUTPUT_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CUSTOM_OUTPUT_DIR}
    COMMENT "Ensuring output directory exists..."
)

add_custom_command(
    OUTPUT "${CUSTOM_OUTPUT_DIR}/${CMAKE_PROJECT_NAME}.bin" "${CUSTOM_OUTPUT_DIR}/${CMAKE_PROJECT_NAME}.elf" "${CUSTOM_OUTPUT_DIR}/srmodels.bin" "${CUSTOM_OUTPUT_DIR}/flasher_args.json"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.bin" "${CUSTOM_OUTPUT_DIR}/${CMAKE_PROJECT_NAME}.bin"
    COMMAND ${CMAKE_COMMAND} -E echo "Successfully copied ${CMAKE_PROJECT_NAME}.bin to ${CUSTOM_OUTPUT_DIR}/${CMAKE_PROJECT_NAME}.bin"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.elf" "${CUSTOM_OUTPUT_DIR}/${CMAKE_PROJECT_NAME}.elf"
    COMMAND ${CMAKE_COMMAND} -E echo "Successfully copied ${CMAKE_PROJECT_NAME}.elf to ${CUSTOM_OUTPUT_DIR}/${CMAKE_PROJECT_NAME}.elf"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_BINARY_DIR}/bootloader/bootloader.bin" "${CUSTOM_OUTPUT_DIR}/bootloader.bin"
    COMMAND ${CMAKE_COMMAND} -E echo "Successfully copied bootloader.bin to ${CUSTOM_OUTPUT_DIR}/bootloader.bin"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_BINARY_DIR}/partition_table/partition-table.bin" "${CUSTOM_OUTPUT_DIR}/partition-table.bin"
    COMMAND ${CMAKE_COMMAND} -E echo "Successfully copied partition-table.bin to ${CUSTOM_OUTPUT_DIR}/partition-table.bin"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_BINARY_DIR}/srmodels/srmodels.bin" "${CUSTOM_OUTPUT_DIR}/srmodels.bin"
    COMMAND ${CMAKE_COMMAND} -E echo "Successfully copied srmodels.bin to ${CUSTOM_OUTPUT_DIR}/srmodels.bin"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_BINARY_DIR}/flasher_args.json" "${CUSTOM_OUTPUT_DIR}/flasher_args.json"
    COMMAND ${CMAKE_COMMAND} -E echo "Successfully copied flasher_args.json to ${CUSTOM_OUTPUT_DIR}/flasher_args.json"
    DEPENDS "${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.bin" "${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.elf" "${CMAKE_BINARY_DIR}/bootloader/bootloader.bin" "${CMAKE_BINARY_DIR}/partition_table/partition-table.bin" "${CMAKE_BINARY_DIR}/srmodels/srmodels.bin" "${CMAKE_BINARY_DIR}/flasher_args.json"
    COMMENT "Copying binary files to ${CUSTOM_OUTPUT_DIR}..."
)


# Ensure the custom command is tied to the build process
add_custom_target(copy_binaries ALL DEPENDS "${CUSTOM_OUTPUT_DIR}/${CMAKE_PROJECT_NAME}.bin" "${CUSTOM_OUTPUT_DIR}/${CMAKE_PROJECT_NAME}.elf" "${CMAKE_BINARY_DIR}/bootloader/bootloader.bin" "${CMAKE_BINARY_DIR}/partition_table/partition-table.bin"  "${CMAKE_BINARY_DIR}/srmodels/srmodels.bin" "${CMAKE_BINARY_DIR}/flasher_args.json")