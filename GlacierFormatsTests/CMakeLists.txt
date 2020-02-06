add_executable(GlacierFormatsTests
    test.cpp
    )
	
set_property(TARGET GlacierFormatsTests PROPERTY CXX_STANDARD 17)
set_property(TARGET GlacierFormatsTests PROPERTY CXX_STANDARD_REQUIRED ON)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
add_subdirectory(googletest)	

target_include_directories(GlacierFormatsTests PUBLIC ${CMAKE_SOURCE_DIR}/GlacierFormats/include)

target_link_libraries(GlacierFormatsTests PUBLIC gtest)
target_link_libraries(GlacierFormatsTests PUBLIC GlacierFormats)