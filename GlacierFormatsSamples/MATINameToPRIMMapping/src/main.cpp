#include "GlacierFormats.h"
#include <fstream>
#include <filesystem>
#include <map>
#include <unordered_set>

using namespace GlacierFormats;


int main(int argc, char** argv) {
	//Initilize GlacierFormats library
	GlacierInit();

	//Get ResourceRepository instance.
	auto repo = ResourceRepository::instance();

	//Get all runtime ids associated with PRIM resources.
	auto prim_ids = repo->getIdsByType("PRIM");

	std::unordered_map<RuntimeId, std::unique_ptr<MATI>> matis;
	std::multimap<std::string, RuntimeId> mati_to_prim_multimap;

	for (const auto& prim_id : prim_ids) {
		//Get all the references accociated with the prim id
		auto references = repo->getResourceReferences(prim_id);
		for (const auto& reference : references) {
			//Get the type of the referenced resource and check if it's "MATI". 
			//All PRIM resources have a least one MATI references
			auto reference_type = repo->getResourceType(reference.id);
			if (reference_type == "MATI") {
				//Parse mati resource and add mati_name->prim_id association to multi_map
				//auto mati_it = matis.find(reference.id);
				//if (mati_it == matis.end()) {
					auto mati = repo->getResource<MATI>(reference.id);
				//	mati_it = matis.insert({ reference.id, std::move(mati) }).first;
				//}
				mati_to_prim_multimap.insert({ mati->name, prim_id });
			}
		}
	}

	//Build text file from multimap
	auto out_path = std::filesystem::current_path() / "MatiNameToPrimIdMap.txt";
	std::ofstream ofs(out_path);
	
	std::string block_name = "";
	for (const auto& line : mati_to_prim_multimap) {
		const auto& mati_name = line.first;
		const auto& prim_id = line.second;
		if (block_name != mati_name) {
			ofs << mati_name << ":\n";
			block_name == mati_name;
		}
		ofs << "\t" << prim_id << "\n";
	}
	ofs.close();
}