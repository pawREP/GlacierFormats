#include "FBXExporter.h"
#include "../BORG.h"

using namespace GlacierFormats::Export;

auto FbxDestroyer = [](auto* obj) { obj->Destroy(); };

FbxSurfacePhong* FBXExport::CreateMaterial(const GlacierFormats::IMaterial* src_material, const char* name) const {

	//TODO: Implement proper material based on MATI, MATE, ...
	//TODO: Figure out how material properties map to blender shader.

	auto lMaterial = FbxSurfacePhong::Create(fbx_scene, name);
	lMaterial->TransparencyFactor.Set(0.f); //TODO: Apparently there is some bug in the SDK related to TransparencyFactor, that makes
											//the material not show up after import into 3Ds max.
	lMaterial->ShadingModel.Set("Phong");
	//lMaterial->BumpFactor.Set(1.0f); 
	lMaterial->BumpFactor.Set(7.129f); //Somehow translates into "Normal Strength" = 1.0f in blender.

	//Diffuse
	if (src_material->getDiffuseMap()) {
		FbxFileTexture* lTexture = FbxFileTexture::Create(fbx_scene, "Diffuse");
		auto diff_tex_name = src_material->getDiffuseMap()->name() + ".tga";
		lTexture->SetFileName(diff_tex_name.c_str());
		lTexture->SetTextureUse(FbxTexture::eStandard);
		lTexture->SetMappingType(FbxTexture::eUV);
		lTexture->SetMaterialUse(FbxFileTexture::eModelMaterial);
		lTexture->SetSwapUV(false);
		lTexture->SetTranslation(0.0, 0.0);
		lTexture->SetScale(1.0, 1.0);
		lTexture->SetRotation(0.0, 0.0);
		lTexture->SetAlphaSource(FbxTexture::eBlack);

		if (lMaterial)
			lMaterial->Diffuse.ConnectSrcObject(lTexture);
	}

	//Specular
	if (src_material->getSpecularMap()) {
		FbxFileTexture* lTexture = FbxFileTexture::Create(fbx_scene, "Specular");
		auto spec_tex_name = src_material->getSpecularMap()->name() + ".tga";
		lTexture->SetFileName(spec_tex_name.c_str());
		lTexture->SetTextureUse(FbxTexture::eStandard);
		lTexture->SetMappingType(FbxTexture::eUV);
		lTexture->SetMaterialUse(FbxFileTexture::eModelMaterial);
		lTexture->SetSwapUV(false);
		lTexture->SetTranslation(0.0, 0.0);
		lTexture->SetScale(1.0, 1.0);
		lTexture->SetRotation(0.0, 0.0);

		if (lMaterial)
			lMaterial->Specular.ConnectSrcObject(lTexture);
	}

	//Normal
	if (src_material->getNormalMap()) {
		FbxFileTexture* lTexture = FbxFileTexture::Create(fbx_scene, "Normal");
		auto normal_tex_name = src_material->getNormalMap()->name() + ".tga";
		lTexture->SetFileName(normal_tex_name.c_str());
		lTexture->SetTextureUse(FbxTexture::eBumpNormalMap);
		lTexture->SetMappingType(FbxTexture::eUV);
		lTexture->SetMaterialUse(FbxFileTexture::EMaterialUse::eModelMaterial);
		lTexture->SetSwapUV(false);
		lTexture->SetTranslation(0.0, 0.0);
		lTexture->SetScale(1.0, 1.0);
		lTexture->SetRotation(0.0, 0.0);

		if (lMaterial)
			lMaterial->Bump.ConnectSrcObject(lTexture);
	}

	return lMaterial;
}

