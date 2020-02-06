#pragma once
#include "Rpkg.h"
#include "BinaryReader.hpp"
#include "BinaryWriter.hpp"
#include <cstdint>
#include <filesystem>
#include <algorithm>
#include <assert.h>
#include "..\thirdparty\lz4\include\lz4.h"
#include "Crypto.h"
#include "ResourceReference.h"
#include "ResourceRepository.h"

using namespace GlacierFormats;

#define TYPE_STR_LEN 4

	//TODO: not entirely safe, might read oob in very small files.
	RPKG_TYPE RPKG::guessArchiveType(BinaryReader& br) {
		if (br.size() <= 0x10) //return early on empty (header only) RPKGs. sh. dlc7.rpkg
			return RPKG_TYPE::BASE;

		br.seek(0x10);
		uint32_t size = br.read<uint32_t>();
		uint64_t rt = br.read<uint64_t>();
		uint64_t rt2 = br.read<uint64_t>();
		uint64_t rt3 = br.read<uint64_t>();
		uint64_t rt4 = br.read<uint64_t>();

		RPKG_TYPE ret = RPKG_TYPE::BASE;
		if (size != 0 &&
			(0xFF00000000000000 & rt) == 0 &&
			(0xFF00000000000000 & rt2) == 0 &&
			(0xFF00000000000000 & rt3) == 0 &&
			(0xFF00000000000000 & rt4) == 0) {
			ret = RPKG_TYPE::PATCH;
		}

		br.seek(0);
		return ret;
	}

	void PkgFile::EntryInfo::read(BinaryReader& br) {
		runtimeID = br.read<uint64_t>();
		data_offset = br.read<uint64_t>();
		uint32_t zsize = br.read<uint32_t>();
		compressed_size = zsize & 0x3FFFFFFF;
		is_compressed = !(compressed_size == 0);
		is_encrypted = (zsize & 0x80000000) != 0;
	}

	void PkgFile::EntryInfo::write(BinaryWriter& bw) const {
		bw.write(runtimeID);
		bw.write(data_offset);

		uint32_t zsize = compressed_size | (static_cast<int>(is_encrypted) << 0x1F);
		bw.write(zsize);
	}

	void PkgFile::EntryDescriptor::read(BinaryReader& br) {
		type = br.readString<4, BinaryReader::Endianness::LE>();
		dependency_descriptor_size = br.read<uint32_t>();
		chunk_size = br.read<uint32_t>();
		size = br.read<uint32_t>();
		mem_size = br.read<uint32_t>();
		video_mem_size = br.read<uint32_t>();
		dependency_count = 0;
		dependency_table_ordering = 0;

		if (dependency_descriptor_size > 0) {
			//The information content of chunk_ds is not well understood, could have more functionality than what's used here.
			uint32_t chunk_ds = br.read<uint32_t>();
			dependency_count = chunk_ds & 0x3FFFFFFF;
			dependency_table_ordering = chunk_ds >> 0x1E;

			references.resize(dependency_count);

#define TEST 0
#if TEST
			//int a = sizeof(Dependency);
			br.read<char>((char*)dependencies.data(), 9 * dependency_count); //wrong result, just a performance test
#else
			if (dependency_table_ordering == 3) {
				for (int i = 0; i < dependency_count; i++)
					references[i].flags = br.read<char>();
				for (int i = 0; i < dependency_count; i++)
					references[i].id = br.read<uint64_t>();
			}

			else {
				for (int i = 0; i < dependency_count; i++)
					references[i].id = br.read<uint64_t>();
				for (int i = 0; i < dependency_count; i++)
					references[i].flags = br.read<char>();
			}
#endif
		}
	};

	void PkgFile::EntryDescriptor::write(BinaryWriter& bw) const {

		bw.writeLEString(type);
		bw.write(dependency_descriptor_size);
		bw.write(chunk_size);
		bw.write(size);
		bw.write(mem_size);
		bw.write(video_mem_size);

		if (dependency_descriptor_size > 0) {
			//The information content of chunk_ds is not well understood, could have more functionality than what's used here.
			uint32_t chunk_ds = dependency_count | (dependency_table_ordering << 0x1E);
			bw.write(chunk_ds);

			if (dependency_table_ordering == 3) {
				for (int i = 0; i < dependency_count; i++)
					bw.write(references[i].flags);
				for (int i = 0; i < dependency_count; i++)
					bw.write(references[i].id);
			}
			else {
				for (int i = 0; i < dependency_count; i++)
					bw.write(references[i].id);
				for (int i = 0; i < dependency_count; i++)
					bw.write(references[i].flags);
			}
		}
	}

	RPKG::RPKG(BinaryReader* br_){
		br = std::unique_ptr<BinaryReader>(br_);

		archive_type = guessArchiveType(*br);

		//header.read(br, archive_type);
		//assert(("RPKG header magic is wrong", br.readString<TYPE_STR_LEN, Endianness::LE>() == "RPKG"));

		uint32_t file_count = br->read<uint32_t>();
		uint32_t entry_info_block_size = br->read<uint32_t>();
		uint32_t entry_descriptor_block_size = br->read<uint32_t>();
		uint32_t deletion_list_size = 0;
		if (archive_type == RPKG_TYPE::PATCH)
			deletion_list_size = br->read<uint32_t>();

		for (int i = 0; i < deletion_list_size; i++)
			deletion_list.push_back(br->read<long long>());

		files.resize(file_count);
		for (auto& file : files)
			file.entry_info.read(*br);
		for (auto& file : files)
			file.entry_descriptor.read(*br);

		for (auto& file : files) {
			runtime_id_map.insert({ file.entry_info.runtimeID, &file });
			file.rawData = nullptr;
		}


		//printf("%s", br.getCoverageReport().c_str());
	}

	const PkgFile* RPKG::getFileByRuntimeId(uint64_t runtime_id) {
		auto it = runtime_id_map.find(runtime_id);
		if (it == runtime_id_map.end())
			return nullptr;
		return it->second;
	}

	void RPKG::rebuildFileDataOffsets() {
		size_t current_data_offset = getDataSectionOffset();

		for (auto& f : files) {
			f.entry_info.data_offset = current_data_offset;
			if (f.entry_info.is_compressed) {
				current_data_offset += f.entry_info.compressed_size;
			}
			else {
				current_data_offset += f.entry_descriptor.size;
			}
		}
	};

	//transfers ownership of new_data to PkgFile with given runtime id, if it exists in the rpkg.
	void RPKG::overrideFile(uint64_t runtime_id, char* new_data, size_t new_data_size) {
		auto it = std::find_if(files.begin(), files.end(), [&](const PkgFile& f) {return f.entry_info.runtimeID == runtime_id; });
		if (it == files.end())
			throw; // tried to override file that doesn't exist in rpkg.

		it->rawData = new_data;
		it->entry_descriptor.size = new_data_size;
		it->entry_info.compressed_size = 0;
		it->entry_info.is_compressed = false;
		it->entry_info.is_encrypted = false;

		rebuildFileDataOffsets();
	}

	//TODO: Split RPKG completely into classes dedicated for reading/viewer and patch building. 
	//The requirements for each use case are so distinct it makes the current implementation needlessly cumbersome, slow, surprising and ugly. We can do better than that.

	//transfers ownership of data ptr to RPKG
	void RPKG::insertFile(uint64_t runtime_id, const std::string type, char* data, size_t data_size, const std::vector<ResourceReference>* references) {
		auto it = std::find_if(files.begin(), files.end(), [&](const PkgFile& f) {return f.entry_info.runtimeID == runtime_id; });
		if (it != files.end())
			return; //TODO: Re-evaluate what the best behaviour is for this case. Import routines of models with materials that reuse textures might trigger this path.

		PkgFile pkg = PkgFile();
		pkg.entry_info.is_compressed = false;
		pkg.entry_info.is_encrypted = false;
		pkg.entry_info.runtimeID = runtime_id;

		pkg.entry_descriptor.size = data_size;
		pkg.entry_descriptor.mem_size = data_size;
		pkg.entry_descriptor.video_mem_size = -1;
		pkg.entry_descriptor.type = type;
		pkg.rawData = data;

		std::vector<ResourceReference> default_references;
		if (!references) {
			default_references = ResourceRepository::instance()->getResourceReferences(runtime_id);
			references = &default_references;
		}
		pkg.entry_descriptor.dependency_count = references->size();
		for (const auto& dep : *references)
			pkg.entry_descriptor.references.push_back(dep);
		pkg.entry_descriptor.dependency_table_ordering = 3;
		pkg.entry_descriptor.dependency_descriptor_size = references->size() * 9 + 4;

		files.push_back(pkg);

		rebuildFileDataOffsets();
	}

	size_t RPKG::getEntryInfoSectionSize() const {
		return files.size() * 0x14;
	}

	//TODO: this is kind of expensive to compute, turn into member var?
	size_t RPKG::getEntryDescriptorSectionSize() const {
		size_t descriptor_section_size = 0;
		for (const auto& f : files)
			descriptor_section_size += f.entry_descriptor.dependency_descriptor_size + 6 * 4;
		return descriptor_section_size;
	}


	size_t RPKG::getDataSectionOffset() const {
		size_t data_section_offset = getEntryInfoSectionSize() + getEntryDescriptorSectionSize()
			+ 0x10 + deletion_list.size() * sizeof(uint64_t);

		if (archive_type == RPKG_TYPE::PATCH)
			data_section_offset += sizeof(int); //size of deletion_list_size member in header.

		return data_section_offset;
	}

	size_t RPKG::getEntryInfoSectionOffset() const {
		if (archive_type == RPKG_TYPE::BASE)
			return 0x10;

		//if patch:
		return 0x14 + deletion_list.size() * sizeof(uint64_t);

	}

	/*
	- Calculate final size
	- Write empty file with that size
	- iterate through PkgFiles and write out data
		- if files have data in rawData, this data is used, otherwise it's read from RPKG::br
		  only uncompressed unencrypted data is supported (encforced by insert file) so
		  EntryDescriptor.size contains size of rawData.
		- data_offsets can be updated while the data writing is going on
	*/
	void RPKG::write(std::filesystem::path dst_path) {

		BinaryWriter bw(dst_path);

		//write header
		uint32_t file_num = files.size();
		uint32_t entry_info_section_size = getEntryInfoSectionSize();
		uint32_t entry_descriptor_section_size = getEntryDescriptorSectionSize();
		uint32_t deletion_list_size = deletion_list.size();

		bw.write("GKPR", 4);
		//bw.writeString<Endianness::LE>(magic);
		bw.write(file_num);
		bw.write(entry_info_section_size);
		bw.write(entry_descriptor_section_size);
		if (archive_type == RPKG_TYPE::PATCH)
			bw.write(deletion_list_size);

		//write deletion list for patch files
		for (const auto& id : deletion_list) {
			bw.write(id);
		}

		//write data
		size_t current_data_offset = getDataSectionOffset();


		for (auto& f : files) {
			size_t data_size = 0;
			if (f.rawData == nullptr) {
				if (f.entry_info.is_compressed) {
					data_size = f.entry_info.compressed_size;
				}
				else {
					data_size = f.entry_descriptor.size;
				}

				f.rawData = new char[data_size];
				br->seek(f.entry_info.data_offset);
				br->read(f.rawData, data_size);

			}
			else {
				data_size = f.entry_descriptor.size;
			}
			//auto dif = current_data_offset - bw.tell();
			//if (dif != 0) {
			//	for (int i = 0; i < dif; ++i)
			//		bw.write('0');
			//	assert(bw.tell() == current_data_offset);
			//}
			bw.seek(current_data_offset);
			bw.write(f.rawData, data_size);
			//delete[] f.rawData;//TODO:yikes, Fix the leaking/ownership issue assoziated with RPKG file insertion.

			f.entry_info.data_offset = current_data_offset;
			current_data_offset += data_size;
		}

		bw.seek(getEntryInfoSectionOffset());

		for (const auto& f : files)
			f.entry_info.write(bw);

		for (const auto& f : files)
			f.entry_descriptor.write(bw);
	}


	//The RPKG default constructor is intendet to be used only for the 
	//construction of new patch archives.
	RPKG::RPKG() : name("newPatch"), archive_type(RPKG_TYPE::PATCH), br(nullptr){

	}

	//TODO: remove all the crypto/compression here
	size_t RPKG::getFileData(const PkgFile& file, char** dst_buf) {
		return 0;
		//br.seek(file.entry_info.data_offset);

		//*dst_buf = new char[file.entry_descriptor.size];

		//char* src_buf;
		//size_t src_buf_size;
		//if (file.entry_info.is_compressed) {
		//	src_buf_size = file.entry_info.compressed_size;
		//	src_buf = new char[src_buf_size];
		//	br.read(src_buf, src_buf_size);
		//	if (file.entry_info.is_encrypted) {
		//		Crypto::rpkgXCrypt(src_buf, src_buf_size);
		//	};
		//	if (LZ4_decompress_safe(src_buf, *dst_buf, file.entry_info.compressed_size, file.entry_descriptor.size) < 0)
		//		throw "Decompression error";
		//	delete[] src_buf;
		//}
		//else {
		//	br.read(*dst_buf, file.entry_descriptor.size);
		//	if (file.entry_info.is_encrypted) {
		//		Crypto::rpkgXCrypt(*dst_buf, file.entry_descriptor.size);
		//	};
		//}
		//
		//return file.entry_descriptor.size;
	}

	size_t RPKG::getFileData(const uint64_t& id, char** dst_buf) {
		const PkgFile* file = getFileByRuntimeId(id);
		size_t data_offset = file->entry_info.data_offset;
		size_t data_size = file->entry_descriptor.size;

		*dst_buf = new char[data_size];

		br->seek(data_offset);

		char* src_buf;
		size_t src_buf_size;
		if (file->entry_info.is_compressed) {
			src_buf_size = file->entry_info.compressed_size;
			src_buf = new char[src_buf_size];
			br->read(src_buf, src_buf_size);
			if (file->entry_info.is_encrypted) {
				Crypto::rpkgXCrypt(src_buf, src_buf_size);
			};
			if (LZ4_decompress_safe(src_buf, *dst_buf, file->entry_info.compressed_size, data_size) < 0)
				throw "Decompression error";
			delete[] src_buf;
		}
		else {
			br->read(*dst_buf, data_size);
			if (file->entry_info.is_encrypted) {
				Crypto::rpkgXCrypt(*dst_buf, data_size);
			};
		}

		return data_size;
	}

	bool RPKG::operator<(const RPKG& rpkg) const {
		return name < rpkg.name;
	}

