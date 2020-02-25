#include <filesystem>
#include <assert.h>
#include "GLTFExporter.h"
#include "GLTFSDK/BufferBuilder.h"
#include "GLTFSDK/Serialize.h"
#include "GLTFSDK/IStreamWriter.h"
#include "GLTFSDK/GLTFResourceWriter.h"
#include "GLTFSDK/GLBResourceWriter.h"
#include "GLTFSDK/ExtensionsKHR.h"
#include "GLTFSDK/Validation.h"
#include "GLTFSDK/AnimationUtils.h"

using namespace Microsoft::glTF;
using namespace GlacierFormats;
using namespace GlacierFormats::Export;

//TODO: Plenty of cleanup and factoring required here

//TODO: Get this function out of here
bool InvertMatrix(const float m[16], float invOut[16])
{
    float inv[16], det;
    int i;

    inv[0] = m[5] * m[10] * m[15] -
        m[5] * m[11] * m[14] -
        m[9] * m[6] * m[15] +
        m[9] * m[7] * m[14] +
        m[13] * m[6] * m[11] -
        m[13] * m[7] * m[10];

    inv[4] = -m[4] * m[10] * m[15] +
        m[4] * m[11] * m[14] +
        m[8] * m[6] * m[15] -
        m[8] * m[7] * m[14] -
        m[12] * m[6] * m[11] +
        m[12] * m[7] * m[10];

    inv[8] = m[4] * m[9] * m[15] -
        m[4] * m[11] * m[13] -
        m[8] * m[5] * m[15] +
        m[8] * m[7] * m[13] +
        m[12] * m[5] * m[11] -
        m[12] * m[7] * m[9];

    inv[12] = -m[4] * m[9] * m[14] +
        m[4] * m[10] * m[13] +
        m[8] * m[5] * m[14] -
        m[8] * m[6] * m[13] -
        m[12] * m[5] * m[10] +
        m[12] * m[6] * m[9];

    inv[1] = -m[1] * m[10] * m[15] +
        m[1] * m[11] * m[14] +
        m[9] * m[2] * m[15] -
        m[9] * m[3] * m[14] -
        m[13] * m[2] * m[11] +
        m[13] * m[3] * m[10];

    inv[5] = m[0] * m[10] * m[15] -
        m[0] * m[11] * m[14] -
        m[8] * m[2] * m[15] +
        m[8] * m[3] * m[14] +
        m[12] * m[2] * m[11] -
        m[12] * m[3] * m[10];

    inv[9] = -m[0] * m[9] * m[15] +
        m[0] * m[11] * m[13] +
        m[8] * m[1] * m[15] -
        m[8] * m[3] * m[13] -
        m[12] * m[1] * m[11] +
        m[12] * m[3] * m[9];

    inv[13] = m[0] * m[9] * m[14] -
        m[0] * m[10] * m[13] -
        m[8] * m[1] * m[14] +
        m[8] * m[2] * m[13] +
        m[12] * m[1] * m[10] -
        m[12] * m[2] * m[9];

    inv[2] = m[1] * m[6] * m[15] -
        m[1] * m[7] * m[14] -
        m[5] * m[2] * m[15] +
        m[5] * m[3] * m[14] +
        m[13] * m[2] * m[7] -
        m[13] * m[3] * m[6];

    inv[6] = -m[0] * m[6] * m[15] +
        m[0] * m[7] * m[14] +
        m[4] * m[2] * m[15] -
        m[4] * m[3] * m[14] -
        m[12] * m[2] * m[7] +
        m[12] * m[3] * m[6];

    inv[10] = m[0] * m[5] * m[15] -
        m[0] * m[7] * m[13] -
        m[4] * m[1] * m[15] +
        m[4] * m[3] * m[13] +
        m[12] * m[1] * m[7] -
        m[12] * m[3] * m[5];

    inv[14] = -m[0] * m[5] * m[14] +
        m[0] * m[6] * m[13] +
        m[4] * m[1] * m[14] -
        m[4] * m[2] * m[13] -
        m[12] * m[1] * m[6] +
        m[12] * m[2] * m[5];

    inv[3] = -m[1] * m[6] * m[11] +
        m[1] * m[7] * m[10] +
        m[5] * m[2] * m[11] -
        m[5] * m[3] * m[10] -
        m[9] * m[2] * m[7] +
        m[9] * m[3] * m[6];

    inv[7] = m[0] * m[6] * m[11] -
        m[0] * m[7] * m[10] -
        m[4] * m[2] * m[11] +
        m[4] * m[3] * m[10] +
        m[8] * m[2] * m[7] -
        m[8] * m[3] * m[6];

    inv[11] = -m[0] * m[5] * m[11] +
        m[0] * m[7] * m[9] +
        m[4] * m[1] * m[11] -
        m[4] * m[3] * m[9] -
        m[8] * m[1] * m[7] +
        m[8] * m[3] * m[5];

    inv[15] = m[0] * m[5] * m[10] -
        m[0] * m[6] * m[9] -
        m[4] * m[1] * m[10] +
        m[4] * m[2] * m[9] +
        m[8] * m[1] * m[6] -
        m[8] * m[2] * m[5];

    det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

    if (det == 0)
        return false;

    det = static_cast<float>(1.0 / det);

    for (i = 0; i < 16; i++)
        invOut[i] = inv[i] * det;

    return true;
}

