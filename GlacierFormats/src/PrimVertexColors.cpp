#include "PrimVertexColors.h"
#include "BinaryReader.hpp"
#include "BinaryWriter.hpp"

using namespace GlacierFormats;

	VertexColors::VertexColors(int vertex_count) noexcept {
		colors = std::vector<unsigned char>(4* vertex_count, 0xFF);
	}

	VertexColors::VertexColors(BinaryReader* br, const SPrimSubMesh* prim_submesh) {
		auto size = 4 * prim_submesh->num_vertex;
		colors.resize(size);
		br->read(colors.data(), size);
	}

	void VertexColors::serialize(BinaryWriter* bw) const {
		bw->write(colors.data(), colors.size());
	};
