#include <vector>
#include "PrimVertexBuffer.h"
#include "PrimSerializationTypes.h"
#include "BinaryReader.hpp"
#include "BinaryWriter.hpp"
#include "Hash.h"
#include "IntegerRangeCompression.h"

using namespace GlacierFormats;

	//VertexBuffer::VertexBuffer() {}

	VertexBuffer::VertexBuffer(const std::vector<float>& positions) {
		is_high_res_buffer = false;
		const int floats_per_vert = 3;
		auto vertex_count = positions.size() / floats_per_vert;

		vertices.reserve(vertex_count);
		for (int i = 0; i < positions.size(); i += floats_per_vert) {
			vertices.emplace_back(positions[i + 0], positions[i + 1], positions[i + 2], 1.0f);
		}
	}

	VertexBuffer::VertexBuffer(BinaryReader* br, const SPrimObjectHeader* prim_object_header, const SPrimMesh* prim_mesh, const SPrimSubMesh* prim_submesh) {
		is_high_res_buffer = (
			((int)prim_object_header->property_flags & (int)SPrimObjectHeader::PROPERTY_FLAGS::HAS_HIRES_POSITIONS) == 
			(int)SPrimObjectHeader::PROPERTY_FLAGS::HAS_HIRES_POSITIONS
			);

		if (!is_high_res_buffer) {
			vertices.resize(prim_submesh->num_vertex);
			for (int i = 0; i < prim_submesh->num_vertex; ++i) {
				//There is an off-by-one error in IOI's compression code. The compressed shorts only range from -32767 to 32767.

				vertices[i].x() = IntegerRangeCompressor<short, float>::decompress(br->read<short>(), prim_mesh->pos_scale[0], prim_mesh->pos_bias[0]);
				vertices[i].y() = IntegerRangeCompressor<short, float>::decompress(br->read<short>(), prim_mesh->pos_scale[1], prim_mesh->pos_bias[1]);
				vertices[i].z() = IntegerRangeCompressor<short, float>::decompress(br->read<short>(), prim_mesh->pos_scale[2], prim_mesh->pos_bias[2]);
				vertices[i].w() = IntegerRangeCompressor<short, float>::decompress(br->read<short>(), prim_mesh->pos_scale[3], prim_mesh->pos_bias[3]);
			}
		}
		else {
			vertices.resize(prim_submesh->num_vertex);
			for (int i = 0; i < prim_submesh->num_vertex; ++i) {
				vertices[i].x() = br->read<float>();
				vertices[i].y() = br->read<float>();
				vertices[i].z() = br->read<float>();
			}
		}
	}

	std::vector<float> VertexBuffer::getCanonicalForm() const
	{
		constexpr int canonical_vertex_size = 3;

		std::vector<float> ret;
		ret.reserve(canonical_vertex_size * vertices.size());
		for (const auto& vert : vertices)
			for (int i = 0; i < canonical_vertex_size; ++i)
				ret.push_back(vert[i]);

		return ret;
	}

	void VertexBuffer::serialize(BinaryWriter* bw) {
		if (!is_high_res_buffer) {
			float scale[4];
			float bias[4];

			BoundingBox bb = BoundingBox(vertices);
			bb.getIntegerRangeCompressionParameters(scale, bias);

			//Only low res serialisation.
			for (const auto& vertex : vertices) {
				//old
				//auto x = static_cast<short>(std::roundf(32767.0 * (vertex[0] - bias[0]) / scale[0]));
				//auto y = static_cast<short>(std::roundf(32767.0 * (vertex[1] - bias[1]) / scale[1]));
				//auto z = static_cast<short>(std::roundf(32767.0 * (vertex[2] - bias[2]) / scale[2]));
				//auto w = static_cast<short>(std::roundf(32767.0));

				auto x = IntegerRangeCompressor<short,float>::compress(vertex[0], scale[0], bias[0]);
				auto y = IntegerRangeCompressor<short,float>::compress(vertex[1], scale[1], bias[1]);
				auto z = IntegerRangeCompressor<short,float>::compress(vertex[2], scale[2], bias[2]);
				short w = 0x7FFF;


				bw->write(x);
				bw->write(y);
				bw->write(z);
				bw->write(w);
			}
		}
		else {
			for (const auto& vertex : vertices) {
				bw->write(vertex.x());
				bw->write(vertex.y());
				bw->write(vertex.z());
			}
		}
	}

	std::vector<Vertex>::iterator VertexBuffer::begin() noexcept
	{
		return vertices.begin();
	}

	std::vector<Vertex>::iterator VertexBuffer::end() noexcept
	{
		return vertices.end();
	}

	std::vector<Vertex>::const_iterator VertexBuffer::begin() const noexcept
	{
		return vertices.begin();
	}

	std::vector<Vertex>::const_iterator VertexBuffer::end() const noexcept
	{
		return vertices.end();
	}

	size_t VertexBuffer::size() const noexcept {
		return vertices.size();
	}

	Vertex& VertexBuffer::operator[](uint32_t idx) {
		return vertices[idx];
	}

	const Vertex& VertexBuffer::operator[](uint32_t idx) const {
		return vertices[idx];
	}

	RecordKey VertexBuffer::recordKey() const {
		return RecordKey{ typeid(VertexBuffer), hash::fnv1a(vertices) };
	}

	BoundingBox<Vertex> VertexBuffer::getBoundingBox() const {
		return BoundingBox<Vertex>(vertices);
	}

