#include <filesystem>
#include <fstream>
#include <thread>
#include <string>
#include "ResourceRepository.h"
#include "Crypto.h"
#include "PRIM.h"
#include "lz4.h"

using namespace GlacierFormats;

std::filesystem::path ResourceRepository::runtime_dir = std::filesystem::path();

	bool ResourceInfo::isEncrypted() const noexcept {
		return zsize & 0x80000000;
	}

	bool ResourceInfo::isCompressed() const noexcept {
		return compressedDataSize();
	}

	uint32_t ResourceInfo::compressedDataSize() const noexcept {
		return zsize & 0x3FFFFFFF;
	}

	std::vector<ResourceReference> ResourceHeader::getReferences() const {
		if (reference_chunk_size == 0)
			return std::vector<ResourceReference>();

		uint32_t chunk_ds;
		memcpy_s(&chunk_ds, sizeof(decltype(chunk_ds)), this->references, sizeof(decltype(chunk_ds)));

		auto dependency_count = chunk_ds & 0x3FFFFFFF;
		auto dependency_table_ordering = chunk_ds >> 0x1E;

		std::vector<ResourceReference> references(dependency_count);

		const RuntimeId* ids = nullptr;
		const char* flags = nullptr;

		if (dependency_table_ordering == 3) {
			flags = reinterpret_cast<const char*>(&this->references[4]);
			ids = reinterpret_cast<const RuntimeId*>(&flags[dependency_count]);
			for (unsigned int i = 0; i < dependency_count; ++i)
				references[i].flags = flags[i];
			for (unsigned int i = 0; i < dependency_count; ++i)
				references[i].id = ids[i];
		}
		else {
			ids = reinterpret_cast<const RuntimeId*>(&this->references[4]);
			flags = reinterpret_cast<const char*>(&ids[dependency_count]);
			for (unsigned int i = 0; i < dependency_count; ++i)
				references[i].id = ids[i];
			for (unsigned int i = 0; i < dependency_count; ++i)
				references[i].flags = flags[i];
		}

		return references;
	}

	ResourceRepositoryData::ResourceRepositoryData(const std::filesystem::path& runtime_path) {
		//TODO: Implement mechanism that excludes user defined patches.
		//Could be based on a unique key in the deletion list. 
		std::vector<std::filesystem::path> rpkg_file_paths;
		for (const auto& it : std::filesystem::directory_iterator(runtime_path)) {
			if (it.is_regular_file() && (it.path().extension().generic_string() == ".rpkg")) {
				rpkg_file_paths.push_back(it.path());
			}
		}
		std::sort(rpkg_file_paths.begin(), rpkg_file_paths.end(), std::less<std::filesystem::path>());

		for (const auto& s : rpkg_file_paths) {
			stream_names.push_back(s.stem().generic_string());
			streams.emplace_back(s, std::ifstream::binary);
			auto& ifs = streams.back();

			Header repo_header;
			ifs.read((char*)&repo_header, sizeof(Header));

			//TODO: It's probably better to check for the substring "patch" in the file name. The check below is a bit scuffed.
			if ((repo_header.deletion_block_id_count & 0xFFFF0000) == 0)
				ifs.seekg(sizeof(Header) + repo_header.deletion_block_id_count * sizeof(RuntimeId));
			else
				ifs.seekg(sizeof(Header) - 4);

			info_data.emplace_back(repo_header.entry_info_block_size / sizeof(ResourceInfo));
			ifs.read(reinterpret_cast<char*>(info_data.back().data()), repo_header.entry_info_block_size);

			header_data.emplace_back(repo_header.entry_descriptor_block_size);
			ifs.read(header_data.back().data(), repo_header.entry_descriptor_block_size);

		}
	}

	ResourceRepository::ResourceRepository(const std::filesystem::path& runtime_path) : ResourceRepositoryData(runtime_path) {
		for (int rpkg = 0; rpkg < info_data.size(); ++rpkg) {

			uint64_t header_data_offset = 0;
			auto header_data_base = header_data[rpkg].data();

			for (int entry_index = 0; entry_index < info_data[rpkg].size(); ++entry_index) {
				auto id = info_data[rpkg][entry_index].runtimeID;

				info[id] = &info_data[rpkg][entry_index];
				stream[id] = &streams[rpkg];

				auto header_entry = reinterpret_cast<ResourceHeader*>(&header_data_base[header_data_offset]);
				header[id] = header_entry;
				header_data_offset += sizeof(ResourceHeader) + header_entry->reference_chunk_size;
			}
		}
	}


	ResourceRepository* ResourceRepository::instance()
	{
		//Glacier runtime directory path has to be set during library startup
		if (runtime_dir.empty())
			return nullptr;
		static ResourceRepository repo(runtime_dir);
		return &repo;
	}

	bool ResourceRepository::contains(const RuntimeId& id) const noexcept {
		return info.find(id) != info.end();
	}

	std::string ResourceRepository::getResourceType(const RuntimeId& id) const {
		if (!contains(id))
			return "";
		std::string type(header.at(id)->type, 4);
		std::reverse(type.begin(), type.end());
		return type;
	}

	std::vector<ResourceReference> ResourceRepository::getResourceReferences(const RuntimeId& id) const {
		if (!contains(id))
			return std::vector<ResourceReference>();
		return header.at(id)->getReferences();
	}

	std::vector<ResourceReference> ResourceRepository::getResourceReferences(const RuntimeId& id, const std::string& type) const {
		if (!contains(id))
			return std::vector<ResourceReference>();
		std::vector<ResourceReference> references = header.at(id)->getReferences();
		std::vector<ResourceReference> references_of_type;//TODO: Maybe erase instead?
		for (const auto& reference : references) {
			auto t = getResourceType(reference.id);
			if (type == t)
				references_of_type.push_back(reference);
		}
		return references_of_type;
	}

	std::vector<RuntimeId> GlacierFormats::ResourceRepository::getIds() const {
		//TODO: This function is needlessly expensive, store id list in ResouceRepositoryData
		std::vector<RuntimeId> ids;
		ids.reserve(header.size());
		for (const auto& h : header)
			ids.push_back(h.first);
		return ids;
	}

	std::vector<RuntimeId> ResourceRepository::getIdsByType(std::string type) const {
		std::reverse(type.begin(), type.end());

		std::vector<RuntimeId> ids;
		for (const auto& h : header) {
			if (memcmp(h.second->type, type.data(), 4) == 0)
				ids.push_back(h.first);
		}
		return ids;
	}

	const std::string ResourceRepository::getSourceStreamName(RuntimeId id) const {
		if (!contains(id))
			return "";

		auto st = stream.at(id);
		auto it = std::find_if(streams.begin(), streams.end(), [&st](const std::ifstream& s) { return &s == st; });
		if (it == streams.end())
			return "";

		auto idx = it - streams.begin();
		return stream_names[idx];
	}

	uint64_t ResourceRepository::getResource(const RuntimeId& id, std::unique_ptr<char[]>& resource) const {
		if (!contains(id))
			return 0;

		auto src_stream = stream[id];
		auto src_info = info.at(id);
		auto src_header = header.at(id);

		src_stream->seekg(src_info->data_offset);

		auto uncompr_size = src_header->data_size;
		resource = std::make_unique<char[]>(uncompr_size);

		if (src_info->isCompressed()) {
			auto compr_size = src_info->compressedDataSize();
			auto compr_data = std::make_unique<char[]>(compr_size);
			src_stream->read(compr_data.get(), compr_size);

			if (src_info->isEncrypted()) {
				Crypto::rpkgXCrypt(compr_data.get(), compr_size);
			}

			if (LZ4_decompress_safe(compr_data.get(), resource.get(), compr_size, uncompr_size) < 0)
				throw "Decompression error";
		}
		else {
			src_stream->read(resource.get(), uncompr_size);
			if (src_info->isEncrypted()) {
				Crypto::rpkgXCrypt(resource.get(), uncompr_size);
			};
		}

		return uncompr_size;
	}
