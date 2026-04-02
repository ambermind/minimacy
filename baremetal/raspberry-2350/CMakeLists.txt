# HOW TO Compile this project:
# Install Raspberry Pi Pico extension
# open this folder
# in left panel, click Raspberry Pi Pico Projects
# click "Configure CMake" => will create build/ directory
# click "Compile" in bottom bar
# restart Pico board while pressing "Boot button"
# click "Run" in bottom bar


cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#add_compile_options("-DON_ILABS_CHALLENGER_RP2350_BCONNECT")
add_compile_options("-DON_MM_2350")
#add_compile_options("-DON_SPARKFUN_PRO_MICRO_RP2350")
#add_compile_options("-DON_ADAFRUIT_FEATHER_RP2350_PSRAM")
#add_compile_options("-DON_PICO_2")

add_compile_options("-Wno-discarded-qualifiers")

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2 CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(minimacy C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

add_executable(minimacy)
pico_generate_pio_header(minimacy ${CMAKE_CURRENT_LIST_DIR}/raspberry-2350-pio.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR})

# Add executable. Default name is the project name, version 0.1

target_sources(minimacy PRIVATE 
	raspberry-2350.c
	raspberry-2350-psram.c
	raspberry-2350-hw.c

	../../src/compiler.c
	../../src/compiler_call.c
	../../src/compiler_globals.c
	../../src/compiler_instances.c
	../../src/compiler_letgetset.c
	../../src/compiler_locals.c
	../../src/compiler_loops.c
	../../src/compiler_match.c
	../../src/compiler_parser.c
	../../src/compiler_prog.c
	../../src/compiler_step1.c
	../../src/compiler_step2.c
	../../src/compiler_step3.c
	../../src/compiler_step4.c
	../../src/compiler_struct.c
	../../src/compiler_sum.c
	../../src/compiler_term.c
	../../src/compiler_try.c
	../../src/crypto_aes.c
	../../src/crypto_checksum.c
	../../src/crypto_des.c
	../../src/crypto_hash.c
	../../src/crypto_rc4.c
	../../src/hw.c
	../../src/hw_console.c
	../../src/hw_fs.c
	../../src/hw_fs_ansi.c
	../../src/hw_fs_romdisk.c
	../../src/hw_random.c
	../../src/hw_thread.c
	../../src/hw_time.c
	../../src/minimacy.c
	../../src/system_bignum.c
	../../src/system.c
	../../src/system_2d.c
	../../src/system_audio.c
	../../src/system_binary.c
	../../src/system_buffer.c
	../../src/system_bytes.c
	../../src/system_core.c
	../../src/system_convert.c
	../../src/system_crypto.c
	../../src/system_deflate.c
	../../src/system_event.c
	../../src/system_file.c
	../../src/system_inflate.c
	../../src/system_lzw.c
	../../src/system_serial.c
	../../src/system_socket.c
	../../src/system_str.c
	../../src/system_tmp.c
	../../src/system_ui.c
	../../src/system_ui_gl.c
	../../src/system_ui_x11.c
	../../src/system_worker.c
	../../src/util.c
	../../src/util_2d.c
	../../src/util_ansi_str.c
	../../src/util_ansi_math.c
	../../src/util_buffer.c
	../../src/util_convert.c
	../../src/util_hashmap.c
	../../src/util_xml.c
	../../src/vm_interpreter.c
	../../src/vm_malloc.c
	../../src/vm_memory.c
	../../src/vm_opcodes.c
	../../src/vm_term.c
	../../src/vm_thread.c
	../../src/vm_types.c
)

pico_set_program_name(minimacy "minimacy")
pico_set_program_version(minimacy "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(minimacy 1)
pico_enable_stdio_usb(minimacy 0)

# Add the standard library to the build
target_link_libraries(minimacy
        pico_stdlib
		pico_rand
		hardware_flash
		hardware_i2c
		hardware_pio
		hardware_pwm
		hardware_spi
)

# Add the standard include files to the build
target_include_directories(minimacy PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts or any other standard includes, if required
)

pico_add_extra_outputs(minimacy)

