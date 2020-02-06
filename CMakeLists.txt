cmake_minimum_required(VERSION 3.10)

project(GlacierFormats CXX)

option(GLACIERFORMATS_ENABLE_TESTS "Enable tests for GlacierForamts" ON)
option(GLACIERFORMATS_ENABLE_SAMPLES "Enable samples for GlacierForamts" OFF)

add_subdirectory (${CMAKE_CURRENT_SOURCE_DIR}/GlacierFormats)

if(GLACIERFORMATS_ENABLE_TESTS)
	add_subdirectory (${CMAKE_CURRENT_SOURCE_DIR}/GlacierFormatsTests)	
endif()

if(GLACIERFORMATS_ENABLE_SAMPLES)
	add_subdirectory (${CMAKE_CURRENT_SOURCE_DIR}/GlacierFormatsSamples)	
endif()
