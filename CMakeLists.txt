
cmake_minimum_required(VERSION 3.27)

project(
	CGames
	VERSION 0.0.1
	DESCRIPTION "A collection of classic games written in C over the course of college."
	LANGUAGES C
)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
	
	set(CMAKE_C_EXTENSIONS OFF)

	set_property(GLOBAL PROPERTY USE_FOLDERS ON)

	find_package(Doxygen)
	if(Doxygen_FOUND)
		add_subdirectory(docs)
	else()
		message(STATUS "Doxygen not found, not building docs")
	endif()

endif()

include(FetchContent)

add_subdirectory(libs)
add_subdirectory(apps)
