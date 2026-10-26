set(APP_NAME "Blobby Volley 2")
set(APP_AUTHOR "")

set(CMAKE_CXX_STANDARD 17)

if (MSVC)
	add_compile_options(/utf-8)
	# When building for MSVC with CMake,
	# the compiler warning flags (like /W3) are added by default.
else()
	add_compile_options(-Wall)
endif()

include_directories(. ../deps/stb)

set(blobby_SRC
	BlobbyApp.cpp BlobbyApp.h
	BlobbyDebug.cpp BlobbyDebug.h
	Clock.cpp Clock.h
	Color.cpp Color.h
	DuelMatch.cpp DuelMatch.h
	DuelMatchState.cpp DuelMatchState.h
	File.cpp File.h
	FileRead.cpp FileRead.h
	FileSystem.cpp FileSystem.h
	FileWrite.cpp FileWrite.h
	Font.cpp Font.h
	GameLogic.cpp GameLogic.h
	GameLogicState.cpp GameLogicState.h
	Global.h
	IMGUI.cpp IMGUI.h
	InputDevice.h
	InputManager.cpp InputManager.h
	InputSource.cpp InputSource.h
	IScriptableComponent.cpp IScriptableComponent.h
	KnockoutQueue.cpp KnockoutQueue.h
	KnockoutTurns.cpp KnockoutTurns.h
	LocalInputSource.cpp LocalInputSource.h
	LocalPlayers.cpp LocalPlayers.h
	PhysicState.cpp PhysicState.h
	PhysicWorld.cpp PhysicWorld.h
	PlayerInput.cpp PlayerInput.h
	PlayerProfile.cpp PlayerProfile.h
	RenderManager.cpp RenderManager.h
	SoundManager.cpp SoundManager.h
	SquishAnimation.h
	StbImplementation.cpp
	SpeedController.cpp SpeedController.h
	SwitchableInputSource.cpp SwitchableInputSource.h
	Theme.h
	UserConfig.cpp UserConfig.h
	Vector.h
	WaitingPlayers.cpp WaitingPlayers.h
	input_device/GamepadInput.cpp
	input_device/KeyboardInput.cpp
	state/KnockoutGameState.cpp state/KnockoutGameState.h
	state/KnockoutSetupState.cpp state/KnockoutSetupState.h
	state/State.cpp state/State.h
	)

find_package(Boost REQUIRED)

if (UNIX)
	add_definitions("-DBLOBBY_DATA_DIR=\"${CMAKE_INSTALL_PREFIX}/${BLOBBY_DATA_INSTALL_DIR}\"")
endif()

set(BLOBBY_COMMON_LIBS lua::lua tinyxml2::tinyxml2 PhysFS::PhysFS SDL2::SDL2main SDL2::SDL2 Boost::boost)

# the stb libraries are not written for our warning level
set_source_files_properties(StbImplementation.cpp PROPERTIES COMPILE_FLAGS -w)

add_executable(blobby main.cpp ${blobby_SRC})
target_link_libraries(blobby ${BLOBBY_COMMON_LIBS})

install(TARGETS blobby DESTINATION bin)

# Header Checks
# Create a test target, consisting of one cpp file for each header, where the cpp file does nothing but
# include its header. If this compiles, then all our headers are self-contained.

# gather up the header files
file(GLOB_RECURSE HEADERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.h)

# create a new target for this test. We use a static library (instead of an executable), because then
# we don't have to worry about things actually linking.
add_library(test-compile-headers STATIC EXCLUDE_FROM_ALL)

foreach(HEADER ${HEADERS})
	# construct a new file name based on the headers name
	string(MAKE_C_IDENTIFIER "${HEADER}" HEADER_TEST_FILE)
	set(TEST_COMPILE_FILE "${CMAKE_CURRENT_BINARY_DIR}/test_compile/${HEADER_TEST_FILE}.cpp")

	# create the test file and add to the target sources
	configure_file(test_compile.cpp.in ${TEST_COMPILE_FILE} @ONLY)
	target_sources(test-compile-headers PUBLIC ${TEST_COMPILE_FILE})
endforeach()

target_link_libraries(test-compile-headers PRIVATE ${BLOBBY_COMMON_LIBS})
