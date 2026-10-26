cmake_minimum_required(VERSION 3.11)

project(blobby-knockout VERSION 1.1.1)

# install into the home folder unless another prefix is given: no root needed, and the
# game shows up in the application menu
if(UNIX AND NOT APPLE AND CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	set(CMAKE_INSTALL_PREFIX "$ENV{HOME}/.local" CACHE PATH "Install path prefix" FORCE)
endif()
# the game data; not share/blobby, that is where the game keeps the settings of the user
set(BLOBBY_DATA_INSTALL_DIR share/blobby-knockout)

option(BUILD_TESTS "Build test programs" OFF)

# use FindBoost.cmake instead of BoostConfig.cmake
if(POLICY CMP0167)
  cmake_policy(SET CMP0167 OLD)
endif()

include(deps/sdl2.cmake)
include(deps/physfs.cmake)

add_subdirectory(data)
add_subdirectory(src)
add_subdirectory(deps)

add_subdirectory(linux)

if(BUILD_TESTS)
	enable_testing()
	add_subdirectory(test)
endif()

include(CPack)