class StreamWriter : public IStreamWriter
{
public:
    StreamWriter(std::filesystem::path pathBase) : root_path(std::move(pathBase)) {
        assert(root_path.has_root_path());
    }

    std::shared_ptr<std::ostream> GetOutputStream(const std::string& filename) const override {
        auto streamPath = root_path / std::filesystem::u8path(filename);
        auto stream = std::make_shared<std::ofstream>(streamPath, std::ios_base::binary);

        if (!stream || !(*stream)) {
            throw std::runtime_error("Unable to create a valid output stream for uri: " + filename);
        }

        return stream;
    }

private:
    std::filesystem::path root_path;
};

struct SkinnedMeshPrimitiveContext {
    std::string mesh_name;
    std::string index_acc;
    std::string vertex_acc;
    std::string normal_acc;
    std::string tangent_acc;
    std::string uv_coord_acc;
    std::vector<std::string>* joint_nodes;
    std::string joints_acc;
    std::string weights_acc;
    std::string inv_bind_mat_id;
    std::string skinId;
    bool is_weighted_mesh;
};

void CreateMeshPrimitveResources(BufferBuilder& buffer_builder, const GlacierFormats::IMesh* mesh, SkinnedMeshPrimitiveContext& ctx) {
    //Index data
    buffer_builder.AddBufferView(BufferViewTarget::ELEMENT_ARRAY_BUFFER);
    auto indices = mesh->getIndexBuffer();
    ctx.index_acc = buffer_builder.AddAccessor(indices, { TYPE_SCALAR, COMPONENT_UNSIGNED_SHORT }).id;//ComponentType has to be unsigned according to spec.

    //Vertex position data
    buffer_builder.AddBufferView(BufferViewTarget::ARRAY_BUFFER);
    auto positions = mesh->getVertexBuffer();

    std::vector<float> bb_min(3, std::numeric_limits<float>::max());    //TODO: replace with IMesh bounding box code
    std::vector<float> bb_max(3, std::numeric_limits<float>::min());
    for (int j = 0; j < 3; ++j) {
        for (int i = j; i < positions.size(); i += 3) {
            bb_min[j] = std::min(positions[i], bb_min[j]);
            bb_max[j] = std::max(positions[i], bb_max[j]);
        }
    }

    ctx.vertex_acc = buffer_builder.AddAccessor(positions, { TYPE_VEC3, COMPONENT_FLOAT, false, bb_min, bb_max }).id;

    //Normals
    buffer_builder.AddBufferView();
    auto normals = mesh->getNormals();
    ctx.normal_acc = buffer_builder.AddAccessor(normals, { TYPE_VEC3, COMPONENT_FLOAT }).id;

    //Tangents
    buffer_builder.AddBufferView();
    auto tangents = mesh->getTangents();
        //set handedness
    for (int i = 3; i < tangents.size(); i += 4)
        tangents[i] = 1.f;
    ctx.tangent_acc = buffer_builder.AddAccessor(tangents, { TYPE_VEC4, COMPONENT_FLOAT }).id;

    //Texture coordinates
    buffer_builder.AddBufferView();
    auto uv_coordinates = mesh->getUVs();
    for (int i = 1; i < uv_coordinates.size(); i = i + 2)
        uv_coordinates[i] *= -1.0f;
    AccessorDesc acc_desc(TYPE_VEC2, COMPONENT_FLOAT);//TODO: Might have to actaully normalize here, the hut key sample model seems to have problems.
    ctx.uv_coord_acc = buffer_builder.AddAccessor(uv_coordinates, acc_desc).id;

    //TODO: Double check if bone weights are exported in normalized form from IMesh interface
    //Weights in PRIM are not necessarily normalized.
    if (!ctx.is_weighted_mesh)
        return;

    const auto& bone_weights = mesh->getBoneWeights();
    constexpr int influence_count = 4;//Count of bones that can influence a single vertex per ACCESSOR_JOINTS_X / ACCESSOR_WEIGHTS_X.

    std::vector<short> joints(mesh->vertexCount() * influence_count, 0);//I wish I could do std::vector<std::array<short, influence_count>> :(
    std::vector<float> weights(mesh->vertexCount() * influence_count, 0);
    for (int vert_id = 0; vert_id < mesh->vertexCount(); ++vert_id) {
        int slot_idx = 0;
        for (const auto& w : bone_weights) {
            if (w.vertex_id == vert_id) {
                assert(slot_idx < influence_count);
                joints[vert_id * influence_count + slot_idx] = static_cast<short>(w.bone_id);//TODO: IMesh interface should be changed to emmit short since PRIM only supports short anyway.
                weights[vert_id * influence_count + slot_idx] = w.weight;
                slot_idx++;
            }
        }
    }
    buffer_builder.AddBufferView();
    ctx.joints_acc = buffer_builder.AddAccessor(joints, { TYPE_VEC4, COMPONENT_UNSIGNED_SHORT }).id;
    buffer_builder.AddBufferView();
    ctx.weights_acc = buffer_builder.AddAccessor(weights, { TYPE_VEC4, COMPONENT_FLOAT }).id;
}

