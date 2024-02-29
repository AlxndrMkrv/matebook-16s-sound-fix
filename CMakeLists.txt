cmake_minimum_required(VERSION 3.10)

# get device information from /sys/class/sound/hwC0D0 (card 0 slot 0)
# Vendor: Conexant (0x14f11f87)
# Chip: SN6140
# Node: 116:11
# kernel modules: codec: snd_hda_codec_conexant
#            pci device: snd_hda_intel
# modalias hdaudio:v14F11F87r*a01*
# pci: 00:1f.3 Multimedia audio controller:
#              Intel Corporation Raptor Lake-P/U/H cAVS (rev 01)


project(matebook-16s-sound-fix LANGUAGES C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED YES)

if(${CMAKE_CURRENT_BINARY_DIR} STREQUAL ${PROJECT_SOURCE_DIR})
    message(FATAL_ERROR "In-source build prohibited!")
endif()

string(REPLACE "-" "_" module_name ${PROJECT_NAME})

set(kernel_dir "" CACHE STRING "Path to the kernel build directory")
if(kernel_dir STREQUAL "")
    find_program(uname_exec uname)
    execute_process(COMMAND ${uname_exec} -r
                    OUTPUT_VARIABLE uname_result
                    OUTPUT_STRIP_TRAILING_WHITESPACE)

    set(kernel_dir /usr/lib/modules/${uname_result}/build)
endif()

include_directories(${kernel_dir}/include/)
include_directories(${kernel_dir}/arch/x86/include)
include_directories(${kernel_dir}/arch/x86/include/generated/)

find_file(kernel_makefile NAMES Makefile
          PATHS ${kernel_dir} NO_DEFAULT_PATH)
find_file(kernel_config NAMES .config
          PATHS ${kernel_dir} NO_DEFAULT_PATH)

if(NOT kernel_makefile)
    message(FATAL_ERROR "There is no Makefile in kernel headers directory")
endif()

if(NOT kernel_config)
    message(FATAL_ERROR "There is no .config in kernel headers directory")
endif()

set(sources ${module_name}.c)
set(kbuild_in Kbuild.in)

# Targets for IDE to property index source files and list miscellaneous files
add_library(${PROJECT_NAME} OBJECT EXCLUDE_FROM_ALL ${sources})
add_custom_target(Misc SOURCES ${kbuild_in} README.md)
add_custom_target(Kernel SOURCES ${kernel_makefile} ${kernel_config})

# Create Kbuild file in build directory from Kbuild.in
string(REPLACE ";" " " sources_string "${sources}")
configure_file(${kbuild_in} Kbuild @ONLY)

# Create symbolic links to source files in build directory
foreach(src ${sources})
    file(CREATE_LINK ${PROJECT_SOURCE_DIR}/${src} ${src} SYMBOLIC)
endforeach()


# Add command to build the module
set(module_cmd ${CMAKE_MAKE_PROGRAM} -C ${kernel_dir}
    M=${CMAKE_CURRENT_BINARY_DIR})
add_custom_command(OUTPUT ${module_name}.ko
                   COMMAND ${module_cmd} modules
                   WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                   DEPENDS ${sources} ${CMAKE_CURRENT_BINARY_DIR}/Kbuild
                   VERBATIM)

# Add targets to build and clear module
add_custom_target(module DEPENDS ${module_name}.ko)
add_custom_target(module-clean COMMAND ${module_cmd} clean)

