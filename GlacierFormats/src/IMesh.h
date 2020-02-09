#pragma once
#include <vector>
#include <string>

namespace GlacierFormats {

	//Face-Vertex Mesh interface.
	//Render meshes contained in PRIM implement this interface and support 
	//conversion to and from other mesh formats that implement it.
	class IMesh	{
	public:
		struct BoneWeight {
			int vertex_id;
			int bone_id;
			float weight;

			bool operator<(const BoneWeight& other) { return this->weight < other.weight; }
		};

		static constexpr int vertex_size = 3;
		static constexpr int normal_size = 3;
		static constexpr int uv_size = 2;

		static void convert(IMesh* dst, const IMesh* src);

		[[nodiscard]] virtual std::string name() const noexcept = 0;
		[[nodiscard]] virtual int materialId() const noexcept = 0;
		[[nodiscard]] virtual size_t vertexCount() const = 0;
		[[nodiscard]] virtual std::vector<float> getVertexBuffer() const = 0;
		[[nodiscard]] virtual std::vector<unsigned short> getIndexBuffer() const = 0;
		[[nodiscard]] virtual std::vector<float> getNormals() const = 0;
		[[nodiscard]] virtual std::vector<float> getTangents() const = 0;
		[[nodiscard]] virtual std::vector<float> getUVs() const = 0;
		[[nodiscard]] virtual std::vector<BoneWeight> getBoneWeights() const = 0;

		//TODO: add move variants of all the setters
		virtual void setVertexBuffer(const std::vector<float>&) = 0;
		virtual void setIndexBuffer(const std::vector<unsigned short>&) = 0;
		virtual void setNormals(const std::vector<float>&) = 0;
		virtual void setTangents(const std::vector<float>&) = 0;
		virtual void setUVs(const std::vector<float>&) = 0;
		virtual void setBoneWeight(const std::vector<BoneWeight>&) = 0;
		virtual void setName(const std::string& name) = 0;
	};

}
