#include "RenderAsset.h"
#include "ResourceRepository.h"
#include "ResourceReference.h"
#include "Hash.h"
#include "Material.h"
#include <string_view>

using namespace GlacierFormats;

	void parseAssetReferences(ResourceRepository* repo, IResourceNode* root, const std::vector<ResourceReference>& references) {
		for (const auto& ref : references) {
			auto type = repo->getResourceType(ref.id);

			std::unique_ptr<IResourceNode> child_node = nullptr;
			switch (hash::fnv1a(type)) {
			case hash::fnv1a("BORG"):
				child_node = std::make_unique<ResourceNode<BORG>>(ref.id, repo->getResource<BORG>(ref.id));
				break;
			case hash::fnv1a("MATI"):
				child_node = std::make_unique<ResourceNode<MATI>>(ref.id, repo->getResource<MATI>(ref.id));
				break;
			case hash::fnv1a("MATE"):
				child_node = std::make_unique<ResourceNode<MATE>>(ref.id, repo->getResource<MATE>(ref.id));
				break;
			case hash::fnv1a("TEXT"):
				child_node = std::make_unique<ResourceNode<TEXT>>(ref.id, nullptr);//TODO: fix if TEXT is ever implemented
				break;
			case hash::fnv1a("TEXD"):
				child_node = std::make_unique <ResourceNode<TEXD>>(ref.id, repo->getResource<TEXD>(ref.id));
				break;
			default:
				continue;
			}

			if (child_node)
				root->insertChildNode(std::move(child_node));
		}

		for (auto& child : root->children()) {
			auto child_references = repo->getResourceReferences(child->id());
			parseAssetReferences(repo, child.get(), child_references);
		}
	}

	GlacierRenderAsset::GlacierRenderAsset(RuntimeId prim_id) : rig_(nullptr) {
		auto repo = ResourceRepository::instance();

		auto source_type = repo->getResourceType(prim_id);
		if (source_type != "PRIM")
			throw std::runtime_error("Can't construct Glacier3DModel from non-PRIM type resource.");

		auto prim = repo->getResource<PRIM>(prim_id);
		if (!prim)
			throw std::runtime_error("Failed to construct PRIM from repository data.");

		root = std::make_unique<ResourceNode<PRIM>>(prim_id, std::move(prim));
		for (auto& primitive : root->get<PRIM>()->resource()->primitives)
			meshes_.push_back(primitive.get());

		auto prim_references = repo->getResourceReferences(prim_id);

		parseAssetReferences(repo, root.get(), prim_references);

		//Material
		//TODO: This assumes that MATIs are sorted by their id. Not sure if that's really the case.
		for (const auto& child : root->children()) {
			auto mati = child->get_if<MATI>();
			if (mati)
				materials_.push_back(new Material(mati));
		}

		//Rig
		for (const auto& child : root->children()) {
			auto borg = child->get_if<BORG>();
			if (borg)
				rig_ = borg->resource().get();
		}

	}

	GlacierFormats::GlacierRenderAsset::GlacierRenderAsset(IMesh* mesh) {
		meshes_.push_back(mesh);
	}

	const std::vector<IMaterial*> GlacierRenderAsset::materials() const {
		return materials_;
	}

	const std::vector<IMesh*> GlacierRenderAsset::meshes() const {
		return meshes_;
	}
	const IRig* GlacierRenderAsset::rig() const {
		return rig_;
	}

	std::string GlacierRenderAsset::name()  const {
		if (!root)
			return "";
		return root->get<PRIM>()->resource()->name();
	}