//Sets cluster weights for a single bone based on weights from a IMesh*
void SetInfluences(const GlacierFormats::IMesh* src_mesh, int bone_id, FbxCluster* cluster) {
	auto vertex_weights = src_mesh->getBoneWeights();
	std::vector<int> indices;
	std::vector<float> weights;
	for (int i = 0; i < vertex_weights.size(); ++i) {
		auto w = vertex_weights[i];
		if (bone_id != w.bone_id)
			continue;
		indices.push_back(w.vertex_id);
		weights.push_back(w.weight);
	}
	cluster->SetControlPointIWCount(indices.size());

	for (int i = 0; i < indices.size(); ++i) {
		cluster->GetControlPointIndices()[i] = indices[i];
		cluster->GetControlPointWeights()[i] = weights[i];
	}
}

FbxCluster* FBXExport::CreateDeformerCluster(const GlacierFormats::IMesh* mesh, const GlacierFormats::IRig* rig, FbxNode* bone) const {
	auto bone_name = bone->GetSkeleton()->GetName();
	auto bone_id = rig->getBoneIdByName(bone_name);

	FbxCluster* cluster = FbxCluster::Create(fbx_manager, "DeformerCluster");
	cluster->SetLink(bone);
	//cluster->SetLinkMode(FbxCluster::eTotalOne);
	cluster->SetLinkMode(FbxCluster::eNormalize);
	SetInfluences(mesh, bone_id, cluster);
	return cluster;
}


void gatherChildNodes(FbxNode* root, std::vector<FbxNode*>& nodes) {
	for (int i = 0; i < root->GetChildCount(); ++i) {
		gatherChildNodes(root->GetChild(i), nodes);
	}
	nodes.push_back(root);

}

FbxSkin* FBXExport::CreateDeformer(const GlacierFormats::IMesh* mesh, const GlacierFormats::IRig* rig, FbxNode* mesh_node, FbxNode* root) const {
	//Skinning
	FbxSkin* skin = FbxSkin::Create(fbx_manager, "Deformer");

	std::vector<FbxNode*> bone_nodes;
	gatherChildNodes(root, bone_nodes);

	FbxAMatrix lXMatrix = mesh_node->EvaluateGlobalTransform();
	for (auto bone : bone_nodes) {
		if (bone->GetSkeleton() == NULL)
			continue;
		auto cluster = CreateDeformerCluster(mesh, rig, bone);
		cluster->SetTransformMatrix(lXMatrix);
		cluster->SetTransformLinkMatrix(bone->EvaluateGlobalTransform());
		skin->AddCluster(cluster);
	}

	return skin;
}

