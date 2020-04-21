#pragma once
#include "ResourceNode.h"
#include "IRenderAsset.h"
#include <vector>
#include <string>
#include <memory>

//Facade that provides convinient access to a complete 3D model, including geometry, rig and textures 
//as implemented in PRIM, BORG and TEXD respectively. 
namespace GlacierFormats {

	class GlacierRenderAsset : public IRenderAsset {
	private:
		std::unique_ptr<IResourceNode> root;

		std::vector<IMaterial*> materials_;
		std::vector<IMesh*> meshes_;
		IRig* rig_;
	public:

		GlacierRenderAsset(RuntimeId root_prim_id);
		GlacierRenderAsset(IMesh* mesh);

		void sortMeshes();

		//Interface
		const std::vector<IMaterial*> materials() const override final;
		const std::vector<IMesh*> meshes() const override final;
		const IRig* rig() const override final;
		std::string name()  const override final;

	};
}

//TODO: GlacierRenderAsset has some ownership inconsistencies. The two constructors result in difference ownership situations.
//Maybe split the non owning case off into something like GenericRednerResource.