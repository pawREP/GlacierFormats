#pragma once
#include <memory>
#include <vector>
#include <variant>
#include <filesystem>
#include "..\thirdparty\DirectXTex-master\DirectXTex\DirectXTex.h"
#include "..\thirdparty\DirectXTex-master\DirectXTex\DDS.h"
#include "GlacierResource.h"


namespace GlacierFormats {

	class BinaryReader;
	class BinaryWriter;


	class TEXD : public GlacierResource<TEXD>
	{
	public:

		enum class TextureType : uint16_t {
			Color = 0,
			Normal = 1,
			Height = 2,
			Compoundnormal = 3,
			Billboard = 4
		};

		enum class Format : uint16_t {
			R8G8 = 0x34,//Normals. very rarely used. Legcy? Only one such tex in chunk0;
			A8 = 0x42,//8-bit grayscale uncompressed. Not used on models?? Overlays
			DXT1 = 0x49,//Color maps, 1-bit alpha (mask). Many uses, color, normal, spec, rough maps on models and decals. Also used as masks.
			DXT5 = 0x4F,//Packed color, full alpha. Similar use as DXT5.
			BC4 = 0x52,//8-bit grayscale. Few or no direct uses on models?
			BC5 = 0x55,//2-channel normal maps
			BC7 = 0x5A//high res color + full alpha. Used for pretty much everything...
		};

#pragma pack(push,1)  
		struct TEXDHeader {
			uint16_t magic; //assert 1
			TextureType type;
			uint32_t file_size;
			uint32_t flags; //unused? assert 0;
			uint16_t width;
			uint16_t height;
			Format format;
			uint8_t mips_cnt;
			uint8_t default_mip;
			uint8_t interpret_as; //unused? assert 00
			uint8_t dimensions; //unused? assert 40.
			uint16_t mips_interpol_mode; //unused? assert 00 00
			uint32_t mips_data_sizes[0x0E]; //sizes are accumulative. Second mips size entry will be 1.25 * the first entry size etc. 
			uint32_t mips_data_sizes_dup[0x0E]; //duplicate of the array above, unknown why this is needed..

			uint32_t IA_data_size; //
			uint32_t IA_data_offset; //input assembler data?????
		};
#pragma pack(pop)

		TEXDHeader header;
		std::vector<char> pixels;

		TEXD();
		TEXD(const TEXD&);
		TEXD(BinaryReader& br, RuntimeId id);

		void serialize(BinaryWriter& bw);

		std::string name() const;

		void saveToDDSBuffer(DirectX::Blob& blob) const;
		bool saveToDDSFile(const std::filesystem::path& path) const;
		bool saveToTGAFile(const std::filesystem::path& path) const;
		bool saveToPNGFile(const std::filesystem::path& path) const;


		static std::unique_ptr<TEXD> loadFromTGAFile(const std::filesystem::path& path);
		static std::unique_ptr<TEXD> loadFromPNGFile(const std::filesystem::path& path);

	private:
		void headerSanityCheck();

		[[nodiscard]] HRESULT getScratchImage(DirectX::TexMetadata* meta, DirectX::ScratchImage& image) const;
	};

}