#pragma once

#include <cstdint>
#include <filesystem>
#include <unordered_map>
#include "BinaryWriter.hpp"
#include "BinaryReader.hpp"
#include "GlacierResource.h"
#include "ResourceReference.h"

namespace GlacierFormats {

	enum class RPKG_TYPE {
		BASE,
		PATCH
	};

	class PkgFile {
	public:
		struct Dependency {
			//The flags are poorly understood but they seem to indicate how the references are aquired/loaded. (streamed, install time acquired, sync, async ...)
			uint8_t flags;
			uint64_t runtimeID;
		};

	private:
		struct EntryInfo {
			uint64_t runtimeID;
			uint64_t data_offset;
			uint32_t compressed_size;
			bool is_encrypted;
			bool is_compressed;

			void read(BinaryReader& br);
			void write(BinaryWriter& bw) const;
		};

		struct EntryDescriptor {

			std::string type;
			uint32_t dependency_descriptor_size;
			uint32_t chunk_size;
			uint32_t size;
			uint32_t mem_size;
			uint32_t video_mem_size;
			uint32_t dependency_count;
			uint32_t dependency_table_ordering;
			std::vector<ResourceReference> references;

			void read(BinaryReader& br);
			void write(BinaryWriter& bw) const;
		};

	public:
		EntryInfo entry_info;
		EntryDescriptor entry_descriptor;
		char* rawData;
	};


	class RPKG {
		//TODO: Needs major refactor since the requirements for this class changed significantly since ResouceRespository was factored out
	private:
		std::unique_ptr<BinaryReader> br;

		RPKG_TYPE guessArchiveType(BinaryReader& br);

		size_t getEntryInfoSectionOffset() const;
		size_t getDataSectionOffset() const;
		size_t getEntryInfoSectionSize() const;
		size_t getEntryDescriptorSectionSize() const;

		void rebuildFileDataOffsets();

	public:

		std::string name;
		RPKG_TYPE archive_type;

		std::vector<uint64_t> deletion_list;
		std::vector<PkgFile> files;

		std::unordered_map<uint64_t, PkgFile*> runtime_id_map;

		RPKG(const std::filesystem::path& path);
		RPKG();

		bool operator<(const RPKG& rpkg) const;
		const PkgFile* getFileByRuntimeId(uint64_t runtime_id);


		void write(std::filesystem::path dst_path);
		void insertFile(uint64_t runtime_id, const std::string type, char* data, size_t data_size, const std::vector<ResourceReference>* references = nullptr);
		void overrideFile(uint64_t runtime_id, char* new_data, size_t new_data_size);

		/*Allocates memory at *dst_buf, fills it with decytped/decompressed data and returns the size
		of allocated memory. Freeing the allocated memory is responsibility of the called.
		*/
		size_t getFileData(const PkgFile& file, char** dst_buf);
		size_t getFileData(const uint64_t& id, char** dst_buf);
	};
}