cmake_minimum_required(VERSION 3.16)

# Android build of Airline Tycoon Deluxe (fan version).
# Builds SDL2, SDL2_image, SDL2_mixer and SDL2_ttf from source (fetched into this folder, see
# ../../README.md) and the game itself as libmain.so, which SDL's Java activity loads.
project(AirlineTycoonAndroid C CXX)

get_filename_component(AT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)

foreach(dep SDL SDL_image SDL_mixer SDL_ttf)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${dep}/CMakeLists.txt")
        message(FATAL_ERROR "${dep} sources are missing in ${CMAKE_CURRENT_SOURCE_DIR}/${dep} - see android/README.md")
    endif()
endforeach()

# ---------------------------------------------------------------------------------------------
# SDL and its satellite libraries (shared, all using the one shared SDL2)
# ---------------------------------------------------------------------------------------------
set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL2_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
add_subdirectory(SDL)

set(BUILD_SHARED_LIBS ON)

set(SDL2IMAGE_INSTALL OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_SAMPLES OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_TESTS OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_VENDORED OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_DEPS_SHARED OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_AVIF OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_JXL OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_TIF OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_WEBP OFF CACHE BOOL "" FORCE)
add_subdirectory(SDL_image)

# The game only needs raw samples and OGG music (stb_vorbis is built in). MIDI music would
# need a software synthesizer with instrument files and is not available on Android.
set(SDL2MIXER_INSTALL OFF CACHE BOOL "" FORCE)
set(SDL2MIXER_SAMPLES OFF CACHE BOOL "" FORCE)
set(SDL2MIXER_VENDORED OFF CACHE BOOL "" FORCE)
set(SDL2MIXER_DEPS_SHARED OFF CACHE BOOL "" FORCE)
set(SDL2MIXER_CMD OFF CACHE BOOL "" FORCE)
set(SDL2MIXER_FLAC OFF CACHE BOOL "" FORCE)
set(SDL2MIXER_GME OFF CACHE BOOL "" FORCE)
set(SDL2MIXER_MOD OFF CACHE BOOL "" FORCE)
set(SDL2MIXER_MP3 OFF CACHE BOOL "" FORCE)
set(SDL2MIXER_MIDI OFF CACHE BOOL "" FORCE)
set(SDL2MIXER_OPUS OFF CACHE BOOL "" FORCE)
set(SDL2MIXER_WAVPACK OFF CACHE BOOL "" FORCE)
set(SDL2MIXER_WAVE ON CACHE BOOL "" FORCE)
set(SDL2MIXER_VORBIS "STB" CACHE STRING "" FORCE)
add_subdirectory(SDL_mixer)

# FreeType comes from SDL_ttf's external/freetype
set(SDL2TTF_INSTALL OFF CACHE BOOL "" FORCE)
set(SDL2TTF_SAMPLES OFF CACHE BOOL "" FORCE)
set(SDL2TTF_VENDORED ON CACHE BOOL "" FORCE)
set(SDL2TTF_HARFBUZZ OFF CACHE BOOL "" FORCE)
add_subdirectory(SDL_ttf)

set(BUILD_SHARED_LIBS OFF)

# The game's CMake files expect the target names of its own Find modules
foreach(lib image mixer ttf)
    if(NOT TARGET SDL2::SDL2_${lib})
        add_library(SDL2::SDL2_${lib} INTERFACE IMPORTED)
        target_link_libraries(SDL2::SDL2_${lib} INTERFACE SDL2_${lib})
    endif()
endforeach()
if(NOT TARGET SDL2::SDL2)
    add_library(SDL2::SDL2 INTERFACE IMPORTED)
    target_link_libraries(SDL2::SDL2 INTERFACE SDL2)
endif()
if(NOT TARGET SDL2::SDL2main)
    # not needed on Android: SDL's Java activity calls SDL_main in libmain.so
    add_library(SDL2::SDL2main INTERFACE IMPORTED)
endif()

# ---------------------------------------------------------------------------------------------
# The game (mirrors the top-level CMakeLists.txt)
# ---------------------------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# "char" is unsigned on Android/ARM, but the game was written for x86 where it is signed
add_compile_options(-fsigned-char)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${AT_ROOT}/cmake")
set(GAMEFILES_FOLDER "${AT_ROOT}/gamefiles")
set(PATCH_FOLDER "${AT_ROOT}/patch")

include_directories(${AT_ROOT}/libs/libflic/repo)

add_subdirectory(${AT_ROOT}/libs/enet at-enet EXCLUDE_FROM_ALL)
target_include_directories(enet PUBLIC ${AT_ROOT}/libs/enet/include)
add_subdirectory(${AT_ROOT}/libs/libsmacker at-libsmacker EXCLUDE_FROM_ALL)
add_subdirectory(${AT_ROOT}/libs/raknet at-raknet EXCLUDE_FROM_ALL)
set(JANSSON_INSTALL OFF CACHE BOOL "" FORCE)
set(JANSSON_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(JANSSON_EXAMPLES OFF CACHE BOOL "" FORCE)
set(JANSSON_WITHOUT_TESTS ON CACHE BOOL "" FORCE)
add_subdirectory(${AT_ROOT}/libs/jansson/repo at-jansson EXCLUDE_FROM_ALL)
add_subdirectory(${AT_ROOT}/libs/libflic/repo at-libflic EXCLUDE_FROM_ALL)

add_subdirectory(${AT_ROOT}/src at-game)
