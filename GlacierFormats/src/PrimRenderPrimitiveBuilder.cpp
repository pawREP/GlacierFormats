#include "PrimRenderPrimitiveBuilder.h"
#include "PrimRenderPrimitive.h"

using namespace GlacierFormats;

	//Test validity and completeness of render mesh held by builder.
	//Should be called before calling build().
	//TODO: This function is very bare bone atm. Doe only test very basic requirements.
	void ZRenderPrimitiveBuilder::validate() const {
		//vertexbuffer, indexbuffer, and vertex data are a minimum requirement for a valid mesh
		if (!prim->vertex_buffer)
			throw std::runtime_error("RenderMesh validation failed: Vertex read_buffer missing");
		if (!prim->index_buffer)
			throw std::runtime_error("RenderMesh validation failed: Index read_buffer missing");
		if (!prim->vertex_data)
			throw std::runtime_error("RenderMesh validation failed: Per vertex data read_buffer missing");

		const auto vertex_count = prim->vertexCount();
		if ((prim->vertex_data->normals.size() != vertex_count) || (prim->vertex_data->uvs.size() != vertex_count))
			throw std::runtime_error("RenderMesh validation failed: Invalid per vertex data read_buffer size");

	}

	ZRenderPrimitiveBuilder::ZRenderPrimitiveBuilder() {
		//Can't use make_unique because of friend relationship
		prim = std::unique_ptr<ZRenderPrimitive>(new ZRenderPrimitive());
	}

	ZRenderPrimitiveBuilder::ZRenderPrimitiveBuilder(std::unique_ptr<ZRenderPrimitive> prim) : prim(std::move(prim)) {
	}

	void ZRenderPrimitiveBuilder::initilizeFromIMesh(const IMesh* mesh) {
		IMesh::convert(prim.get(), mesh);
	}

	void ZRenderPrimitiveBuilder::setVertexBuffer(const std::vector<float>& vertices) {
		prim->setVertexBuffer(vertices);
	}

	void ZRenderPrimitiveBuilder::setIndexBuffer(const std::vector<unsigned short>& indices) {
		prim->setIndexBuffer(indices);
	}

	void ZRenderPrimitiveBuilder::setUvBuffer(const std::vector<float>& uvs) {
		prim->vertex_data->setUVs(uvs);
	}

	void ZRenderPrimitiveBuilder::setNormalBuffer(const std::vector<float>& normals) {
		prim->vertex_data->setNormals(normals);
	}

	void ZRenderPrimitiveBuilder::setTangentBuffer(const std::vector<float>& tangents) {
		prim->vertex_data->setNormals(tangents);
	}

	void ZRenderPrimitiveBuilder::setVertexBuffer(std::unique_ptr<VertexBuffer> vertex_buffer) {
		prim->vertex_buffer = std::move(vertex_buffer);
	}

	void ZRenderPrimitiveBuilder::setIndexBuffer(std::unique_ptr<IndexBuffer> index_buffer) {
		prim->index_buffer = std::move(index_buffer);
	}

	void ZRenderPrimitiveBuilder::setVertexDataBuffer(std::unique_ptr<VertexDataBuffer> vertex_data_buffer) {
		prim->vertex_data = std::move(vertex_data_buffer);
	}

	void ZRenderPrimitiveBuilder::setCollisionBuffer(std::unique_ptr<CollisionData> collision) {
		prim->collision_data = std::move(collision);
	}

	void ZRenderPrimitiveBuilder::setBoneWeightBuffer(std::unique_ptr<BoneWeightBuffer> boneweight_buffer) {
		prim->bone_weight_buffer = std::move(boneweight_buffer);
	}

	void ZRenderPrimitiveBuilder::setBoneFlags(std::unique_ptr<VertexColors> vertex_colors) {
		prim->vertex_colors = std::move(vertex_colors);
	}

	void ZRenderPrimitiveBuilder::setClothData(std::unique_ptr<ClothData> cloth_data) {
		prim->cloth_data = std::move(cloth_data);
	}

	void ZRenderPrimitiveBuilder::setBoneInfo(std::unique_ptr<BoneIndices> bone_indices) {
		prim->bone_indices = std::move(bone_indices);
	}

	void ZRenderPrimitiveBuilder::setMUnkTabl2(std::unique_ptr<MUnkTabl2> m2) {
		prim->m_unk_tabl2 = std::move(m2);
	}

	void ZRenderPrimitiveBuilder::setPropertyFlags(SPrimObject::PROPERTY_FLAGS flags) {
		prim->remnant.submesh_properties = flags;
	}

	void ZRenderPrimitiveBuilder::setLodMask(unsigned char lod_mask) {
		prim->remnant.lod_mask = lod_mask;
	}

	void ZRenderPrimitiveBuilder::setWireColor(unsigned int wire_color)
	{
		prim->remnant.wire_color = wire_color;
	}

	void ZRenderPrimitiveBuilder::setVariantId(char variant_id)
	{
		prim->remnant.variant_id = variant_id;
	}

	void ZRenderPrimitiveBuilder::setMaterialId(short material_id)
	{
		prim->remnant.material_id = material_id;
	}


	std::unique_ptr<ZRenderPrimitive> ZRenderPrimitiveBuilder::build() {
		validate();
		return std::move(prim);
	}

