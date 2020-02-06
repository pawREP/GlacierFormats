#include <string>
#include <stdexcept>
#include "GLTFMesh.h"

using namespace GlacierFormats;

GLTFMesh::GLTFMesh() {

}

std::string GLTFMesh::name() const noexcept {
    return name_;
}

int GLTFMesh::materialId() const noexcept {
    //throw std::runtime_error("Not implemented");//TODO: IMpl
    return 0;
}

size_t GLTFMesh::vertexCount() const
{
    return positions.size() / IMesh::vertex_size;
}

std::vector<float> GLTFMesh::getVertexBuffer() const {
    return positions;
}

std::vector<unsigned short> GLTFMesh::getIndexBuffer() const {
    return index_buffer;
}

std::vector<float> GLTFMesh::getNormals() const {
    return normals;
}

std::vector<float> GLTFMesh::getUVs() const {
    return uvs;
}

std::vector<GlacierFormats::IMesh::BoneWeight> GLTFMesh::getBoneWeights() const {
    return weights;
}

void GLTFMesh::setVertexBuffer(const std::vector<float>& pos) {
    positions = pos;
}

void GLTFMesh::setIndexBuffer(const std::vector<unsigned short>& indices_) {
    index_buffer = indices_;
}

void GLTFMesh::setNormals(const std::vector<float>& normals_) {
    normals = normals_;
}

void GLTFMesh::setUVs(const std::vector<float>& uvs_) {
    uvs = uvs_;
}

void GLTFMesh::setBoneWeight(const std::vector<BoneWeight>& weights_) {
    weights = weights_;
}

void GLTFMesh::setName(const std::string& name) {
    name_ = name;
}
