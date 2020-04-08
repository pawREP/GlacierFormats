#pragma once
#include <filesystem>
#include <string>

namespace GlacierFormats {

//#define GLACIER_DEBUG_PRINT(FMT, ...) printf("DEBUG(%s, %s, %d):\n\t" FMT "\n", __FILE__, __func__ , __LINE__, __VA_ARGS__);
#define GLACIER_DEBUG_PRINT(...) printf("test:", __VA_ARGS__);

}