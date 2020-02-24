#include "PrimVertexDataBuffer.h"
#include "BinaryReader.hpp"
#include "BinaryWriter.hpp"
#include "PrimBoundingBox.h"
#include "Util.h"
#include "IntegerRangeCompression.h"

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
			//TODO: Read complete blocks for performance

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

			uvs[i].x() = IntegerRangeCompressor<short, float>::decompress(br->read<short>(), prim_mesh->uv_scale[0], prim_mesh->uv_bias[0]);
			uvs[i].y() = IntegerRangeCompressor<short, float>::decompress(br->read<short>(), prim_mesh->uv_scale[1], prim_mesh->uv_bias[1]);
		}
	}

	void VertexDataBuffer::serialize(BinaryWriter* bw) {
		//calc uv scale and bias
		float uv_scale[2];
		float uv_bias[2];
		BoundingBox bb = BoundingBox(uvs);
		bb.getIntegerRangeCompressionParameters(uv_scale, uv_bias);

		for (size_t i = 0; i < normals.size(); ++i) {

			const auto& normal = normals[i];
			const auto& tangent = tangents[i];

			bw->write(Compress8BitFloat(normal.x()));
			bw->write(Compress8BitFloat(normal.y()));
			bw->write(Compress8BitFloat(normal.z()));
			bw->write(Compress8BitFloat(normal.w()));

			
			bw->write(Compress8BitFloat(tangent.x()));
			bw->write(Compress8BitFloat(tangent.y()));
			bw->write(Compress8BitFloat(tangent.z()));
			bw->write(Compress8BitFloat(tangent.w()));

			if (bitangents.size()) {
				bw->write(Compress8BitFloat(bitangents[i].x()));
				bw->write(Compress8BitFloat(bitangents[i].y()));
				bw->write(Compress8BitFloat(bitangents[i].z()));
				bw->write(Compress8BitFloat(0.f));
			}
			else {
				auto bitangent = cross(normal.xyz(), tangent.xyz());
				bw->write(Compress8BitFloat(bitangent.x()));
				bw->write(Compress8BitFloat(bitangent.y()));
				bw->write(Compress8BitFloat(bitangent.z()));
				bw->write(Compress8BitFloat(0.f));
			}

			bw->write(IntegerRangeCompressor<short, float>::compress(uvs[i].x(), uv_scale[0], uv_bias[0]));
			bw->write(IntegerRangeCompressor<short, float>::compress(uvs[i].y(), uv_scale[1], uv_bias[1]));
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

	std::vector<float> GlacierFormats::VertexDataBuffer::getTangents() const {
		std::vector<float> ret;

		const int tangent_size = 4;
		auto f_cnt = tangents.size() * tangent_size;
		ret.resize(f_cnt);
		memcpy_s(ret.data(), f_cnt * sizeof(float), tangents.data(), f_cnt * sizeof(float));
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
	
	//Set tangents from vector of floats. 4 floats per tangent.
	void VertexDataBuffer::setTangents(const std::vector<float>& tangent_buffer) {
		const int tangent_size = 4;
		auto tangent_count = tangent_buffer.size() / tangent_size;

		tangents.resize(tangent_count);
		auto tangent_data_size = tangent_size * tangent_count * sizeof(float);
		memcpy_s(tangents.data(), tangent_data_size, tangent_buffer.data(), tangent_data_size);
	}

	void VertexDataBuffer::setUVs(const std::vector<float>& uv_buffer) {
		const int uv_size = 2;
		auto uv_count = uv_buffer.size() / uv_size;

		uvs.resize(uv_count);
		assert(vectorSizeInBytes(uvs) == vectorSizeInBytes(uv_buffer));
		memcpy_s(uvs.data(), vectorSizeInBytes(uvs), uv_buffer.data(), vectorSizeInBytes(uv_buffer));
	}

