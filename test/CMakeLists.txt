set(CMAKE_CXX_STANDARD 17)

set(SRC
	../src/BlobbyDebug.cpp    ../src/BlobbyDebug.h
	../src/FileWrite.cpp      ../src/FileWrite.h
	../src/FileRead.cpp       ../src/FileRead.h
	../src/FileSystem.cpp     ../src/FileSystem.h
	../src/File.cpp           ../src/File.h
	../src/PlayerInput.h      ../src/PlayerInput.cpp
	../src/DuelMatchState.cpp ../src/DuelMatchState.h
	../src/GameLogicState.cpp ../src/GameLogicState.h
	../src/PhysicState.cpp    ../src/PhysicState.h
	../src/DuelMatch.cpp      ../src/DuelMatch.h
	../src/Clock.cpp          ../src/Clock.h
	../src/PhysicWorld.cpp    ../src/PhysicWorld.h 
	../src/GameLogic.cpp      ../src/GameLogic.h
	../src/InputSource.cpp    ../src/InputSource.h
	../src/KnockoutQueue.cpp ../src/KnockoutQueue.h
	../src/PlayerProfile.cpp ../src/PlayerProfile.h
	../src/KnockoutTurns.cpp ../src/KnockoutTurns.h
	../src/WaitingPlayers.cpp ../src/WaitingPlayers.h
	../src/IScriptableComponent.cpp ../src/IScriptableComponent.h
	../src/UserConfig.cpp     ../src/UserConfig.h
	../src/Color.cpp          ../src/Color.h
)

find_package(Boost REQUIRED)
find_package(PhysFS REQUIRED)
find_package(SDL2 REQUIRED)

if ("${SDL2_LIBRARIES}" STREQUAL "")
	set(SDL2_LIBRARIES "SDL2::SDL2")
endif ("${SDL2_LIBRARIES}" STREQUAL "")

add_executable(blobbytest FileTest.cpp KnockoutTest.cpp ${SRC})

target_include_directories(blobbytest PRIVATE ${Boost_INCLUDE_DIR} ${PHYSFS_INCLUDE_DIR} ${SDL2_INCLUDE_DIRS} ../src)
target_link_libraries(blobbytest ${PHYSFS_LIBRARY} ${SDL2_LIBRARIES} lua tinyxml2)

add_test(NAME blobbytest COMMAND blobbytest WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
