#pragma once
#include <vector>
#include "ResourceNode.h"
#include "IMaterial.h"

namespace GlacierFormats {

	//class Material : public IMaterial {
	//private:
	//	const ResourceNode<MATI>* mati_node;

	//public:
	//	Material(const ResourceNode<MATI>* mati_node);

	//	std::unique_ptr<TEXD> getTextureMap(MATE::TextureType type) const;

	//	int getTextureCount() const override final;
	//	std::vector<std::string> getTextureNames() const override final;

	//	std::unique_ptr<TEXD> getTextureResourceByIndex(int idx) const override final;
	//	//mapping texture resources to a specific texture type is a bit fragile and might be wrong in some cases. 
	//	std::unique_ptr<TEXD> getDiffuseMap() const override final;
	//	std::unique_ptr<TEXD> getNormalMap() const override final;
	//	std::unique_ptr<TEXD> getSpecularMap() const override final;
	//};
}

