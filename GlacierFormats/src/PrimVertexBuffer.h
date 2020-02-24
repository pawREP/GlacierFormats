#pragma once
#include <vector>
#include "Vector.h"
#include "PrimSerializationTypes.h"
#include "PrimBoundingBox.h"
#include "PrimReusableRecord.h"

namespace GlacierFormats {

	class BinaryReader;
	class BinaryWriter;

	using Vertex = Vec<float, 4>;

	//Holds vertex position data.
	//The vertex data in this class is represented as four floats per vertex where
	//the 4th float is 1.0f. The serialized fromat either consists of 3 floats or
	//a integer range compressed set of four signed shorts. The serialized format used
	//is indicated by the SPrimOnjectHeader::HAS_HIRES_POSITIONS flag. The float represenation
	//is used if the flag is set.

	class VertexBuffer : public ReusableRecord {
	private:
		std::vector<Vertex> vertices;
		bool is_high_res_buffer;

	public:
		VertexBuffer(const std::vector<float>& positions);
		VertexBuffer(BinaryReader* br, const SPrimObjectHeader* prim_object_header, const SPrimMesh* prim_mesh, const SPrimSubMesh* prim_submesh);

		[[nodiscard]] std::vector<float> getCanonicalForm() const;

		BoundingBox<Vertex> getBoundingBox() const;

		void serialize(BinaryWriter* br);

		size_t size() const noexcept;
		std::vector<Vertex>::iterator begin() noexcept;
		std::vector<Vertex>::iterator end() noexcept;
		std::vector<Vertex>::const_iterator begin() const noexcept;
		std::vector<Vertex>::const_iterator end() const noexcept;

		Vertex& operator[](uint32_t idx);
		const Vertex& operator[](uint32_t idx) const;

		RecordKey recordKey() const override final;
	};

}