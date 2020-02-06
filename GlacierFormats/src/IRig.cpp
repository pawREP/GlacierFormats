#include "IRig.h"

using namespace GlacierFormats;

void IRig::convert(IRig* dst, const IRig* src) {
	dst->setFromCanonicalBoneList(src->getCanonicalBoneList());
}
