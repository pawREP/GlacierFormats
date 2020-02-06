#include "Crypto.h"

using namespace GlacierFormats;

void Crypto::rpkgXCrypt(char* buf, size_t len) {
	unsigned char key[] = { 0xdc, 0x45, 0xa6, 0x9c, 0xd3, 0x72, 0x4c, 0xab };
	for (int i = 0; i < len; i++) {
		buf[i] ^= key[i % sizeof(key)];
	}
}
