cmake_minimum_required(VERSION 3.5)
project (GFSample_RenderAssetIO)

file(GLOB source_files
    "${CMAKE_CURRENT_LIST_DIR}/src/main.cpp"
)

add_executable(GFSample_RenderAssetIO ${source_files})

target_link_libraries(GFSample_RenderAssetIO
    GlacierFormats
)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)

