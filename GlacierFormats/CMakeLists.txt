cmake_minimum_required(VERSION 3.10)

project(GlacierFormats CXX)

add_subdirectory (${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/DirectXTex-master)

#LZ4 Compression
set(LZ4_LIB_NAME liblz4)
set(LZ4_INCLUED_PATH ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/lz4/include/)
set(LZ4_LIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/lz4/lib/)
set(LZ4_RUNTIME_PATH ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/lz4/runtime/)
include_directories(${LZ4_INCLUED_PATH})
link_directories(${LZ4_LIB_PATH})

#GLTF
set(GLTF_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glTF-SDK-master)
add_subdirectory (${GLTF_DIR})
include_directories(${GLTF_DIR}/GLTFSDK/Inc)
link_directories(${GLTF_DIR}/GLTFSDK/$(Configuration))

# add_subdirectory (${CMAKE_CURRENT_SOURCE_DIR}/tests)

#set(LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

file (GLOB_RECURSE SRCFILES "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
add_library(${PROJECT_NAME} ${SRCFILES})

target_link_libraries(${PROJECT_NAME} INTERFACE DirectXTex)
target_link_libraries(${PROJECT_NAME} INTERFACE ${LZ4_LIB_NAME})
target_link_libraries(${PROJECT_NAME} INTERFACE GLTFSDK)
target_link_directories(${PROJECT_NAME} INTERFACE ${LZ4_LIB_PATH})

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)

IF(MSVC)
    add_definitions(-DNOMINMAX)
ENDIF()

target_include_directories(GlacierFormats PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(GlacierFormats PUBLIC "${LZ4_INCLUED_PATH}")

# add_custom_command(target ${project_name} post_build
     # command ${cmake_command} -e copy_if_different
         # "${lz4_runtime_path}/${lz4_lib_name}.dll"
         # $<target_file_dir:${project_name}>)

# add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
     # COMMAND ${CMAKE_COMMAND} -e copy_if_different
		# "${LZ4_RUNTIME_PATH}/${LZ4_LIB_NAME}.dll"
		 # "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
		 # #"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$(CONFIGURATION)/${LZ4_LIB_NAME}.dll")
         # # "${CMAKE_BINARY_DIR}/${LZ4_LIB_NAME}.dll")