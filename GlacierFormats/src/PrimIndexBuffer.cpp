#include <vector>
#include "PrimIndexBuffer.h"
#include "BinaryReader.hpp"
#include "BinaryWriter.hpp"
#include "PrimSerializationTypes.h"

using namespace GlacierFormats;

	IndexBuffer::IndexBuffer(const std::vector<uint16_t>& indices) : indices(indices) {
	}

	IndexBuffer::IndexBuffer(BinaryReader* br, const SPrimSubMesh* prim_submesh) {
		auto num_indices = prim_submesh->num_indices + prim_submesh->num_indices_ex;
		indices.resize(num_indices);
		br->read(indices.data(), indices.size());
	}

	void IndexBuffer::serialize(BinaryWriter* bw) {
		//TODO: Warn about large buffers and/or use index buffer extension. Needs more research, requirements for split index buffer unknown. 
		bw->write(indices.data(), indices.size());
		bw->align();
	}

	std::vector<unsigned short> IndexBuffer::getCanonicalForm() const {
		return indices;
	}

	size_t IndexBuffer::size() const {
		return indices.size();
	}

	std::vector<uint16_t>::iterator IndexBuffer::begin()
	{
		return indices.begin();
	}

	std::vector<uint16_t>::iterator IndexBuffer::end()
	{
		return indices.end();
	}

	uint16_t& IndexBuffer::operator[](uint32_t idx) {
		return indices[idx];
	}

