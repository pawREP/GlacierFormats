#pragma once
#include <windows.h>
#include <vector>
#include <string>
#include <cinttypes>
#include <type_traits>
#include <filesystem>
#include <unordered_map>
#include "ResourceReference.h"

namespace GlacierFormats {

#pragma pack(push, 1)
	struct Header {
		char magic[4];
		uint32_t file_count;
		uint32_t entry_info_block_size;
		uint32_t entry_descriptor_block_size;
		uint32_t deletion_block_id_count;
	};

	struct ResourceInfo {
		uint64_t runtimeID;
		uint64_t data_offset;
		uint32_t zsize;

		[[nodiscard]] bool isEncrypted() const noexcept;
		[[nodiscard]] bool isCompressed() const noexcept;
		[[nodiscard]] uint32_t compressedDataSize() const noexcept;
	};

	struct ResourceHeader {
		char type[4];
		uint32_t reference_chunk_size;
		uint32_t states_chunk_size;
		uint32_t data_size;
		int32_t memory_size;
		int32_t video_memory_size;
		char references[0];
		//Zero-sized arrays are not standard conform but most major compilers support them. (clang, gcc, msvc)
		//This struct is never instatiated and only used as a view into continues memory. The reason why this 
		//is done in the first place comes down to the inefficient design of the RPKG format. Defering reference parsing
		//to access time improves the performance of ResourceRepository construction greatly.
		//See ResourceHeader::getReferences() for how the array is used.

		ResourceHeader() = delete;
		ResourceHeader(const ResourceHeader&) = delete;
		ResourceHeader(ResourceHeader&&) = delete;
		ResourceHeader& operator=(const ResourceHeader&) = delete;

		std::vector<ResourceReference> getReferences() const;
	};
#pragma pack(pop)

	class ResourceRepositoryData {
	protected:
		std::vector<std::ifstream> streams;
		std::vector<std::string> stream_names;
		std::vector<std::vector<ResourceInfo>> info_data;
		std::vector<std::vector<char>> header_data;

		ResourceRepositoryData(const std::filesystem::path& runtime_path);
	};

	//Provides transparent read access to repository resource data and references.
	class ResourceRepository : protected ResourceRepositoryData
	{
		//This class is optimized for fast construction. Most calculations are off-loaded to access routines.
		//TODO: The class uses some type punning that's techincally UB. This should be fixed once bit_cast is released with C++20.
	private:
		

		std::unordered_map<RuntimeId, const ResourceInfo*> info;
		std::unordered_map<RuntimeId, const ResourceHeader*> header;
		mutable std::unordered_map<RuntimeId, std::ifstream*> stream;

		ResourceRepository(const std::filesystem::path& runtime_path);
		ResourceRepository(const ResourceRepository&) = delete;
		ResourceRepository(ResourceRepository&&) = delete;
		ResourceRepository& operator=(const ResourceRepository&) = delete;

	public:
		static std::filesystem::path runtime_dir;
		static ResourceRepository* instance();

		[[nodiscard]] bool contains(const RuntimeId& id) const noexcept;

		template<typename T>
		std::unique_ptr<T> getResource(RuntimeId id) const;
		uint64_t getResource(const RuntimeId& id, std::unique_ptr<char[]>& resource) const;

		std::string getResourceType(const RuntimeId& id) const;
		std::vector<ResourceReference> getResourceReferences(const RuntimeId& id) const;
		std::vector<ResourceReference> getResourceReferences(const RuntimeId& id, const std::string& type) const;

		std::vector<RuntimeId> getIdsByType(const char type[4]) const;

		//Returns the name of the archive file that the resource with the given id is retreived from. 
		const std::string getSourceStreamName(RuntimeId id) const;
	};

	template<typename T>
	inline std::unique_ptr<T> ResourceRepository::getResource(RuntimeId id) const {
		if (!contains(id))
			return nullptr;
		std::unique_ptr<char[]> resource_data = nullptr;
		auto size = getResource(id, resource_data);
		return GlacierResource<T>::readFromBuffer(std::move(resource_data), size, id); //,< leak
	}

}