#
# Copyright (C)  The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

cmake_minimum_required(VERSION 3.4.1)

# build native_app_glue as a static lib
add_library(native_app_glue STATIC
    ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)

set(CMAKE_C_FLAGS "-O3 -D ON_ANDROID")

# now build app's shared lib
add_library(native-minimacy SHARED
        ../../../../../../src/compiler.c
        ../../../../../../src/compiler_call.c
        ../../../../../../src/compiler_globals.c
        ../../../../../../src/compiler_instances.c
        ../../../../../../src/compiler_letgetset.c
        ../../../../../../src/compiler_locals.c
        ../../../../../../src/compiler_loops.c
        ../../../../../../src/compiler_match.c
        ../../../../../../src/compiler_parser.c
        ../../../../../../src/compiler_prog.c
        ../../../../../../src/compiler_step1.c
        ../../../../../../src/compiler_step2.c
        ../../../../../../src/compiler_step3.c
        ../../../../../../src/compiler_step4.c
        ../../../../../../src/compiler_struct.c
        ../../../../../../src/compiler_sum.c
        ../../../../../../src/compiler_term.c
        ../../../../../../src/compiler_try.c
        ../../../../../../src/crypto_aes.c
        ../../../../../../src/crypto_checksum.c
        ../../../../../../src/crypto_des.c
        ../../../../../../src/crypto_hash.c
        ../../../../../../src/crypto_rc4.c
        ../../../../../../src/hw.c
        ../../../../../../src/hw_console.c
        ../../../../../../src/hw_fs.c
        ../../../../../../src/hw_fs_ansi.c
        ../../../../../../src/hw_fs_romdisk.c
        ../../../../../../src/hw_random.c
        ../../../../../../src/hw_thread.c
        ../../../../../../src/hw_time.c
        ../../../../../../src/minimacy.c
        ../../../../../../src/system_bignum.c
        ../../../../../../src/system.c
        ../../../../../../src/system_2d.c
        ../../../../../../src/system_audio.c
        ../../../../../../src/system_binary.c
        ../../../../../../src/system_buffer.c
        ../../../../../../src/system_bytes.c
        ../../../../../../src/system_core.c
        ../../../../../../src/system_convert.c
        ../../../../../../src/system_crypto.c
        ../../../../../../src/system_deflate.c
        ../../../../../../src/system_event.c
        ../../../../../../src/system_file.c
        ../../../../../../src/system_inflate.c
        ../../../../../../src/system_lzw.c
        ../../../../../../src/system_serial.c
        ../../../../../../src/system_socket.c
        ../../../../../../src/system_str.c
        ../../../../../../src/system_tmp.c
        ../../../../../../src/system_ui.c
        ../../../../../../src/system_ui_gl.c
        ../../../../../../src/system_worker.c
        ../../../../../../src/util.c
        ../../../../../../src/util_2d.c
        ../../../../../../src/util_buffer.c
        ../../../../../../src/util_convert.c
        ../../../../../../src/util_hashmap.c
        ../../../../../../src/util_xml.c
        ../../../../../../src/vm_interpreter.c
        ../../../../../../src/vm_memory.c
        ../../../../../../src/vm_opcodes.c
        ../../../../../../src/vm_term.c
        ../../../../../../src/vm_thread.c
        ../../../../../../src/vm_types.c
        ./system_ui_android.c)

# Export ANativeActivity_onCreate(),
# Refer to: https://github.com/android-ndk/ndk/issues/381.

set(CMAKE_SHARED_LINKER_FLAGS
    "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

target_include_directories(native-minimacy PRIVATE
    ${ANDROID_NDK}/sources/android/native_app_glue)

# add lib dependencies
target_link_libraries(native-minimacy
    android
    native_app_glue
    log
    GLESv2
    EGL
    OpenSLES
    m)
