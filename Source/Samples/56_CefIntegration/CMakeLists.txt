#
# Copyright (c) 2008-2016 the Urho3D project.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

if (NOT URHO3D_NETWORK)
    return ()
endif ()

# Define target name
set (TARGET_NAME 56_CefIntegration)


#################################################
# Chromium Embedded Framework (CEF) Standard Binary Distribution
#**note** when using a different distribution, change the CEF_DISTRIBUTION value!
set(CEF_DISTRIBUTION cef_binary_3.2785.1466.g80e473e_windows32)

#################################################
# libcef_dll_wrapper
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/${CEF_DISTRIBUTION})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/${CEF_DISTRIBUTION}/include)
set(LIBS ${LIBS} libcef_dll_wrapper) 

########################
# turn off dbg check 
add_definitions(-DNDEBUG)

########################
# specify what we want here
set(CEF_BLD_TYPE Release)

if (WIN32)
    list (APPEND ABSOLUTE_PATH_LIBS ${CMAKE_CURRENT_SOURCE_DIR}/${CEF_DISTRIBUTION}/${CEF_BLD_TYPE}/libcef.lib )
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/${CEF_DISTRIBUTION}/${CEF_BLD_TYPE}/libcef.dll DESTINATION ${CMAKE_BINARY_DIR}/bin)
elseif (APPLE)
    list (APPEND ABSOLUTE_PATH_LIBS ${CMAKE_CURRENT_SOURCE_DIR}/CHANGE_MY_PATH_OSX/$<CONFIGURATION>/libcef.dylib )
elseif (ANDROID)
else ()
    # Linux
    list (APPEND ABSOLUTE_PATH_LIBS ${CMAKE_CURRENT_SOURCE_DIR}/CHANGE_MY_PATH_linux32/$<CONFIGURATION>/libcef.so )
endif ()

########################
# copy resource files
set(CEF_BINARY_FILES
	natives_blob.bin
	snapshot_blob.bin
)

set(CEF_RESOURCE_FILES
	cef.pak
	cef_100_percent.pak
	cef_200_percent.pak
	cef_extensions.pak
	devtools_resources.pak
	icudtl.dat
	locales
)

foreach(fileResource ${CEF_BINARY_FILES} )
    file(COPY ${CEF_DISTRIBUTION}/${CEF_BLD_TYPE}/${fileResource} DESTINATION ${CMAKE_BINARY_DIR}/bin)
endforeach()

foreach(fileResource ${CEF_RESOURCE_FILES} )
    file(COPY ${CEF_DISTRIBUTION}/Resources/${fileResource} DESTINATION ${CMAKE_BINARY_DIR}/bin)
endforeach()

#################################################
# cefsimple
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/${CEF_DISTRIBUTION}/cefsimple )

file (GLOB SIMPLE_CC_FILES ${CEF_DISTRIBUTION}/cefsimple/*.cc )
file (GLOB SIMPLE_H_FILES ${CEF_DISTRIBUTION}/cefsimple/*.h )
source_group ("Simple" FILES ${SIMPLE_CC_FILES} ${SIMPLE_H_FILES})

#################################################
# Define source files
define_source_files (EXTRA_H_FILES ${COMMON_SAMPLE_H_FILES})

set (SOURCE_FILES ${CPP_FILES} ${H_FILES} ${SIMPLE_CC_FILES} ${SIMPLE_H_FILES})

# Setup target with resource copying
setup_main_executable ()

# Setup test cases
setup_test ()


