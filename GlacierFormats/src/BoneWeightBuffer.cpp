#include "BoneWeightBuffer.h"
#include "BinaryReader.hpp"
#include "BinaryWriter.hpp"

using namespace GlacierFormats;

	VertexWeights::VertexWeights() {

	}

	VertexWeights::VertexWeights(BinaryReader* br) {
		unsigned char w_buf[base_weight_count];
		unsigned char id_buf[base_weight_count];

		//parse base weights
		br->read(w_buf, base_weight_count);
		br->read(id_buf, base_weight_count);

		for (int i = 0; i < base_weight_count; ++i)
			weights[i] = static_cast<float>(w_buf[i]) / 255.0f;

		for (int i = 0; i < base_weight_count; ++i)
			bone_ids[i] = id_buf[i];

		//parse weight extension
		br->read(w_buf, extended_weight_count);
		br->read(id_buf, extended_weight_count);

		for (int i = 0; i < extended_weight_count; ++i)
			weights[base_weight_count + i] = static_cast<float>(w_buf[i]) / 255.0f;

		for (int i = 0; i < extended_weight_count; ++i)
			bone_ids[base_weight_count + i] = id_buf[i];
	};

	void VertexWeights::serialize(BinaryWriter* bw) const {
		for (int i = 0; i < base_weight_count; ++i)
			bw->write(static_cast<unsigned char>(std::roundf(weights[i] * 255.0f)));

		for (int i = 0; i < base_weight_count; ++i)
			bw->write(bone_ids[i]);

		for (int i = 0; i < extended_weight_count; ++i)
			bw->write(static_cast<unsigned char>(std::roundf(weights[base_weight_count + i] * 255.0f)));

		for (int i = 0; i < extended_weight_count; ++i)
			bw->write(bone_ids[base_weight_count + i]);
	}

	bool GlacierFormats::VertexWeights::operator==(const VertexWeights& other) const {
		if (weights != other.weights)
			return false;
		if (bone_ids != other.bone_ids)
			return false;

		return true;
	}

	VertexWeightBuffer::VertexWeightBuffer() {

	}

	VertexWeightBuffer::VertexWeightBuffer(BinaryReader* br, const SPrimSubMesh* prim_submesh) {
		for (int vertex_id = 0; vertex_id < prim_submesh->num_vertex; ++vertex_id) {
			weights.emplace_back(br);
		}
	}

	void VertexWeightBuffer::serialize(BinaryWriter* bw) const {
		for (const auto& w : weights)
			w.serialize(bw);
	}

	std::vector<IMesh::VertexWeight> VertexWeightBuffer::getCanonicalForm() const
	{
		std::vector<IMesh::VertexWeight> ret;
		for (int i = 0; i < weights.size(); ++i) {
			const auto& bone_weight = weights[i];
			for (size_t j = 0; j < VertexWeights::size; ++j) {
				const auto weights = bone_weight.weights[j];
				const auto bone_id = bone_weight.bone_ids[j];
				if (bone_weight.weights[j] != 0.0f)
					ret.push_back({ i, bone_id, weights });
			}
		}
		return ret;
	}

	void VertexWeightBuffer::setFromCanonicalForm(int vertex_count, const std::vector<IMesh::VertexWeight>& weight_buffer) {
		weights.resize(vertex_count);
		for (auto& new_weight : weight_buffer) {
			auto& vertex_weights = weights[new_weight.vertex_id];
			auto slot_id = 0;
			while ((vertex_weights.weights[slot_id] != 0.f) && (slot_id != VertexWeights::size))
				++slot_id;

			if (slot_id == VertexWeights::size)
				throw std::runtime_error("Glacier render meshes only support 4 bone weights per vertex");

			vertex_weights.weights[slot_id] = new_weight.weights;
			vertex_weights.bone_ids[slot_id] = new_weight.bone_id;
		}

	}

	VertexWeights& VertexWeightBuffer::operator[](uint32_t idx) {
		return weights[idx];
	}

	const VertexWeights& VertexWeightBuffer::operator[](uint32_t idx) const {
		return weights[idx];
	}

	bool VertexWeightBuffer::operator==(const VertexWeightBuffer& other) const {
		if (weights.size() != other.weights.size())
			return false;

		for (int i = 0; i < weights.size(); ++i) {
			if (!((*this)[i] == other[i]))
				return false;
		}

		return true;
	}
