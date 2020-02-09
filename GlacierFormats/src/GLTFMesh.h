#pragma once 
#include "IMesh.h"

namespace GlacierFormats {

	class GLTFMesh : public IMesh {
	private:
		std::string name_;

		std::vector<unsigned short> index_buffer;
		std::vector<float> positions;
		std::vector<float> uvs;
		std::vector<float> normals;
		std::vector<float> tangents;
		std::vector<BoneWeight> weights;

	public:
		GLTFMesh();

		//IMesh
		[[nodiscard]] std::string name() const noexcept override final;
		[[nodiscard]] int materialId() const noexcept override final;
		[[nodiscard]] size_t vertexCount() const override final;
		[[nodiscard]] std::vector<float> getVertexBuffer() const override final;
		[[nodiscard]] std::vector<unsigned short> getIndexBuffer() const override final;
		[[nodiscard]] std::vector<float> getNormals() const override final;
		[[nodiscard]] std::vector<float> getTangents() const override final;
		[[nodiscard]] std::vector<float> getUVs() const override final;
		[[nodiscard]] std::vector<BoneWeight> getBoneWeights() const override final;

		void setVertexBuffer(const std::vector<float>&) override final;
		void setIndexBuffer(const std::vector<unsigned short>&) override final;
		void setNormals(const std::vector<float>&) override final;
		void setTangents(const std::vector<float>&) override final;
		void setUVs(const std::vector<float>&) override final;
		void setBoneWeight(const std::vector<BoneWeight>&) override final;
		void setName(const std::string&) override final;
	};
}