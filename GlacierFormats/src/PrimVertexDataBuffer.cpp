#include "PrimVertexDataBuffer.h"
#include "BinaryReader.hpp"
#include "BinaryWriter.hpp"
#include "PrimBoundingBox.h"
#include "Util.h"

using namespace GlacierFormats;

namespace {

	inline float Decompress8BitFloat(uint8_t b) {
		return 2.0f * static_cast<float>(b) / 255.0f - 1.0f;
	}

	inline uint8_t Compress8BitFloat(float f) {
		return static_cast<uint8_t>(std::round((f + 1.0) / 2.0 * 255.0));
	}

	[[noreturn]] void PerfectReserializationExperiment(BinaryReader* br, int vertex_count) {
		std::vector<Vec<uint8_t, 4>> original_byte_normals(vertex_count);
		for (auto& normal : original_byte_normals) {
			for (auto& val : normal)
				val = br->read<uint8_t>();
		}

		std::vector<Vec<float, 4>> float_normals(vertex_count);
		for (int i = 0; i < float_normals.size(); ++i) {
			for (int j = 0; j < 4; ++j) {
				float_normals[i][j] = Decompress8BitFloat(original_byte_normals[i][j]);
			}
		}

		std::vector<Vec<uint8_t, 4>> new_byte_normals(vertex_count);
		for (int i = 0; i < new_byte_normals.size(); ++i) {
			for (int j = 0; j < 4; ++j) {
				new_byte_normals[i][j] = Compress8BitFloat(float_normals[i][j]);
			}
		}

		//Test equality
		for (int i = 0; i < original_byte_normals.size(); ++i) {
			for (int j = 0; j < 4; ++j) {
				auto ov = original_byte_normals[i][j];
				auto nv = new_byte_normals[i][j];
				assert(ov == nv);
			}
		}

		throw;
	}
}


	VertexDataBuffer::VertexDataBuffer() {
	}

	VertexDataBuffer::VertexDataBuffer(BinaryReader* br, const SPrimMesh* prim_mesh, const SPrimSubMesh* prim_submesh) {
		normals.resize(prim_submesh->num_vertex);
		tangents.resize(prim_submesh->num_vertex);
		bitangents.resize(prim_submesh->num_vertex);
		uvs.resize(prim_submesh->num_vertex);

		//PerfectReserializationExperiment(br, prim_submesh->num_vertex);
		//Experiment(br, prim_submesh->num_vertex);

		for (size_t i = 0; i < prim_submesh->num_vertex; ++i) {


			//Normal
			//TODO: Consider switching to Vec<float, 3> normals, 4th term likely always .0f. Do scan of full repo to confirm. Would simplify mesh import a bit.
			normals[i].x() = Decompress8BitFloat(br->read<uint8_t>());
			normals[i].y() = Decompress8BitFloat(br->read<uint8_t>());
			normals[i].z() = Decompress8BitFloat(br->read<uint8_t>());
			normals[i].w() = Decompress8BitFloat(br->read<uint8_t>());

			tangents[i].x() = Decompress8BitFloat(br->read<uint8_t>());
			tangents[i].y() = Decompress8BitFloat(br->read<uint8_t>());
			tangents[i].z() = Decompress8BitFloat(br->read<uint8_t>());
			tangents[i].w() = Decompress8BitFloat(br->read<uint8_t>());

			bitangents[i].x() = Decompress8BitFloat(br->read<uint8_t>());
			bitangents[i].y() = Decompress8BitFloat(br->read<uint8_t>());
			bitangents[i].z() = Decompress8BitFloat(br->read<uint8_t>());
			bitangents[i].w() = Decompress8BitFloat(br->read<uint8_t>());

			uvs[i].x() = static_cast<double>(br->read<short>())* prim_mesh->uv_scale[0] / 32767.0 + prim_mesh->uv_bias[0];
			uvs[i].y() = static_cast<double>(br->read<short>())* prim_mesh->uv_scale[1] / 32767.0 + prim_mesh->uv_bias[1];
		}
	}

	void VertexDataBuffer::serialize(BinaryWriter* bw, float uv_scale[2], float uv_bias[2]) {
		//calc uv scale and bias
		BoundingBox bb = BoundingBox(uvs);
		bb.getIntegerRangeCompressionParameters(uv_scale, uv_bias);

		for (size_t i = 0; i < normals.size(); ++i) {

			bw->write(Compress8BitFloat(normals[i].x()));
			bw->write(Compress8BitFloat(normals[i].y()));
			bw->write(Compress8BitFloat(normals[i].z()));
			bw->write(Compress8BitFloat(normals[i].w()));

			//TODO: Implement logic to calculate tangents and bitangents. Import routines might not initilize them.
			bw->write(Compress8BitFloat(tangents[i].x()));
			bw->write(Compress8BitFloat(tangents[i].y()));
			bw->write(Compress8BitFloat(tangents[i].z()));
			bw->write(Compress8BitFloat(tangents[i].w()));

			bw->write(Compress8BitFloat(bitangents[i].x()));
			bw->write(Compress8BitFloat(bitangents[i].y()));
			bw->write(Compress8BitFloat(bitangents[i].z()));
			bw->write(Compress8BitFloat(bitangents[i].w()));

			//UV coordinates
			bw->write(static_cast<short>(std::roundf(32767.0 * (uvs[i].x() - uv_bias[0]) / uv_scale[0])));
			bw->write(static_cast<short>(std::roundf(32767.0 * (uvs[i].y() - uv_bias[1]) / uv_scale[1])));
		}
	}

	std::vector<float> VertexDataBuffer::getNormals() const
	{
		constexpr int canonical_normal_size = 3;

		std::vector<float> ret;
		ret.reserve(canonical_normal_size * normals.size());
		for (const auto& normal : normals)
			for (int i = 0; i < canonical_normal_size; ++i)
				ret.push_back(normal[i]);
		return ret;
	}

	std::vector<float> VertexDataBuffer::getUVs() const
	{
		constexpr int canonical_uv_size = 2;
		static_assert(sizeof(UV) == canonical_uv_size * sizeof(float));
		std::vector<float> ret(canonical_uv_size * uvs.size());
		memcpy_s(ret.data(), sizeof(decltype(ret)::value_type) * ret.size(), uvs.data(), uvs.size() * sizeof(decltype(uvs)::value_type));

		//invert y coord;
		for (int i = 1; i < ret.size(); i += 2)
			ret[i] *= -1.0f;
		return ret;
	}

	void VertexDataBuffer::setNormals(const std::vector<float>& normal_buffer) {
		const int normal_size = 3;
		auto normal_count = normal_buffer.size() / normal_size;

		normals = decltype(normals)();
		normals.reserve(normal_count);
		//assert(vectorSizeInBytes(normals) == vectorSizeInBytes(normal_buffer));
		for (int i = 0; i < normal_count; ++i) {
			normals.emplace_back(normal_buffer[3 * i + 0], normal_buffer[3 * i + 1], normal_buffer[3 * i + 2], 0);
		}
	}

	//TODO: Implement logic so bitangent is automatically calculated once normals and tangents are set.
	void VertexDataBuffer::setTangents(const std::vector<float>& tangent_buffer) {
		const int tangent_size = 3;
		auto tangent_count = tangent_buffer.size() / tangent_size;

		tangents = decltype(tangents)();
		tangents.reserve(tangent_count);
		for (int i = 0; i < tangent_count; ++i) {
			tangents.emplace_back(tangent_buffer[3 * i + 0], tangent_buffer[3 * i + 1], tangent_buffer[3 * i + 2], 0);
		}
	}

	void VertexDataBuffer::setUVs(const std::vector<float>& uv_buffer) {
		const int uv_size = 2;
		auto uv_count = uv_buffer.size() / uv_size;

		uvs.resize(uv_count);
		assert(vectorSizeInBytes(uvs) == vectorSizeInBytes(uv_buffer));
		memcpy_s(uvs.data(), vectorSizeInBytes(uvs), uv_buffer.data(), vectorSizeInBytes(uv_buffer));
	}

