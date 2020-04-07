#include "GLTFAsset.h"
#include "GLTFMesh.h"
#include "GLTFSDK/IStreamReader.h"
#include "GLTFSDK/GLTFResourceReader.h"
#include "GLTFSDK/GLBResourceReader.h"
#include "GLTFSDK/Deserialize.h"

#include <unordered_map>

using namespace GlacierFormats;
using namespace Microsoft::glTF;

namespace
{
    class StreamReader : public IStreamReader
    {
    public:
        StreamReader(std::filesystem::path path) : path_base(std::move(path)) {
            assert(path_base.has_root_path());
        }

        std::shared_ptr<std::istream> GetInputStream(const std::string& filename) const override
        {
            auto streamPath = path_base / std::filesystem::u8path(filename);
            auto stream = std::make_shared<std::ifstream>(streamPath, std::ios_base::binary);

            if (!stream || !(*stream))
                throw std::runtime_error("Unable to create a valid input stream for uri: " + filename);

            return stream;
        }

    private:
        std::filesystem::path path_base;
    };
}

void parseTextures(GLTFAsset* asset, const Document& document) {
    for (const auto& img : document.images.Elements()) {
        assert((img.mimeType == "image/png", "Unsupported mimeType. GLTF import only supports png texture import."));
        std::filesystem::path image_path(img.uri);
        const auto image_name = image_path.stem().generic_string();//TODO: check if image_name is hex number format. (Texture names have to be TEXD runtime ids)
        asset->textures.push_back(image_name);
    }
}

//void foo(Document& document, Node& node, const std::unordered_map<std::string, int>* bone_name_to_index_map, std::vector<int>& map) {
//    if(node.)
//    mapbone_name_to_index_map->at(node.name)
//};

