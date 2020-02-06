#pragma once

#include <memory>
#include <iostream>
#include <filesystem>
#include <chrono>
#include "fbxsdk.h"
#include "../IRenderAsset.h"

namespace GlacierFormats {
	namespace Export {

		//Deprecated FBX exporter. Use GLTF. 
		class FBXExport : public IExporter {
		private:
			mutable FbxManager* fbx_manager;
			mutable FbxScene* fbx_scene;

			FbxNode* CreateSkeleton(const GlacierFormats::IRig* rig, const char* name) const;
			FbxSurfacePhong* CreateMaterial(const GlacierFormats::IMaterial* src_material, const char* name) const;
			FbxSkin* CreateDeformer(const GlacierFormats::IMesh* mesh, const GlacierFormats::IRig* rig, FbxNode* mesh_node, FbxNode* skeletonRootNode) const;
			FbxCluster* CreateDeformerCluster(const GlacierFormats::IMesh* mesh, const GlacierFormats::IRig* rig, FbxNode* bone) const;

			void processMesh(const GlacierFormats::IRenderAsset& model) const;
			void processRig(const GlacierFormats::IRig* src_rig, const GlacierFormats::IMesh* src_mesh, FbxMesh* pMesh) const;
			void Export(const std::string& path) const;

		public:
			void operator()(const GlacierFormats::IRenderAsset& model, const std::filesystem::path& export_dir) const override final;
		};
	
	}
}