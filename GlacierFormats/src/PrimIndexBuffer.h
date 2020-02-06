#pragma once
#include "PrimSerializationTypes.h"
#include <cinttypes>
#include <vector>

namespace GlacierFormats {

	class BinaryWriter;
	class BinaryReader;

	struct SPrimSubMesh;

	class IndexBuffer {
	private:
		std::vector<uint16_t> indices;

	public:
		IndexBuffer(const std::vector<uint16_t>& indices);
		IndexBuffer(BinaryReader* br, const SPrimSubMesh* prim_submesh);

		void serialize(BinaryWriter* bw);

		std::vector<unsigned short> getCanonicalForm() const;

		size_t size() const;
		std::vector<uint16_t>::iterator begin();
		std::vector<uint16_t>::iterator end();

		uint16_t& operator[](uint32_t idx);
	};

}