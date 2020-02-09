#pragma once
#include <vector>
#include "Vector.h"
#include "PrimSerializationTypes.h"

namespace GlacierFormats {

	class BinaryReader;
	class BinaryWriter;

	using Normal = Vec<float, 4>;
	using Tangent = Vec<float, 4>;
	using Bitangent = Vec<float, 4>;
	using UV = Vec<float, 2>;

	class VertexDataBuffer
	{
	public:
		std::vector<Normal> normals;
		std::vector<Tangent> tangents;
		std::vector<Bitangent> bitangents;
		std::vector<UV> uvs;

		VertexDataBuffer();
		VertexDataBuffer(BinaryReader* br, const SPrimMesh* prim_mesh, const SPrimSubMesh* prim_submesh);
		void serialize(BinaryWriter* bw, float scale[2], float bias[2]);

		std::vector<float> getNormals() const;
		std::vector<float> getTangents() const;
		std::vector<float> getUVs() const;

		void setNormals(const std::vector<float>&);
		void setTangents(const std::vector<float>&);
		void setUVs(const std::vector<float>&);
	};
}