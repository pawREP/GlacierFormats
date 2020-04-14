#pragma once

#include "..\thirdparty\DirectXTex-master\DirectXTex\DirectXTex.h"
#include "..\thirdparty\DirectXTex-master\DirectXTex\DDS.h"
#include "GlacierResource.h"

namespace GlacierFormats {

	class BinaryReader;
	class BinaryWriter;

	enum class TextureType : uint16_t {
		Color = 0,
		Normal = 1,
		Height = 2,
		Compoundnormal = 3,
		Billboard = 4,
		Unk = 0x0100
	};

	enum class TextureFormat : uint16_t {
		//R16G16B16A16 = 0x0A,
		R8G8B8A8 = 0x1C,
		R8G8 = 0x34,//Normals. very rarely used. Legacy? Only one such tex in chunk0;
		A8 = 0x42,//8-bit grayscale uncompressed. Not used on models?? Overlays
		DXT1 = 0x49,//Color maps, 1-bit alpha (mask). Many uses, color, normal, spec, rough maps on models and decals. Also used as masks.
		DXT5 = 0x4F,//Packed color, full alpha. Similar use as DXT5.
		BC4 = 0x52,//8-bit grayscale. Few or no direct uses on models?
		BC5 = 0x55,//2-channel normal maps
		BC7 = 0x5A//high res color + full alpha. Used for pretty much everything...
	};

#pragma pack(push, 1)  
	struct TextureHeader {
		uint16_t magic; //assert 1. ()
		TextureType type;
		uint32_t file_size;
		uint32_t flags; //unused? assert 0;
		uint16_t width;
		uint16_t height;
		TextureFormat format;
		uint8_t mips_cnt;
		uint8_t default_mip;
		uint8_t interpret_as; //unused? assert 00
		uint8_t dimensions; //unused? assert 40.
		uint16_t mips_interpol_mode; //unused? assert 00 00
		uint32_t mips_data_sizes[0x0E]; //sizes are accumulative. Second mips size entry will be 1.25 * the first entry size etc. 
		uint32_t mips_data_sizes_dup[0x0E]; //duplicate of the array above, unknown why this is needed..
		uint32_t texture_atlas_data_size; //
		uint32_t texture_atlas_data_offset; //Texture atlas data likely contains info about how the texture is partitioned. 
											//TODO: Further research needed. Texture atlas data is currently not reimported.
	};
#pragma pack(pop)

	template<typename T>
	class TextureResource : public GlacierResource<T> {
	public:
		TextureHeader header;
		std::vector<char> texture_atlas_data;
		std::vector<char> pixels;

		TextureResource();
		TextureResource(const TextureResource&);
		TextureResource(BinaryReader& br, RuntimeId id);

		void serialize(BinaryWriter& bw);

		std::string name() const;

		void saveToDDSBuffer(DirectX::Blob& blob) const;
		bool saveToDDSFile(const std::filesystem::path& path) const;
		bool saveToTGAFile(const std::filesystem::path& path) const;
		bool saveToPNGFile(const std::filesystem::path& path) const;

		static std::unique_ptr<T> loadFromTGAFile(const std::filesystem::path& path);
		static std::unique_ptr<T> loadFromPNGFile(const std::filesystem::path& path);

		[[nodiscard]] HRESULT getScratchImage(DirectX::TexMetadata* meta, DirectX::ScratchImage& image) const;

	private:
		void regularizeHeader(TextureHeader& header);

	};

	//TODO: Move and make non template
	template<typename Dst, typename Src>
	std::unique_ptr<Dst> generateTextFromTexd(const std::unique_ptr<Src>& texd);
}