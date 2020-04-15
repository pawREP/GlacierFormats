#include "Texture.h"
#include "ResourceRepository.h"
#include "TEXT.h"
#include "TEXD.h"
#include "Util.h"

using namespace GlacierFormats;

RuntimeId getTEXTBackReference(RuntimeId texd_id) {
	auto repo = ResourceRepository::instance();

	if (repo->getResourceType(texd_id) != "TEXD")
		return 0;

	auto text_ids = repo->getIdsByType("TEXT");
	for (const auto& text_id : text_ids) {
		const auto refs = repo->getResourceReferences(text_id);
		for (const auto& ref : refs) {
			if (ref.id == texd_id)
				return text_id;
		}
	}

	return 0;
}

//Construct Texture based on TEXT or TEXD runtime id.
Texture::Texture(RuntimeId texture_resource_id) {
	const auto repo = ResourceRepository::instance();

	const auto texture_resource_type = repo->getResourceType(texture_resource_id);

	RuntimeId text_id = 0;
	RuntimeId texd_id = 0;
	if (texture_resource_type == "TEXD") {
		texd_id = texture_resource_id;
		text_id = getTEXTBackReference(texd_id);
	}
	else if (texture_resource_type == "TEXT") {
		text_id = texture_resource_id;

		const auto texd_ref = repo->getResourceReferences(text_id, "TEXD");
		if (texd_ref.size())
			texd_id = texd_ref.front().id;
	}
	else {
		return;
	}

	if(text_id)
		text = repo->getResource<TEXT>(text_id);
	if (texd_id)
		texd = repo->getResource<TEXD>(texd_id);
}

std::unique_ptr<Texture> Texture::loadFromTGAFile(const std::filesystem::path& path) {
	const auto repo = ResourceRepository::instance();

	RuntimeId texd_id = Util::runtimeIdFromFilePath(path);
	if (repo->getResourceType(texd_id) != "TEXD")
		return nullptr;

	auto texture = std::make_unique<Texture>(texd_id);
	texture->text = std::make_unique<TEXT>(*texture->texd);

	return texture;
}