GLTFAsset::GLTFAsset(const std::filesystem::path& path, const std::unordered_map<std::string, int>* bone_name_to_index_map) : source_directory(path.parent_path()) {
    //TODO: Factor this function

    if (!path.has_filename())
        throw std::runtime_error("no filename");

    if (!path.has_extension())
        throw std::runtime_error("no filename extension");

    name_ = path.stem().generic_string();

    // Pass the absolute path, without the filename, to the stream reader
    auto streamReader = std::make_unique<StreamReader>(path.parent_path());

    std::filesystem::path pathFile = path.filename();
    std::filesystem::path pathFileExt = pathFile.extension();

    std::string manifest;

    auto MakePathExt = [](const std::string& ext) {return "." + ext; };

    std::unique_ptr<GLTFResourceReader> resourceReader;

    // If the file has a '.gltf' extension then create a GLTFResourceReader
    if (pathFileExt == MakePathExt(GLTF_EXTENSION))
    {
        auto gltfStream = streamReader->GetInputStream(pathFile.u8string()); // Pass a UTF-8 encoded filename to GetInputString
        auto gltfResourceReader = std::make_unique<GLTFResourceReader>(std::move(streamReader));

        std::stringstream manifestStream;

        // Read the contents of the glTF file into a string using a std::stringstream
        manifestStream << gltfStream->rdbuf();
        manifest = manifestStream.str();

        resourceReader = std::move(gltfResourceReader);
    }

    // If the file has a '.glb' extension then create a GLBResourceReader. This class derives
    // from GLTFResourceReader and adds support for reading manifests from a GLB container's
    // JSON chunk and resource data from the binary chunk.
    if (pathFileExt == MakePathExt(GLB_EXTENSION))
    {
        auto glbStream = streamReader->GetInputStream(pathFile.u8string()); // Pass a UTF-8 encoded filename to GetInputString
        auto glbResourceReader = std::make_unique<GLBResourceReader>(std::move(streamReader), std::move(glbStream));

        manifest = glbResourceReader->GetJson(); // Get the manifest from the JSON chunk

        resourceReader = std::move(glbResourceReader);
    }

    if (!resourceReader)
        throw std::runtime_error("Command line argument path filename extension must be .gltf or .glb");

    Document document;
    try {
        document = Deserialize(manifest);
    }
    catch (const GLTFException & e) {
        std::stringstream ss;

        ss << "Microsoft::glTF::Deserialize failed: ";
        ss << e.what();

        throw std::runtime_error(ss.str());
    }

    //Parse Textures
    parseTextures(this, document);

    //Parse meshes
    assert((document.scenes.Size() == 1, "GLTF should only have one scene"));
    const auto& scene = document.scenes.Front();
    auto node_count = document.nodes.Size();
    for (int i = 0; i < node_count; ++i) {
        const auto& node = document.nodes.Get(i);
        if (node.meshId == "")
            continue; //Node isn't a mesh node, return early.

        const auto& mesh = document.meshes.Get(node.meshId);

        std::string accessor_id;

        assert(mesh.primitives.size() == 1); //The GLTF exporter maps PRIM submeshes directly to GLTF meshes. During import the same correspondance is expected. 

        for (const auto& primitive : mesh.primitives) {
            //auto gltf_mesh = std::unique_ptr<GlacierFormats::IMesh>(new GLTFMesh());
            std::unique_ptr<IMesh> gltf_mesh = std::make_unique<GLTFMesh>();

            //name
            gltf_mesh->setName(mesh.name);

            //indices
            const Accessor& accessor = document.accessors.Get(primitive.indicesAccessorId);
            auto indices_data = resourceReader->ReadBinaryData<unsigned short>(document, accessor);
            gltf_mesh->setIndexBuffer(std::move(indices_data));

            //positions
            if (primitive.TryGetAttributeAccessorId(ACCESSOR_POSITION, accessor_id)) {
                const Accessor& accessor = document.accessors.Get(accessor_id);
                auto position_data = resourceReader->ReadBinaryData<float>(document, accessor);
                gltf_mesh->setVertexBuffer(std::move(position_data));
            }

            //uvs
            if (primitive.TryGetAttributeAccessorId(ACCESSOR_TEXCOORD_0, accessor_id)) {
                const Accessor& accessor = document.accessors.Get(accessor_id);
                auto uv_data = resourceReader->ReadBinaryData<float>(document, accessor);
                gltf_mesh->setUVs(std::move(uv_data));
            }

            //normals
            if (primitive.TryGetAttributeAccessorId(ACCESSOR_NORMAL, accessor_id)) {
                const Accessor& accessor = document.accessors.Get(accessor_id);
                auto normal_data = resourceReader->ReadBinaryData<float>(document, accessor);
                gltf_mesh->setNormals(std::move(normal_data));
            }
            //TODO: Error handling for missing normals, tangents, tex coords.
            //tangents
            //GLACIER_ASSERT_TRUE(primitive.TryGetAttributeAccessorId(ACCESSOR_TANGENT, accessor_id));
            if (primitive.TryGetAttributeAccessorId(ACCESSOR_TANGENT, accessor_id))
            {
                const Accessor& accessor = document.accessors.Get(accessor_id);
                auto tangent_data = resourceReader->ReadBinaryData<float>(document, accessor);
                gltf_mesh->setTangents(std::move(tangent_data));
            }else{
                //TODO: ungly ugly ugly hack
                const Accessor& accessor = document.accessors.Get(accessor_id);
                auto normal_data = resourceReader->ReadBinaryData<float>(document, accessor);
                std::vector<float> tangents(normal_data.size() / 3 * 4);
                gltf_mesh->setTangents(std::move(tangents));
            }

            //bone data
            if (primitive.HasAttribute(ACCESSOR_JOINTS_0) && primitive.HasAttribute(ACCESSOR_WEIGHTS_0) && (node.skinId != "")) {
                const auto& skin = document.skins.Get(node.skinId);

                //Compute re-mapping
                if (!bone_name_to_index_map)
                    throw std::runtime_error("GLTF Import: Tried to parse weighted mesh without passing a bone id mapping to GLTFAsset::GLTFAsset.");
                std::vector<int> bone_remapping(skin.jointIds.size()); //maps gltf joint index to corresponding borg joint index.
                for (int joint_index = 0; joint_index < skin.jointIds.size(); ++joint_index) {
                    const auto joint_id = skin.jointIds[joint_index];
                    const auto& joint_node = document.nodes.Get(joint_id);
                    try {
                        bone_remapping[joint_index] = bone_name_to_index_map->at(joint_node.name);
                    }
                    catch (...) {
                        throw std::runtime_error("GLTF Import: GlTF skeleton doesn't match the skeleton of the import target.");
                    }
                }

                accessor_id = primitive.GetAttributeAccessorId(ACCESSOR_JOINTS_0);
                const Accessor& joints_accessor = document.accessors.Get(accessor_id);

                accessor_id = primitive.GetAttributeAccessorId(ACCESSOR_WEIGHTS_0);
                const Accessor& weights_accessor = document.accessors.Get(accessor_id);

                const auto joints_data = resourceReader->ReadBinaryData<unsigned short>(document, joints_accessor);
                const auto weights_data = resourceReader->ReadBinaryData<float>(document, weights_accessor);

                //structure of the joints_data and weights_data is:
                //Joints: 4 bone_ids per vertex. unused bone_ids are indicated with weights of zero
                //Weights; 4 weights that correspond to the bone_ids in joints. 0.0f if empty.
                const auto vertex_count = gltf_mesh->vertexCount();
                const auto influnces = 4;
                assert((joints_data.size() / influnces) == vertex_count);

                std::vector<IMesh::BoneWeight> bone_weights;
                bone_weights.reserve(vertex_count * influnces);
                for (int i = 0; i < joints_data.size(); ++i) {
                    auto vertex_id = i / influnces;
                    IMesh::BoneWeight weight{ vertex_id, bone_remapping[joints_data[i]], weights_data[i] };
                    bone_weights.push_back(weight);
                }
                //TODO: Normalization required? Read spec.
                gltf_mesh->setBoneWeight(bone_weights);
            }

            meshes_.push_back(std::move(gltf_mesh));
        }
    }
}

const std::vector<GlacierFormats::IMaterial*> GLTFAsset::materials() const {
    return std::vector<GlacierFormats::IMaterial*>();
}

const std::vector<GlacierFormats::IMesh*> GLTFAsset::meshes() const {
    std::vector<GlacierFormats::IMesh*> ret(meshes_.size());
    for (int i = 0; i < meshes_.size(); ++i)
        ret[i] = meshes_[i].get();
    return ret;
}

const GlacierFormats::IRig* GLTFAsset::rig() const {
    return nullptr;
}

std::string GLTFAsset::name() const {
    return name_;
}