void FBXExport::processMesh(const GlacierFormats::IRenderAsset& model) const {

	FbxNode* skeletonRoot = nullptr;
	if (model.rig())
		skeletonRoot = CreateSkeleton(model.rig(), "Skeleton");

	for (auto prim : model.meshes()) {
		//for (int i = 2; i < 3; ++i) {
			//const auto& prim = model.meshes()[i];

		std::string mesh_name = prim->name();

		FbxNode* lMeshNode = FbxNode::Create(fbx_scene, mesh_name.c_str());
		FbxMesh* lMesh = FbxMesh::Create(fbx_scene, "mesh");
		lMeshNode->SetNodeAttribute(lMesh);

		FbxNode* lRootNode = fbx_scene->GetRootNode();
		lRootNode->AddChild(lMeshNode);

		//Initialize mesh node data
		lMesh->InitControlPoints(prim->vertexCount());
		FbxVector4* lControlPoints = lMesh->GetControlPoints();
		auto vertex_buffer = prim->getVertexBuffer();
		for (size_t i = 0; i < prim->vertexCount(); i++) {
			//lControlPoints[i] = FbxVector4(vertex_buffer[3 * i], vertex_buffer[3 * i + 1], vertex_buffer[3 * i + 2]);
			lControlPoints[i] = FbxVector4(vertex_buffer[3 * i], vertex_buffer[3 * i + 2], vertex_buffer[3 * i + 1]);
		}

		//Initialize polygons
		const auto index_buffer = prim->getIndexBuffer();
		const auto tris_count = index_buffer.size() / 3; //TODO: Add to IMesh interface
		for (size_t i = 0; i < tris_count; i++)
		{
			lMesh->BeginPolygon(0);
			lMesh->AddPolygon(index_buffer[3 * i]);
			lMesh->AddPolygon(index_buffer[3 * i + 1]);
			lMesh->AddPolygon(index_buffer[3 * i + 2]);
			lMesh->EndPolygon();
		}
		lMesh->BuildMeshEdgeArray();

		//Initialize UV
		lMesh->CreateLayer();
		FbxLayer* uv_layer = lMesh->GetLayer(0);
		FbxLayerElementUV* lUVDiffuseElement = lMesh->CreateElementUV("UV");
		lUVDiffuseElement->SetMappingMode(FbxLayerElement::eByControlPoint);
		lUVDiffuseElement->SetReferenceMode(FbxLayerElement::eDirect);

		const auto uvs = prim->getUVs();
		const auto uv_count = uvs.size() / 2;//TODO; uv count to prim interface
		for (size_t i = 0; i < uv_count; i++)
			lUVDiffuseElement->GetDirectArray().Add(FbxVector2(uvs[2 * i], uvs[2 * i + 1]));

		uv_layer->SetUVs(lUVDiffuseElement);

		//Material
		auto id = prim->materialId();

		//There might be a bug with meshes that only have a single material, where the material id
		//in prim will be set to 1 instead of 0. ?
		auto src_materials = model.materials();
		if (src_materials.size() <= id) {
			if (src_materials.size() == 1) {
				id = 0;
			}
			else {
				continue;//TODO: the materials are messed up, this code shouldn't be reachable
				//This code might be reached if one of the texture maps couldn 't be found????
			}

			auto material = src_materials[id];
			auto lMaterial = CreateMaterial(material, "Material");
			lMesh->GetNode()->AddMaterial(lMaterial);
		}


		//TODO: Add check for Rig id of model ebefore attaching to skeleton
		//TODO: Can PRIM models even have more than one rig? 
		if (skeletonRoot) {
			lRootNode->AddChild(skeletonRoot);

			auto skin = CreateDeformer(prim, model.rig(), lMeshNode, skeletonRoot);
			lMesh->AddDeformer(skin);
		}
	}
}



FbxNode* FBXExport::CreateSkeleton(const GlacierFormats::IRig* rig, const char* name) const {
	auto bones = rig->getCanonicalBoneList();

	FbxString rootName(name);

	std::vector<FbxNode*> nodes;

	nodes.resize(bones.size());
	for (int i = 0; i < bones.size(); ++i) {
		auto& bone = bones[i];
		//auto bone_name = rootName + bone.name.c_str();
		auto bone_name = bone.name.c_str();

		auto attribute = FbxSkeleton::Create(fbx_scene, bone_name);
		//attribute->LimbLength.Set(0.1);//Does nothing? Might not be parsed by blender/
		attribute->LimbLength.Set(0.01);
		//attribute->Size.Set(1.0f);
		if (bone.parent_id == -1) //Root node / Ground
			attribute->SetSkeletonType(FbxSkeleton::eRoot);
		else
			attribute->SetSkeletonType(FbxSkeleton::eLimbNode);

		nodes[bone.id] = FbxNode::Create(fbx_scene, bone_name);
		nodes[bone.id]->SetNodeAttribute(attribute);
	}

	//hierarchy
	for (size_t i = 0; i < bones.size(); i++) {
		int32_t parent_id = bones[i].parent_id;
		if (parent_id == -1)
			continue;
		nodes[parent_id]->AddChild(nodes[i]);
	}

	for (int i = 0; i < nodes.size(); ++i) {
		auto& bone = bones[i];

		nodes[bone.id]->LclTranslation.Set(FbxDouble4(bone.position[0], bone.position[2], bone.position[1], 1));
	}
	return nodes.front();
}

