#include "PrimClothData.h"
#include "BinaryReader.hpp"
#include "BinaryWriter.hpp"

using namespace GlacierFormats;

enum class ClothDataType {
	UNKNOWN,
	SMOLL,
	LARGE
};

ClothDataType guessSizeType(BinaryReader* br, int vertex_count, int size_hint) {
	auto base_pos = br->tell();
	auto size = br->read<uint16_t>();
	br->seek(base_pos);

	auto v0_size_guess = size;
	auto v1_size_guess = 0x14 * vertex_count;

	if (v0_size_guess > size_hint)
		return ClothDataType::LARGE;

	if (v1_size_guess > size_hint)
		return ClothDataType::SMOLL;

	auto v0_delta = size_hint - v0_size_guess;
	auto v1_delta = size_hint - v1_size_guess;

	if (v0_delta < v1_delta)
		return ClothDataType::SMOLL;
	return ClothDataType::LARGE;
}

ClothData::ClothData(BinaryReader* br, const SPrimSubMesh* prim_submesh, const SPrimMesh* prim_mesh) {

	bool is_small_buffer = (((int)prim_mesh->cloth_flags & (int)SPrimMesh::CLOTH_FLAGS::USE_SMOLL_CLOTH_BLOCK)) == (int)SPrimMesh::CLOTH_FLAGS::USE_SMOLL_CLOTH_BLOCK;
	if (is_small_buffer)
		data.resize(br->peek<uint16_t>());
	else
		data.resize(0x14 * prim_submesh->num_vertex);
	br->read(data.data(), data.size());
	br->align();
}

void ClothData::serialize(BinaryWriter* bw) {
	bw->write(data.data(), data.size());
}