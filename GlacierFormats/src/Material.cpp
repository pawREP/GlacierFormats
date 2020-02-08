#include "Material.h"


namespace GlacierFormats {


	int Material::getTextureCount() const {
		const auto& mati = mati_node->resource();
		int tex_count = 0;

		for (int i = 0; i < mati->property_binders.size(); ++i) {
			const auto& binder = mati->property_binders[i];
			if (binder.name == "TEXT") {
				tex_count++;
			}
		}
		return tex_count;
	}

	std::vector<std::string> Material::getTextureNames() const {
		std::vector<std::string> texture_names(getTextureCount());

		const auto& mati = mati_node->resource();
		for (const auto& binder : mati->property_binders) {
			if (binder.name == "TEXT") {
				auto texture_name = binder.properties.at("NAME").get<std::string>();
				auto texture_index = binder.properties.at("TXID").get<int>();
				//TODO: some txids, return -1 here, why?
				if (texture_index == -1)
					continue;
				texture_names[texture_index] = std::string(texture_name);
			}
		}

		return texture_names;
	}

	std::unique_ptr<TEXD> Material::getTextureResourceByIndex(int idx) const { //TODO: I don't like the look of this whole function.

		const auto& mati_children = mati_node->children();

		auto running_index = 0;
		for (const auto& mati_child : mati_children) {
			if (mati_child->isType<TEXT>()) {
				if (running_index != idx) {
					running_index++;
					continue;
				}

				const auto& texd_children = mati_child->children();
				for (const auto& texd_child : texd_children) {
					if (texd_child->isType<TEXD>()) {
						const auto& texd = texd_child->get<TEXD>()->resource();
						return std::make_unique<TEXD>(*texd);
					}
				}
			}
		}
		return nullptr;
		//UNREACHABLE; //getTextureResourceByIndex(int idx) called with out of bounds index. fn isn't part of interface so this should never happen.
	}

	std::unique_ptr<TEXD> Material::getTextureMap(MATE::TextureType type) const {
		const auto is_mate_node = [](const std::unique_ptr<IResourceNode>& node) { return node->isType<MATE>(); };

		const auto& children = mati_node->children();
		const auto it = std::find_if(children.begin(), children.end(), is_mate_node);
		assert((it != children.end())); //All MATI nodes should have a MATE child.
		auto& mate = (*it)->get<MATE>()->resource();

		//TODO: TextureNames depends on TextureCount which counts TEXT not TEXD, this might be an issue.
		auto tex_names = getTextureNames();
		auto tex_index = 0;
		for (int i = 0; i < tex_names.size(); ++i) {
			if (mate->getTextureType(tex_names[i]) == type) {
				tex_index = i;
				break;
			}
		}

		return getTextureResourceByIndex(tex_index);
	}

	Material::Material(const ResourceNode<MATI>* mati_node) : mati_node(mati_node) {

	}

	std::unique_ptr<TEXD> Material::getDiffuseMap() const {
		return getTextureMap(MATE::TextureType::Albedo);
	}

	std::unique_ptr<TEXD> Material::getNormalMap() const {
		return getTextureMap(MATE::TextureType::Normal);
	}

	std::unique_ptr<TEXD> Material::getSpecularMap() const {
		return getTextureMap(MATE::TextureType::Specular);
	}


}