void FBXExport::processRig(const GlacierFormats::IRig* src_rig, const GlacierFormats::IMesh* src_mesh, FbxMesh* pMesh) const {
	auto& rig = src_rig;

	if (!rig)
		return;

	auto src_bones = rig->getCanonicalBoneList();

	std::vector<FbxNode*> bone_nodes;

	for (const auto& bone : src_bones) {
		bone_nodes.push_back(FbxNode::Create(fbx_scene, bone.name.c_str()));
	}

	//set hirarchy
	fbx_scene->GetRootNode()->AddChild(bone_nodes.front());
	for (size_t i = 0; i < src_bones.size(); i++) {
		int32_t parent_id = src_bones[i].parent_id;
		if (parent_id != -1) {
			bone_nodes[parent_id]->AddChild(bone_nodes[i]);
		}
	}

	auto borg = dynamic_cast<const GlacierFormats::BORG*>(rig);//TODO: This is only temp, add transform accessor to interface

	for (int i = 0; i < bone_nodes.size(); ++i) {
		//TODO: Add some transformation here to match the coordinate space of exported meshes, (check fbx in blender)
		auto& transform = borg->bind_pose[i].localTransformation;
		FbxQuaternion rotQ(transform.i, transform.j, transform.k, transform.real);

		//adjust transform to match mesh export
		//FbxQuaternion coord_trans;
		//coord_trans.ComposeSphericalXYZ(FbxDouble3(0, -90, 180));
		//rotQ *= coord_trans;

		auto rot = rotQ.DecomposeSphericalXYZ();
		bone_nodes[i]->LclRotation.Set(rot);

		auto pos = borg->bind_pose[i].position;
		bone_nodes[i]->LclTranslation.Set(FbxDouble3(pos.x(), pos.y(), pos.z()));
	}

	//Skinning
	FbxSkin* skin = FbxSkin::Create(fbx_manager, "Deformer");

	for (int bone_id = 0; bone_id < src_bones.size(); ++bone_id) {
		auto bone_node = bone_nodes[bone_id];

		FbxCluster* cluster = FbxCluster::Create(fbx_manager, "DeformerCluster");
		cluster->SetLink(bone_node);
		cluster->SetLinkMode(FbxCluster::eTotalOne);
		SetInfluences(src_mesh, bone_id, cluster);
		skin->AddCluster(cluster);
	}

	pMesh->AddDeformer(skin);

	FbxPose* pose = FbxPose::Create(fbx_scene, "Pose");
	pose->SetIsBindPose(true);

	for (int i = 0; i < skin->GetClusterCount(); ++i) {
		auto cluster = skin->GetCluster(i)->GetLink();
		pose->Add(cluster, cluster->EvaluateGlobalTransform());
	}

	fbx_scene->AddPose(pose);

	//Second pose that isn't bind pose, to fix importer error in 3ds max
	//TODO: This doesn't fix the issue. Needs more testing.
	FbxPose* pose2 = FbxPose::Create(fbx_scene, "Pose2");
	pose2->SetIsBindPose(false);
	fbx_scene->AddPose(pose2);
}

void FBXExport::Export(const std::string& path) const {
	FbxIOSettings* ios = FbxIOSettings::Create(fbx_manager, IOSROOT);
	fbx_manager->SetIOSettings(ios);

	FbxExporter* lExporter = FbxExporter::Create(fbx_manager, "");

	auto export_path = path + "\\model.fbx";
	bool lExportStatus = lExporter->Initialize(export_path.c_str(), -1, fbx_manager->GetIOSettings());

	lExporter->Export(fbx_scene);

	lExporter->Destroy();
}

[[deprecated("Superseded by GLTF exporter")]] void FBXExport::operator()(const GlacierFormats::IRenderAsset& model, const std::filesystem::path& export_dir) const {
	const char* scene_name = "SceneName";

	std::unique_ptr<FbxManager, decltype(FbxDestroyer)> fbxManager(FbxManager::Create(), FbxDestroyer);
	std::unique_ptr<FbxScene, decltype(FbxDestroyer)> fbxScene(FbxScene::Create(fbxManager.get(), scene_name), FbxDestroyer);
	fbx_manager = fbxManager.get();
	fbx_scene = fbxScene.get();

	//AddModelToScene(model);
	//model.materials
	processMesh(model);
	//processRig(model.rig,model.);

	Export(export_dir.generic_string());

}