void CreateJoints(const GlacierFormats::IRig* rig, Document& document, std::vector<std::string>& joints) {
    //Add joints in reverse tree order so indices doen't have to be known in advance.
    auto bones = rig->getCanonicalBoneList();
    std::vector<Node> bone_nodes(bones.size());

    for (int i = 0; i < bones.size(); ++i) {
        Node node;
        node.name = bones[i].name;
        node.translation = Vector3(bones[i].position[0], bones[i].position[1], bones[i].position[2]);
        bone_nodes[i] = node;
    }

    auto id_offset = document.nodes.Size();
    for (int i = 0; i < bones.size(); ++i) {
        for (int j = 0; j < bones.size(); ++j) {
            if (bones[i].parent_id == j) {
                bone_nodes[j].children.push_back(std::to_string(i + id_offset));
            }
        }
    }

    for (auto& node : bone_nodes) {
        auto node_id = document.nodes.Append(node, AppendIdPolicy::GenerateOnEmpty).id;
        joints.push_back(node_id);
    }
}

std::string CreateRigResources(BufferBuilder& buffer_builder, const GlacierFormats::IRig* rig) {
    buffer_builder.AddBufferView();

    auto bones = rig->getCanonicalBoneList();

    std::vector<Matrix4> inv_bind(bones.size());

    for (int i = 0; i < bones.size(); ++i) {
        Matrix4 inv_bind_mat = Matrix4::IDENTITY;
        auto& pos = bones[i].position;
        inv_bind_mat.values[12] = -pos[0];
        inv_bind_mat.values[13] = -pos[1];
        inv_bind_mat.values[14] = -pos[2];

        Matrix4 inv;
        auto suc = InvertMatrix(inv_bind_mat.values.data(), inv.values.data());
        if (!suc)
            throw std::runtime_error("Non invertible bind matrix encountered while building GLTF rig resources");
        inv_bind[i] = inv_bind_mat;
    }

    std::vector<Matrix4> inv_bindn(bones.size());
    for (int i = 0; i < bones.size(); ++i) {
        int current_id = i;
        inv_bindn[i] = inv_bind[i];
        while (bones[current_id].parent_id != -1) {
            for (int j = 12; j < 15; ++j) {
                inv_bindn[i].values[j] += inv_bind[bones[current_id].parent_id].values[j];
            }
            current_id = bones[current_id].parent_id;
        }
    }

    std::vector<float> inv_bind_mats;
    for (int i = 0; i < bones.size(); ++i) {
        for (int j = 0; j < 16; ++j) {
            inv_bind_mats.push_back(inv_bindn[i].values[j]);
        }
    }
    return buffer_builder.AddAccessor(inv_bind_mats, { TYPE_MAT4, COMPONENT_FLOAT }).id;
}


std::string CreateTexture(Document& document, std::string&& uri) {
    Image image;
    image.uri = std::move(uri);
    auto image_id = document.images.Append(std::move(image), AppendIdPolicy::GenerateOnEmpty).id;

    Texture texture;
    texture.imageId = image_id;
    auto texture_id = document.textures.Append(std::move(texture), AppendIdPolicy::GenerateOnEmpty).id;
    return texture_id;
}

