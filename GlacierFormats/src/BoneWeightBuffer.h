#pragma once

#include "Vector.h"
#include "PrimSerializationTypes.h"
#include "IMesh.h"

#include <vector>

namespace GlacierFormats {

	class BinaryReader;
	class BinaryWriter;

	//Collection of influences or a single vertex
	struct VertexWeights {
	public:

		//Max number of influences per vertex
		constexpr static unsigned int size = 6;

		Vec<float, size> weights;
		Vec<unsigned char, size> bone_ids;

		VertexWeights();
		VertexWeights(BinaryReader* br);
		void serialize(BinaryWriter* bw) const;

		bool operator==(const VertexWeights& other) const;

	private:
		constexpr static int base_weight_count = 4;
		constexpr static int extended_weight_count = size - base_weight_count;

	};

	class VertexWeightBuffer {

		std::vector<VertexWeights> weights;

	public:
		VertexWeightBuffer();
		VertexWeightBuffer(BinaryReader* br, const SPrimSubMesh* prim_submesh);
		void serialize(BinaryWriter* bw) const;

		std::vector<IMesh::VertexWeight> getCanonicalForm() const;
		void setFromCanonicalForm(int vertex_count, const std::vector<IMesh::VertexWeight>& weights);

		VertexWeights& operator[](uint32_t idx);
		const VertexWeights& operator[](uint32_t idx) const;
		bool operator==(const VertexWeightBuffer& other) const;
	};

}