cmake_minimum_required (VERSION 2.8.2)
set (CMAKE_VERBOSE_MAKEFILE TRUE)
set (DEBUG OFF)

#------------------------------------------
# Setup
#------------------------------------------

if (DEBUG)
	set (CMAKE_BUILD_TYPE Debug)
endif()

add_definitions( -DSRC_DIR="${CMAKE_CURRENT_SOURCE_DIR}/src" )
if(APPLE)
	add_definitions( -DUSE_APPLE )
endif(APPLE)

project (particle_art C CXX)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	option(USE_GLEW "Compile with core arb instead of glew" OFF)
else()
	option(USE_GLEW "Compile with core arb instead of glew" ON)
endif()

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set (CMAKE_INSTALL_PREFIX "${${PROJECT_NAME}_SOURCE_DIR}/build/install" CACHE PATH "default install path" FORCE )
endif()

set (SOURCEFILES 
	${CMAKE_CURRENT_SOURCE_DIR}/src/art.cpp
	#${CMAKE_CURRENT_SOURCE_DIR}/src/bouncing_ball.cpp
	#${CMAKE_CURRENT_SOURCE_DIR}/src/water_fountain.cpp
	#${CMAKE_CURRENT_SOURCE_DIR}/src/fire.cpp
	#${CMAKE_CURRENT_SOURCE_DIR}/src/fireworks.cpp
)

set (HEADERFILES
	${CMAKE_CURRENT_SOURCE_DIR}/src/shader.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/helper.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/trimesh.hpp
)

source_group("Header Files" FILES ${HEADERFILES})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

#------------------------------------------
# Download dependencies
#------------------------------------------

# GLFW
set(glfw_checkout_Dir ${CMAKE_SOURCE_DIR}/lib/glfw)
make_directory(${glfw_checkout_Dir})
include(ExternalProject)

set(glfw_PREFIX "${glfw_checkout_Dir}")
set(glfw_INSTALL_DIR "${glfw_checkout_Dir}/install")

set(glfw_CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${glfw_INSTALL_DIR} -DUSE_MSVC_RUNTIME_LIBRARY_DLL=ON -DGLFW_BUILD_DOCS=OFF -DGLFW_BUILD_EXAMPLES=OFF -DGLFW_BUILD_TESTS=OFF )
set(glfw_DIR "${glfw_INSTALL_DIR}")

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	set(glfw_CMAKE_ARGS ${glfw_CMAKE_ARGS} -DCMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD="c++11" -DCMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY="libc++")
endif()

#   if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
#		set(GIT_EXECUTABLE "C:/Program Files (x86)/Git/bin/git.exe")
#   endif (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
   
ExternalProject_add(glfw
		PREFIX ${glfw_PREFIX}
# GIT_TAG gpu-affinity
        	GIT_REPOSITORY https://github.com/glfw/glfw
		INSTALL_DIR ${glfw_INSTALL_DIR}
		CMAKE_ARGS ${glfw_CMAKE_ARGS}
		UPDATE_COMMAND ""
)
 
set_property(TARGET glfw PROPERTY FOLDER "Dependencies")

set(GLFW_INCLUDE_DIR ${glfw_INSTALL_DIR}/include CACHE INTERNAL "Directory of GLFW header files")
if(MSVC)
	set(GLFW_LIBRARY ${glfw_INSTALL_DIR}/lib/glfw3.lib CACHE INTERNAL "GLFW lib file")
else()
	set(GLFW_LIBRARY ${glfw_INSTALL_DIR}/lib/libglfw3.a CACHE INTERNAL "GLFW lib file")
endif()	
include_directories(${GLFW_INCLUDE_DIR})


# OpenGL
find_package(OpenGL REQUIRED)
set(OPENGL_LIBRARIES ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY})
set(OPENGL_INCLUDE_DIRS ${OPENGL_INCLUDE_DIR})
include_directories(${OPENGL_INCLUDE_DIRS})


# Glew has some opengl extensions we'll make use of
if(USE_GLEW)

	set(glew_dir ${CMAKE_CURRENT_SOURCE_DIR}/lib/glew) # a place to put it
	make_directory(${glew_dir})
	set(glew_CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${glew_dir})
	ExternalProject_add(glew
		PREFIX ${glew_dir}
		GIT_REPOSITORY https://github.com/Perlmint/glew-cmake # We'll use perlmint's repo
		INSTALL_DIR ${glew_dir}
		CMAKE_ARGS ${glew_CMAKE_ARGS}
		UPDATE_COMMAND ""
	)

	find_package(GLEW REQUIRED)
	link_libraries(${GLEW_LIBRARIES}) 

	set(GLEW_INCLUDE_DIR ${glew_dir}/include CACHE INTERNAL "Directory of GLEW header files")
	if(MSVC)
		set(GLEW_LIBRARY $ENV{GLEW_HOME}/lib/Release/Win32/glew32.lib CACHE INTERNAL "GLEW lib file") 
	else()
		set(GLEW_LIBRARY ${glew_dir}/lib/libglew.a CACHE INTERNAL "GLEW lib file")
	endif()	
	include_directories(${GLEW_INCLUDE_DIR})
	set_property(TARGET glew PROPERTY FOLDER "Dependencies")

	add_definitions(-DUSE_GLEW)

