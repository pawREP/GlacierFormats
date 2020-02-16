#include <type_traits>
#include <functional>
#include "PrimRenderPrimitive.h"
#include "BinaryReader.hpp"
#include "PrimSerializationTypes.h"
#include "PrimBoundingBox.h"
#include "PrimRenderPrimitiveSerialization.h"
#include "PrimRenderPrimitiveBuilder.h"

using namespace GlacierFormats;

	ZRenderPrimitive::ZRenderPrimitive() {

	}

	ZRenderPrimitive::ZRenderPrimitive(ZRenderPrimitive&& src) {
		remnant = src.remnant;
		vertex_buffer = std::move(src.vertex_buffer);
		index_buffer = std::move(src.index_buffer);
		vertex_data = std::move(src.vertex_data);
		bone_weight_buffer = std::move(src.bone_weight_buffer);
		vertex_colors = std::move(src.vertex_colors);
		collision_data = std::move(src.collision_data);
		cloth_data = std::move(src.cloth_data);
		bone_info = std::move(src.bone_info);
		bone_indices = std::move(src.bone_indices);
	}

	uint32_t ZRenderPrimitive::serialize(BinaryWriter* bw) const {
		RenderPrimitiveSerializer serializer;
		return serializer.serialize(bw, this);
	}

	bool ZRenderPrimitive::isWeightedMesh() const {
		return bone_weight_buffer != nullptr;
	}


	[[nodiscard]] std::string ZRenderPrimitive::name() const noexcept {
		//TODO: Replace this with a real hash function or an actual name
		//TODO: Experiments show that some submeshes share the same vertex_buffer => same hash. Not good!
		size_t hash = 0;
		for (const auto& v : *vertex_buffer)
			for (const auto& c : v)
				hash ^= std::hash<float>{}(c);

		std::stringstream ss;
		ss << std::hex;
		ss << std::uppercase;
		ss << hash;
		return ss.str();

		//std::string extention("_mat_" + std::to_string(remnant.material_id) + "_lod_" + std::to_string(remnant.lod_mask));
		//return base_name + extention; 
	}

	[[nodiscard]] int ZRenderPrimitive::materialId() const noexcept
	{
		return remnant.material_id;
	}

	[[nodiscard]] size_t ZRenderPrimitive::vertexCount() const
	{
		return vertex_buffer->size();
	}

	[[nodiscard]] std::string ZRenderPrimitive::name(RuntimeId prim_id) const noexcept
	{
		std::string mesh_name = 
		mesh_name += "_mat_" + std::to_string(remnant.material_id) + "_lod_" + std::to_string(remnant.lod_mask);
		return mesh_name;
	}

	[[nodiscard]] std::vector<float> ZRenderPrimitive::getVertexBuffer() const
	{
		return vertex_buffer->getCanonicalForm();
	}

	[[nodiscard]] std::vector<unsigned short> ZRenderPrimitive::getIndexBuffer() const
	{
		return index_buffer->getCanonicalForm();
	}

	[[nodiscard]] std::vector<float> ZRenderPrimitive::getNormals() const
	{
		return vertex_data->getNormals();
	}

	std::vector<float> GlacierFormats::ZRenderPrimitive::getTangents() const {
		return vertex_data->getTangents();
	}

	[[nodiscard]] std::vector<float> ZRenderPrimitive::getUVs() const
	{
		return vertex_data->getUVs();
	}

	[[nodiscard]] std::vector<IMesh::BoneWeight> ZRenderPrimitive::getBoneWeights() const
	{
		if (!isWeightedMesh())
			return std::vector<IMesh::BoneWeight>();

		return bone_weight_buffer->getCanonicalForm();
	}

	void ZRenderPrimitive::setVertexBuffer(const std::vector<float>& vertex_buffer_) {
		vertex_buffer = std::make_unique<VertexBuffer>(vertex_buffer_);
	}

	void ZRenderPrimitive::setIndexBuffer(const std::vector<unsigned short>& index_buffer_) {
		index_buffer = std::make_unique<IndexBuffer>(index_buffer_);
	}

	void ZRenderPrimitive::setNormals(const std::vector<float>& normal_buffer_) {
		//TODO: Having some of the per vertex data in a separate structure like in the serialized PRIM
		//layout seems overly cumbersome and more prone to errors and corruption due to user error
		//Doesn't seem like it's worth the trouble and performance benefits. 
		//Refactor this and shift the complexity to (de)serialization.
		if (!vertex_data)
			vertex_data = std::make_unique<VertexDataBuffer>();
		vertex_data->setNormals(normal_buffer_);
	}

	void GlacierFormats::ZRenderPrimitive::setTangents(const std::vector<float>& tangents_) {
		if (!vertex_data)
			vertex_data = std::make_unique<VertexDataBuffer>();
		vertex_data->setTangents(tangents_);
	}

	void ZRenderPrimitive::setUVs(const std::vector<float>& uvs) {
		if (!vertex_data)
			vertex_data = std::make_unique<VertexDataBuffer>();
		vertex_data->setUVs(uvs);
	}

	void ZRenderPrimitive::setBoneWeight(const std::vector<BoneWeight>& weights) {
		if (!bone_weight_buffer)
			bone_weight_buffer = std::make_unique<BoneWeightBuffer>();
		bone_weight_buffer->setFromCanonicalForm(vertex_buffer->size(), weights);
	}

	void ZRenderPrimitive::setName(const std::string& name) {
		//Nothing to do here. name is derived via hashing of members.
	}


