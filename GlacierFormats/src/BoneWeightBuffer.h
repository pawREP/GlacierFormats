#pragma once

#include "Vector.h"
#include "PrimSerializationTypes.h"
#include "IMesh.h"

#include <vector>

namespace GlacierFormats {

	class BinaryReader;
	class BinaryWriter;

	struct BoneWeight {//TODO: rename to vertex weights? Maybe rework this struct all together, IO is kind of ugly
	public:
		Vec<float, 4> weight;
		Vec<unsigned char, 4> bone_id;
		float c; //This term seems to be related to cloth simulation. Should be nulled if the cloth data buffer is removed from a native model, 
				//failing to do so results in some ugly vertex displacements.

		BoneWeight();
		BoneWeight(BinaryReader* br);
		void serialize(BinaryWriter* bw) const;
	};

	class BoneWeightBuffer
	{
		std::vector<BoneWeight> weights;

	public:
		BoneWeightBuffer();
		BoneWeightBuffer(BinaryReader* br, const SPrimSubMesh* prim_submesh);
		void serialize(BinaryWriter* bw) const;

		size_t size() const;
		auto begin() const;
		auto end() const;

		std::vector<IMesh::BoneWeight> getCanonicalForm() const;
		void setFromCanonicalForm(int vertex_count, const std::vector<IMesh::BoneWeight>& weights);

		BoneWeight& operator[](uint32_t idx);
		const BoneWeight& operator[](uint32_t idx) const;
	};

}