endif()



#------------------------------------------
# Include Directories
#------------------------------------------


# Include Directories
include_directories (
  .
)

# Windows Section #
if (MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    # Tell MSVC to use main instead of WinMain for Windows subsystem executables
    set_target_properties(${WINDOWS_BINARIES} PROPERTIES LINK_FLAGS "/ENTRY:mainCRTStartup")
endif()

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++11")
	set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
	find_library(COCOA_LIB Cocoa)
    	find_library(CARBON_LIB Carbon)
	find_library(IOKIT_LIB IOKit)
	find_library(CORE_FOUNDATION_FRAMEWORK CoreFoundation)
    	find_library(CORE_VIDEO_FRAMEWORK CoreVideo)
	set(LIBS_ALL ${LIBS_ALL} ${COCOA_LIB} ${CARBON_LIB} ${IOKIT_LIB} ${CORE_FOUNDATION_FRAMEWORK} ${CORE_VIDEO_FRAMEWORK})
	message(STATUS "${CORE_VIDEO_FRAMEWORK}")
endif()

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	find_package(Threads)
	find_package(X11)
	set(LIBS_ALL ${LIBS_ALL} ${CMAKE_THREAD_LIBS_INIT} rt Xrandr Xxf86vm Xi Xcursor Xinerama m dl ${X11_LIBRARIES})
endif()

make_directory(${CMAKE_BINARY_DIR}/lib)
make_directory(${CMAKE_BINARY_DIR}/bin)
set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
foreach (CONF ${CMAKE_CONFIGURATION_TYPES})
	string (TOUPPER ${CONF} CONF)
	set (CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONF} ${CMAKE_BINARY_DIR}/bin)
	set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONF} ${CMAKE_BINARY_DIR}/lib)
	set (CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONF} ${CMAKE_BINARY_DIR}/lib)
endforeach(CONF CMAKE_CONFIGURATION_TYPES)

set(CMAKE_DEBUG_POSTFIX "d")
set(CMAKE_RELEASE_POSTFIX "")
set(CMAKE_RELWITHDEBINFO_POSTFIX "rd")
set(CMAKE_MINSIZEREL_POSTFIX "s")

#set the build postfix extension according to the current configuration
if (CMAKE_BUILD_TYPE MATCHES "Release")
	set(CMAKE_BUILD_POSTFIX "${CMAKE_RELEASE_POSTFIX}")
elseif (CMAKE_BUILD_TYPE MATCHES "MinSizeRel")
	set(CMAKE_BUILD_POSTFIX "${CMAKE_MINSIZEREL_POSTFIX}")
elseif (CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo")
	set(CMAKE_BUILD_POSTFIX "${CMAKE_RELWITHDEBINFO_POSTFIX}")
elseif (CMAKE_BUILD_TYPE MATCHES "Debug")
	set(CMAKE_BUILD_POSTFIX "${CMAKE_DEBUG_POSTFIX}")
else()
	set(CMAKE_BUILD_POSTFIX "")
endif()

# Build Target
add_executable ( ${PROJECT_NAME} ${HEADERFILES} ${SOURCEFILES} )
#add_execetuable( ${

if (MSVC)
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
endif()

if(USE_GLEW)
	target_link_libraries(${PROJECT_NAME} ${GLFW_LIBRARY} ${GLEW_LIBRARY} ${OPENGL_LIBRARIES} ${LIBS_ALL})
	add_dependencies( ${PROJECT_NAME} glfw glew)
else()
	target_link_libraries(${PROJECT_NAME} ${GLFW_LIBRARY} ${OPENGL_LIBRARIES} ${LIBS_ALL})
	add_dependencies( ${PROJECT_NAME} glfw)
endif()

if (DEBUG)
	#set_target_properties(${PROJECT_NAME} PROPERTIES ENABLE_EXPORTS TRUE)
endif()

#install(DIRECTORY ${PROJECT_NAME}/include/ DESTINATION include)

install( TARGETS ${PROJECT_NAME}
         LIBRARY DESTINATION lib
         ARCHIVE DESTINATION lib
         RUNTIME DESTINATION bin)