//Create generic SpecGloss PBR material from textures.
//Glacier uses the spec gloss workflow for materials. The gneric SG_material exported from this function
//doesn't look great in blender though, so it't better to use the generic MetalRough material for now.
//This might change once the material files are reverse engineering to a greater extent.
std::string CreateSpecGlossMaterial(Document& document, std::string* albedo, std::string* normal, std::string* specular) {
    Material material;
    if (normal)
        material.normalTexture.textureId = *normal;
    const auto normal_scale = 0.6f;
    material.normalTexture.scale = normal_scale;

    //Glacier Materials are SpecGloss materials, PBRSpecularGlossiness extension required
    KHR::Materials::PBRSpecularGlossiness sg_material;
    if (albedo)
        sg_material.diffuseTexture.textureId = *albedo;
    if (specular)
        sg_material.specularGlossinessTexture.textureId = *specular;
    const auto gloss_factor = 0.27f;
    sg_material.glossinessFactor = gloss_factor;
    material.SetExtension<Microsoft::glTF::KHR::Materials::PBRSpecularGlossiness>(sg_material);
    document.extensionsUsed.emplace(Microsoft::glTF::KHR::Materials::PBRSPECULARGLOSSINESS_NAME);
    document.extensionsRequired.emplace(Microsoft::glTF::KHR::Materials::PBRSPECULARGLOSSINESS_NAME);

    return document.materials.Append(std::move(material), AppendIdPolicy::GenerateOnEmpty).id;
}

std::string CreateMetalRoughMaterial(Document& document, std::string* albedo, std::string* normal) {
    Material material;
    if (albedo)
        material.metallicRoughness.baseColorTexture.textureId = *albedo;
    if (normal)
        material.normalTexture.textureId = *normal;
    const auto normal_scale = 0.6f;
    material.normalTexture.scale = normal_scale;

    return document.materials.Append(std::move(material), AppendIdPolicy::GenerateOnEmpty).id;
}

constexpr bool USE_SPEC_GLOSS_EXT = false;

std::string CreateMaterial(Document& document, const GlacierFormats::IMaterial* src_material) {
    //TODO: Externalize this fixed extention and think about the texture situation. 
    //GLTF doesn't officially support tga and most viewers ignore the textures. Blender thankfully doesn't. 
    //DDS can be supported via a vendor extension but this extention might 
    //not be supported in blender and other editors. Further research required... 
    const std::string ext = ".tga";

    auto albedo_texture_name = src_material->getDiffuseMap()->name() + ext;//TODO: potentially null.
    auto normal_texture_name = src_material->getNormalMap()->name() + ext;
    auto specular_texture_name = src_material->getSpecularMap()->name() + ext;

    auto albedo = CreateTexture(document, albedo_texture_name.c_str());
    auto normal = CreateTexture(document, normal_texture_name.c_str());
    auto specular = CreateTexture(document, specular_texture_name.c_str());

    if constexpr (USE_SPEC_GLOSS_EXT)
        return CreateSpecGlossMaterial(document, &albedo, &normal, &specular);
    else
        return CreateMetalRoughMaterial(document, &albedo, &normal);
}

std::string CreateMesh(Document& document, const GlacierFormats::IMaterial* src_material, const SkinnedMeshPrimitiveContext& ctx) {
    std::string material_id;
    //if(src_material)
    //    material_id = CreateMaterial(document, src_material);

    MeshPrimitive mesh_primitive;
    mesh_primitive.indicesAccessorId = ctx.index_acc;
    mesh_primitive.attributes[ACCESSOR_POSITION] = ctx.vertex_acc;
    mesh_primitive.attributes[ACCESSOR_NORMAL] = ctx.normal_acc;
    mesh_primitive.attributes[ACCESSOR_TANGENT] = ctx.tangent_acc;
    mesh_primitive.attributes[ACCESSOR_TEXCOORD_0] = ctx.uv_coord_acc;
    if (ctx.is_weighted_mesh) {
        mesh_primitive.attributes[ACCESSOR_JOINTS_0] = ctx.joints_acc;
        mesh_primitive.attributes[ACCESSOR_WEIGHTS_0] = ctx.weights_acc;
    }
    //if (src_material)
    //    mesh_primitive.materialId = material_id;

    Mesh mesh;
    mesh.name = ctx.mesh_name;
    mesh.primitives.push_back(std::move(mesh_primitive));
    return document.meshes.Append(std::move(mesh), AppendIdPolicy::GenerateOnEmpty).id;
}

