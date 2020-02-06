#include "IMesh.h"

namespace GlacierFormats {

	void IMesh::convert(IMesh* dst, const IMesh* src) {
		dst->setVertexBuffer(src->getVertexBuffer());
		dst->setIndexBuffer(src->getIndexBuffer());
		dst->setNormals(src->getNormals());
		dst->setUVs(src->getUVs());
		auto bone_weights = src->getBoneWeights();
		if (bone_weights.size())
			dst->setBoneWeight(bone_weights);
	}
}
