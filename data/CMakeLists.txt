macro(add_zip_archive _dir _filetype)
	file(GLOB_RECURSE archive_src RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${_dir}/*.${_filetype})
	add_custom_command(
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${_dir}.zip
		COMMAND ${CMAKE_COMMAND} -E tar cf ${CMAKE_CURRENT_BINARY_DIR}/${_dir}.zip --format=zip ${archive_src}
		DEPENDS ${archive_src}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		VERBATIM
		)
	add_custom_target(${_dir}_zip ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${_dir}.zip)
#	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${_dir].zip DESTINATION /usr)
endmacro(add_zip_archive _dir _filetype)


add_zip_archive(gfx bmp)
add_zip_archive(sounds wav)
add_zip_archive(backgrounds bmp)
add_zip_archive(rules lua)
add_zip_archive(fonts ttf)

set(install_files
	${CMAKE_CURRENT_BINARY_DIR}/gfx.zip
	${CMAKE_CURRENT_BINARY_DIR}/sounds.zip
	${CMAKE_CURRENT_BINARY_DIR}/backgrounds.zip
	${CMAKE_CURRENT_BINARY_DIR}/rules.zip
	${CMAKE_CURRENT_BINARY_DIR}/fonts.zip
	${CMAKE_CURRENT_SOURCE_DIR}/fonts/OFL.txt
	${CMAKE_CURRENT_SOURCE_DIR}/api.lua
	${CMAKE_CURRENT_SOURCE_DIR}/rules_api.lua
	${CMAKE_CURRENT_SOURCE_DIR}/Icon.bmp
	${CMAKE_CURRENT_SOURCE_DIR}/config.xml)

if (WIN32 OR SWITCH)
	install(FILES ${install_files} DESTINATION data)
elseif (UNIX AND NOT APPLE)
	install(FILES ${install_files} DESTINATION ${BLOBBY_DATA_INSTALL_DIR})
endif (WIN32 OR SWITCH)

set(data_files ${install_files} PARENT_SCOPE)