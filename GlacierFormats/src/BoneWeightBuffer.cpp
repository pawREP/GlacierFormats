#include "BoneWeightBuffer.h"
#include "BinaryReader.hpp"
#include "BinaryWriter.hpp"

using namespace GlacierFormats;

	BoneWeight::BoneWeight() {
		weight = Vec<float, 4>(0, 0, 0, 0);
		bone_id = Vec<unsigned char, 4>(0, 0, 0, 0);
		c = 0;
	}

	BoneWeight::BoneWeight(BinaryReader* br) {
		unsigned char we[4];

		br->read(we, 4);
		br->read(bone_id.data(), 4);
		c = br->read<float>();

		int sum = 0;
		for (int i = 0; i < 4; ++i)
			sum += we[i];

		for (int i = 0; i < 4; ++i)
			weight[i] = static_cast<float>(we[i]) / sum;
	};

	void BoneWeight::serialize(BinaryWriter* bw) const {
		for (int i = 0; i < 4; ++i)
			bw->write(static_cast<unsigned char>(weight[i] * 255.0f));

		for (int i = 0; i < 4; ++i)
			bw->write(bone_id[i]);

		bw->write(c);
	}

	BoneWeightBuffer::BoneWeightBuffer() {

	}

	BoneWeightBuffer::BoneWeightBuffer(BinaryReader* br, const SPrimSubMesh* prim_submesh) {
		for (int vertex_id = 0; vertex_id < prim_submesh->num_vertex; ++vertex_id) {
			weights.emplace_back(br);
		}
	}

	void BoneWeightBuffer::serialize(BinaryWriter* bw) const {
		for (const auto& w : weights)
			w.serialize(bw);
	}

	size_t BoneWeightBuffer::size() const
	{
		return weights.size();
	}

	auto BoneWeightBuffer::begin() const
	{
		return weights.begin();
	}

	auto BoneWeightBuffer::end() const
	{
		return weights.end();
	}

	std::vector<IMesh::BoneWeight> BoneWeightBuffer::getCanonicalForm() const
	{
		std::vector<IMesh::BoneWeight> ret;
		for (int i = 0; i < weights.size(); ++i) {
			const auto& bone_weight = weights[i];
			for (size_t j = 0; j < decltype(BoneWeight::weight)::size(); ++j) {
				const auto weight = bone_weight.weight[j];
				const auto bone_id = bone_weight.bone_id[j];
				if (bone_weight.weight[j] != 0.0f)
					ret.push_back({ i, bone_id, weight });
			}
		}
		return ret;
	}

	void BoneWeightBuffer::setFromCanonicalForm(int vertex_count, const std::vector<IMesh::BoneWeight>& weight_buffer) {
		weights.resize(vertex_count);
		for (auto& new_weight : weight_buffer) {
			auto& weight = weights[new_weight.vertex_id];
			auto slot_id = 0;
			while ((weight.weight[slot_id] != 0.f) && (slot_id != 4))
				++slot_id;

			if (slot_id == 4)
				throw std::runtime_error("Glacier render meshes only support 4 bone weights per vertex");

			weight.weight[slot_id] = new_weight.weight;
			weight.bone_id[slot_id] = new_weight.bone_id;
		}

	}

	BoneWeight& BoneWeightBuffer::operator[](uint32_t idx) {
		return weights[idx];
	}

	const BoneWeight& BoneWeightBuffer::operator[](uint32_t idx) const {
		return weights[idx];
	}