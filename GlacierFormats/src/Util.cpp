#include "Util.h"
#include "ResourceRepository.h"
#include "BORG.h"

std::unordered_map<std::string, int> GlacierFormats::Util::getBoneNameToIdMap(RuntimeId prim_id) {
	auto repo = ResourceRepository::instance();
	auto borg_ref = repo->getResourceReferences(prim_id, "BORG");
	if (!borg_ref.size())
		return std::unordered_map<std::string, int>();
	auto borg_resource = repo->getResource<BORG>(borg_ref.front().id);
	return borg_resource->getNameToBoneIndexMap();
}