std::string CreateSkin(Document& document, const SkinnedMeshPrimitiveContext& ctx) {
    Skin skin;
    skin.name = "Armature";
    for (const auto& id : (*ctx.joint_nodes))
        skin.jointIds.push_back(id);
    skin.inverseBindMatricesAccessorId = ctx.inv_bind_mat_id;
    skin.skeletonId = ctx.joint_nodes->front();
    return document.skins.Append(std::move(skin), AppendIdPolicy::GenerateOnEmpty).id;
}

std::string CreateMeshPrimitiveEntities(const GlacierFormats::IMesh* mesh, const GlacierFormats::IMaterial* src_material, Document& document, const SkinnedMeshPrimitiveContext& ctx) {
    Node mesh_node;
    mesh_node.name = ctx.mesh_name;
    mesh_node.meshId = CreateMesh(document, src_material, ctx);
    if (ctx.is_weighted_mesh)
        mesh_node.skinId = ctx.skinId;

    return document.nodes.Append(std::move(mesh_node), AppendIdPolicy::GenerateOnEmpty).id;
}

void GLTFExporter::operator()(const GlacierFormats::IRenderAsset& model, const std::filesystem::path& path) const {
    auto stream_writer = std::make_unique<StreamWriter>(path);

    auto extension = GLTF_EXTENSION;//TODO: Externalize
    auto file_name = std::filesystem::path(model.name() + "." + extension);

    std::unique_ptr<ResourceWriter> resource_writer;
    if (extension == GLTF_EXTENSION)
        resource_writer = std::make_unique<GLTFResourceWriter>(std::move(stream_writer));
    else
        resource_writer = std::make_unique<GLBResourceWriter>(std::move(stream_writer));

    BufferBuilder buffer_builder(std::move(resource_writer));

    if (extension == GLTF_EXTENSION)
        buffer_builder.AddBuffer();
    else
        buffer_builder.AddBuffer(GLB_BUFFER_ID);

    Document document;
    Scene scene;
    scene.name = model.name();

    //Meshes
    for (int i = 0; i < model.meshes().size(); ++i) {
        SkinnedMeshPrimitiveContext ctx{};

        if (model.rig() != nullptr) {
            ctx.is_weighted_mesh = true;

            //Sub meshes could in principle share a rig but Blender's GLTF IO plugin is unfortunately not spec conform and can't deal with shared skins.
            std::vector<std::string> joints;
            ctx.joint_nodes = &joints;
            CreateJoints(model.rig(), document, joints);

            ctx.inv_bind_mat_id = CreateRigResources(buffer_builder, model.rig());

            Node armature_empty;
            armature_empty.name = "Armature";
            armature_empty.children.push_back(joints.front());
            auto armature_empty_id = document.nodes.Append(armature_empty, AppendIdPolicy::GenerateOnEmpty).id;
            scene.nodes.push_back(armature_empty_id);

            ctx.skinId = CreateSkin(document, ctx);
        }
        else {
            ctx.is_weighted_mesh = false;
        }

        const GlacierFormats::IMesh* mesh = model.meshes()[i];
        ctx.mesh_name = mesh->name();
        CreateMeshPrimitveResources(buffer_builder, mesh, ctx);

        int mat_id = model.meshes()[i]->materialId();
        IMaterial* src_material = nullptr;
        if(model.materials().size())
            src_material = model.materials()[0];//TODO: Fix material id mess. (See notes in 010)
        auto mesh_id = CreateMeshPrimitiveEntities(mesh, src_material, document, ctx);
        scene.nodes.push_back(mesh_id);
    }

    document.SetDefaultScene(std::move(scene), AppendIdPolicy::GenerateOnEmpty);

    buffer_builder.Output(document);
    try {
        Validation::Validate(document);
    }
    catch (const  GLTFException & e) {
        throw std::runtime_error("GLTF validation failed: " + std::string(e.what()));
    }

    std::string manifest;

    try {
        auto extension_serializer = KHR::GetKHRExtensionSerializer();//Only required when exporting GlossSpec materials.
        manifest = Serialize(document, extension_serializer, SerializeFlags::Pretty);
    }
    catch (const GLTFException & e) {
        throw std::runtime_error("GLTF serialization failed: " + std::string(e.what()));
    }

    if (extension != GLTF_EXTENSION)
        dynamic_cast<GLBResourceWriter*>(resource_writer.get())->Flush(manifest, file_name.u8string());
    else
        buffer_builder.GetResourceWriter().WriteExternal(file_name.u8string(), manifest);
}

void GlacierFormats::Export::GLTFExporter::operator()(const IMesh& mesh, const std::filesystem::path& export_dir) const {
    throw std::runtime_error("Not implemented");
}
