#include "GlacierTypes.h"

using namespace GlacierFormats;

std::string runtimeIdToHexString(RuntimeId id)
{
	char str_buf[0x10];
	sprintf_s(str_buf, "%I64X", id);
	return std::string(str_buf);
}
