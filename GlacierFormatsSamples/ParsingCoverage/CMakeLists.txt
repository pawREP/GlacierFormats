cmake_minimum_required(VERSION 3.5)
project (GFSample_ParsingCoverage)

file(GLOB source_files
    "${CMAKE_CURRENT_LIST_DIR}/src/main.cpp"
)

add_executable(GFSample_ParsingCoverage ${source_files})

target_link_libraries(GFSample_ParsingCoverage
    GlacierFormats
)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)

