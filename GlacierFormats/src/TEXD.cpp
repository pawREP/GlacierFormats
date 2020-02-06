#include "TEXD.h"
#include <cinttypes>
#include "..\thirdparty\DirectXTex-master\DirectXTex\DirectXTex.h"
#include "..\thirdparty\DirectXTex-master\DirectXTex\DDS.h"
#include "BinaryReader.hpp"
#include "BinaryWriter.hpp"
#include "ResourceRepository.h"
#include "EncodingDevice.h"

using namespace GlacierFormats;

	namespace {
		[[nodiscard]] bool initilizeTEXDHeaderById(RuntimeId id, TEXD::TEXDHeader& header) {
			auto repo = ResourceRepository::instance();
			auto repository_default_texd = repo->getResource<TEXD>(id);
			if (!repository_default_texd)
				return false;
			header = repository_default_texd->header;
			return true;
		}
	}

	TEXD::TEXD() : GlacierResource<TEXD>(0xFFFFFFFFFFFFFFFF) {
		//TODO: This shouldn't really exist. Fix the code that makes use of the default constructor.
	}

	TEXD::TEXD(const TEXD& other) : GlacierResource<TEXD>(other.id), header(other.header), pixels(other.pixels) {

	}

	TEXD::TEXD(BinaryReader& br, RuntimeId id) : GlacierResource<TEXD>(id) {
		header = br.read<TEXDHeader>();
		headerSanityCheck();

		//This pixel_data_size doesn't work for TEXT files for some reason. Try filesize-headersize.
		//auto pixel_data_size = header->mips_data_sizes[header->mips_cnt - 1];//the mips_data_sizes array is accumulative;
		auto pixel_data_size = br.size() - br.tell();
		pixels.resize(pixel_data_size);
		br.read(pixels.data(), pixels.size());
	}

	void TEXD::headerSanityCheck() {
		assert(("TEXDHeader::magic != 0x0001", header.magic == 0x0001));
		assert(("TEXDHeader::flags != 0x00", header.flags == 0x00));
		assert(("TEXDHeader::interpret_as != 0x00", header.interpret_as == 0x00));//Used in TEXT files
		assert(("TEXDHeader::dimensions != 0x40", header.dimensions == 0x40));//Used in TEXT files
		assert(("TEXDHeader::mips_interpol_mode != 0x00", header.mips_interpol_mode == 0x00));//Used in TEXT files
		assert(("TEXDHeader::mips_cnt <= 1", header.mips_cnt > 1));//Used in TEXT files
	}

	HRESULT TEXD::getScratchImage(DirectX::TexMetadata* meta, DirectX::ScratchImage& image) const
	{
		DirectX::Blob blob;
		saveToDDSBuffer(blob);

		return DirectX::LoadFromDDSMemory(blob.GetBufferPointer(), blob.GetBufferSize(), 0, meta, image);
	}

	std::string TEXD::name() const {
		return GlacierResource::name();
	}

	void TEXD::saveToDDSBuffer(DirectX::Blob& blob) const
	{
		DirectX::DDS_HEADER dds_header{};
		DirectX::DDS_HEADER_DXT10 dds_header_dxt10{};

		//TODO: The init might have to change based on params in the texd header;
		dds_header.size = 124;
		dds_header.flags = DDS_HEADER_FLAGS_TEXTURE | DDS_HEADER_FLAGS_MIPMAP;
		dds_header.height = header.height;
		dds_header.width = header.width;
		dds_header.pitchOrLinearSize = 0;//as per doc, most programs ignore this value anyway.
		dds_header.depth = 0;
		dds_header.mipMapCount = header.mips_cnt;
		std::fill(dds_header.reserved1, &dds_header.reserved1[11], 0);
		dds_header.caps = DDS_SURFACE_FLAGS_TEXTURE | DDS_SURFACE_FLAGS_MIPMAP;
		dds_header.caps2 = NULL;
		dds_header.caps3 = NULL;
		dds_header.caps4 = NULL;
		dds_header.reserved2 = NULL;

		switch (header.format) {
		case Format::R8G8:
			dds_header.ddspf = DirectX::DDSPF_R8G8_B8G8;
			break;
		case Format::A8:
			assert((header.type == TextureType::Color));
			dds_header.ddspf = DirectX::DDSPF_A8;
			break;
		case Format::DXT1:
			dds_header.ddspf = DirectX::DDSPF_DXT1;
			break;
		case Format::DXT5:
			dds_header.ddspf = DirectX::DDSPF_DXT5;
			break;
		case Format::BC4:
			dds_header.ddspf = DirectX::DDSPF_BC4_UNORM;
			break;
		case Format::BC5:
			dds_header.ddspf = DirectX::DDSPF_BC5_UNORM;
			break;
		case Format::BC7:
			//throw;
			dds_header.ddspf = DirectX::DDSPF_DX10;
			dds_header_dxt10.dxgiFormat = DXGI_FORMAT_BC7_UNORM;
			dds_header_dxt10.resourceDimension = DirectX::DDS_DIMENSION_TEXTURE2D;
			dds_header_dxt10.miscFlag = 0;
			dds_header_dxt10.arraySize = 1;
			dds_header_dxt10.miscFlags2 = 0;
			break;
		}

		auto dds_file_buffer_size = 0;
		dds_file_buffer_size += sizeof(DirectX::DDS_MAGIC);
		dds_file_buffer_size += sizeof(DirectX::DDS_HEADER);
		if (header.format == Format::BC7)
			dds_file_buffer_size += sizeof(DirectX::DDS_HEADER_DXT10);
		dds_file_buffer_size += pixels.size();

		blob.Initialize(dds_file_buffer_size);
		char* dds_file_buffer = reinterpret_cast<char*>(blob.GetBufferPointer());

		//TODO: replace this mess with a stream writer;
		int buf_off = 0;
		memcpy_s(&dds_file_buffer[buf_off], sizeof(DirectX::DDS_MAGIC), &DirectX::DDS_MAGIC, sizeof(DirectX::DDS_MAGIC));
		buf_off += sizeof(DirectX::DDS_MAGIC);

		memcpy_s(&dds_file_buffer[buf_off], sizeof(dds_header), &dds_header, sizeof(dds_header));
		buf_off += sizeof(dds_header);

		if (header.format == Format::BC7) {
			memcpy_s(&dds_file_buffer[buf_off], sizeof(dds_header_dxt10), &dds_header_dxt10, sizeof(dds_header_dxt10));
			buf_off += sizeof(dds_header_dxt10);
		}

		memcpy_s(&dds_file_buffer[buf_off], pixels.size(), pixels.data(), pixels.size());
	}

	bool TEXD::saveToDDSFile(const std::filesystem::path& path) const
	{
		DirectX::Blob dds_buffer;
		saveToDDSBuffer(dds_buffer);

		DirectX::TexMetadata meta;
		DirectX::ScratchImage image;
		auto hr = DirectX::LoadFromDDSMemory(dds_buffer.GetBufferPointer(), dds_buffer.GetBufferSize(), 0, &meta, image);
		if (FAILED(hr))
			return false;

		std::wstring wpath = path.generic_wstring();
		hr = DirectX::SaveToDDSFile(image.GetImages(), image.GetImageCount(), meta, 0, wpath.c_str());
		if (FAILED(hr))
			return false;
		return true;
	}

	[[nodiscard]] DXGI_FORMAT TEXDFormatToDXGIFormat(TEXD::Format format) {
		switch (format) {
		case TEXD::Format::A8:
			return DXGI_FORMAT_A8_UNORM;
		case TEXD::Format::R8G8:
			return DXGI_FORMAT_R8G8_UNORM;
		case TEXD::Format::DXT1:
			return DXGI_FORMAT_BC1_UNORM;
		case TEXD::Format::DXT5:
			return DXGI_FORMAT_BC3_UNORM;
		case TEXD::Format::BC7:
			return DXGI_FORMAT_BC7_UNORM;
		case TEXD::Format::BC4:
			return DXGI_FORMAT_BC4_UNORM;
		case TEXD::Format::BC5:
			return DXGI_FORMAT_BC5_UNORM;
		default:
			throw;
		}
	}

	[[nodiscard]] HRESULT DXCompress(const DirectX::ScratchImage& in, DirectX::ScratchImage& out, DXGI_FORMAT format) {
		HRESULT hr;
		hr = DirectX::Compress(EncodingDevice(),in.GetImages(), in.GetImageCount(), in.GetMetadata(), format, DirectX::TEX_COMPRESS_PARALLEL, DirectX::TEX_THRESHOLD_DEFAULT, out);
		if (!FAILED(hr))
			return hr;
		//Try CPU encoder if GPU fails;
		return DirectX::Compress(in.GetImages(), in.GetImageCount(), in.GetMetadata(), format, DirectX::TEX_COMPRESS_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT, out);

	}

	[[nodiscard]] HRESULT DXCompress(const DirectX::ScratchImage& in, DirectX::ScratchImage& out, TEXD::Format format) {
		return DXCompress(in, out, TEXDFormatToDXGIFormat(format));
	}

	bool TEXD::saveToTGAFile(const std::filesystem::path& dir) const
	{
		HRESULT hr;

		DirectX::TexMetadata meta;
		DirectX::ScratchImage orig_image;
		DirectX::ScratchImage conv_image;

		DirectX::ScratchImage* export_image = &orig_image;

		hr = getScratchImage(&meta, orig_image);
		if (FAILED(hr))
			return false;

		switch (header.format) {
		case Format::A8:
			break;
		case Format::R8G8:
			hr = DirectX::Convert(*orig_image.GetImage(0, 0, 0), DXGI_FORMAT_R8G8B8A8_UNORM, 0, 0.f, conv_image);
			if (FAILED(hr))
				return false;

			//Fix blue channel
			for (int i = 2; i < conv_image.GetPixelsSize(); i += 4)
				conv_image.GetPixels()[i] = 0xFF;
			break;
		case Format::DXT1://Color maps, 1-bit alpha (mask)
		case Format::DXT5://Packed color, full alpha
		case Format::BC7://High res color + full alpha
			hr = DirectX::Decompress(*orig_image.GetImage(0, 0, 0), DXGI_FORMAT_R8G8B8A8_UNORM, conv_image);
			if (FAILED(hr))
				return false;
			break;
		case Format::BC4://8-bit grayscale
			hr = DirectX::Decompress(*orig_image.GetImage(0, 0, 0), DXGI_FORMAT_A8_UNORM, conv_image);
			if (FAILED(hr))
				return false;
			break;
		case Format::BC5://2-channel normal maps
			hr = DirectX::Decompress(*orig_image.GetImage(0, 0, 0), DXGI_FORMAT_R8G8B8A8_UNORM, conv_image);
			if (FAILED(hr))
				return false;

			//Fix blue channel
			for (int i = 2; i < conv_image.GetPixelsSize(); i += 4)
				conv_image.GetPixels()[i] = 0xFF;
			break;
		default:
			throw;
		}

		switch (header.format) {
		case Format::A8:
			export_image = &orig_image;
			break;
		case Format::R8G8:
		case Format::DXT1://Color maps, 1-bit alpha (mask)
		case Format::DXT5://Packed color, full alpha
		case Format::BC7://High res color + full alpha
		case Format::BC4://8-bit grayscale
		case Format::BC5://2-channel normal maps
			export_image = &conv_image;
			break;
		default:
			throw;
		}

		auto full_path = dir /  (name() + ".tga");
		std::wstring wpath = full_path.generic_wstring();
		hr = DirectX::SaveToTGAFile(*export_image->GetImage(0, 0, 0), wpath.c_str(), &meta);
		if (FAILED(hr))
			return false;

		return true;
	}

	bool TEXD::saveToPNGFile(const std::filesystem::path& path) const	{
		HRESULT hr;	//TODO: factor common code between PNG and TGA export

		DirectX::TexMetadata meta;
		DirectX::ScratchImage orig_image;
		DirectX::ScratchImage conv_image;

		DirectX::ScratchImage* export_image = &orig_image;

		hr = getScratchImage(&meta, orig_image);
		if (FAILED(hr))
			return false;

		switch (header.format) {
		case Format::A8:
			break;
		case Format::R8G8:
			hr = DirectX::Convert(*orig_image.GetImage(0, 0, 0), DXGI_FORMAT_R8G8B8A8_UNORM, 0, 0.f, conv_image);
			if (FAILED(hr))
				return false;

			//Fix blue channel
			for (int i = 2; i < conv_image.GetPixelsSize(); i += 4)
				conv_image.GetPixels()[i] = 0xFF;
			break;
		case Format::DXT1://Color maps, 1-bit alpha (mask)
		case Format::DXT5://Packed color, full alpha
		case Format::BC7://High res color + full alpha
			hr = DirectX::Decompress(*orig_image.GetImage(0, 0, 0), DXGI_FORMAT_R8G8B8A8_UNORM, conv_image);
			if (FAILED(hr))
				return false;
			break;
		case Format::BC4://8-bit grayscale
			hr = DirectX::Decompress(*orig_image.GetImage(0, 0, 0), DXGI_FORMAT_A8_UNORM, conv_image);
			if (FAILED(hr))
				return false;
			break;
		case Format::BC5://2-channel normal maps
			hr = DirectX::Decompress(*orig_image.GetImage(0, 0, 0), DXGI_FORMAT_R8G8B8A8_UNORM, conv_image);
			if (FAILED(hr))
				return false;

			//Fix blue channel
			for (int i = 2; i < conv_image.GetPixelsSize(); i += 4)
				conv_image.GetPixels()[i] = 0xFF;
			break;
		default:
			throw;
		}

		switch (header.format) {
		case Format::A8:
			export_image = &orig_image;
			break;
		case Format::R8G8:
		case Format::DXT1://Color maps, 1-bit alpha (mask)
		case Format::DXT5://Packed color, full alpha
		case Format::BC7://High res color + full alpha
		case Format::BC4://8-bit grayscale
		case Format::BC5://2-channel normal maps
			export_image = &conv_image;
			break;
		default:
			throw;
		}

		auto full_path = path / name() / ".png";
		std::wstring wpath = full_path.generic_wstring();
		REFGUID container_format = DirectX::GetWICCodec(DirectX::WIC_CODEC_PNG);
		hr = DirectX::SaveToWICFile(*export_image->GetImage(0,0,0), 0, container_format, wpath.c_str());
		if (FAILED(hr))
			return false;

		return true;
	}

	void TEXD::serialize(BinaryWriter& bw) {
		bw.write(header);
		bw.write(pixels.data(), pixels.size());
	}



	std::unique_ptr<TEXD> TEXD::loadFromTGAFile(const std::filesystem::path& path) {
		HRESULT hr;

		DirectX::TexMetadata meta;
		DirectX::ScratchImage input_image;
		DirectX::ScratchImage mip_chain;

		std::wstring wstr = path.generic_wstring();
		hr = DirectX::LoadFromTGAFile(wstr.c_str(), &meta, input_image);
		if (FAILED(hr))
			return nullptr;

		auto texd = std::make_unique<TEXD>();

		auto stem = std::filesystem::path(path).stem().generic_string();
		RuntimeId id = stem;
		//try {id = std::stoull(stem, nullptr, 16);} catch (std::exception e) { return nullptr; }
		if (!initilizeTEXDHeaderById(id, texd->header))
			return nullptr;

		//TODO: Input images that don't match the size of the original get resized to the original dimensions.
		//This is likely not necessary if the header is updated properly. There is some commented out code below tht attempts this.
		if ((input_image.GetImage(0, 0, 0)->width != texd->header.width) || (input_image.GetImage(0, 0, 0)->height != texd->header.height)) {
			DirectX::ScratchImage resized_img;
			DirectX::Resize(*input_image.GetImage(0, 0, 0), texd->header.width, texd->header.height, DirectX::TEX_FILTER_DEFAULT, resized_img);
			hr = DirectX::GenerateMipMaps(*resized_img.GetImage(0, 0, 0), 0, texd->header.mips_cnt, mip_chain);
		}
		else {
			hr = DirectX::GenerateMipMaps(*input_image.GetImage(0, 0, 0), 0, texd->header.mips_cnt, mip_chain);
		}
		if (FAILED(hr))
			return nullptr;

		DirectX::ScratchImage out_image;
		switch (texd->header.format) {
		case Format::A8:
			out_image = std::move(mip_chain);
			break;
		case Format::R8G8:
		case Format::DXT1:
		case Format::DXT5:
		case Format::BC7:
		case Format::BC4:
			hr = DXCompress(mip_chain, out_image, texd->header.format);
			if (FAILED(hr))
				return nullptr;
			break;
		case Format::BC5:
			for (int i = 2; i < mip_chain.GetPixelsSize(); i += 4)
				mip_chain.GetPixels()[i] = 0xFF;

			hr = DXCompress(mip_chain, out_image, texd->header.format);
			if (FAILED(hr))
				return nullptr;
			break;
		}

		assert(out_image.GetImageCount() <= (sizeof(TEXDHeader::mips_data_sizes) / sizeof(TEXDHeader::mips_data_sizes[0])));

		texd->pixels.resize(out_image.GetPixelsSize());
		memcpy_s(texd->pixels.data(), out_image.GetPixelsSize(), out_image.GetPixels(), out_image.GetPixelsSize());


		//update header;
		//texd->header.file_size = sizeof(TEXDHeader) + texd->pixels.size();
		//texd->header.width = out_image.GetImage(0,0,0)->width;
		//texd->header.height = out_image.GetImage(0,0,0)->height;
		//texd->header.mips_cnt = out_image.GetImageCount();

		//for (int i = 0; i < out_image.GetImageCount(); ++i) {
		//	texd->header.mips_data_sizes[i] = 0;
		//	texd->header.mips_data_sizes_dup[i] = 0;
		//}

		//const auto block_width = 4; //TODO: Bug! only true for block compressed images;

		//texd->header.mips_data_sizes[0] = out_image.GetImage(0, 0, 0)->rowPitch * out_image.GetImage(0, 0, 0)->height / block_width;
		//for (int i = 1; i < out_image.GetImageCount(); ++i) {
		//	texd->header.mips_data_sizes[i] =
		//		texd->header.mips_data_sizes[i - 1] +
		//		out_image.GetImage(i, 0, 0)->height * out_image.GetImage(i, 0, 0)->rowPitch / block_width;
		//}
		//for (int i = 0; i < out_image.GetImageCount(); ++i)
		//	texd->header.mips_data_sizes_dup[i] = texd->header.mips_data_sizes[i];
		
		return texd;
	}

	std::unique_ptr<TEXD> TEXD::loadFromPNGFile(const std::filesystem::path& path) {
		//TODO: factor function, remove dup code from PNG/TGA import
		HRESULT hr;

		DirectX::TexMetadata meta;
		DirectX::ScratchImage input_image;
		DirectX::ScratchImage mip_chain;

		std::wstring wstr = path.generic_wstring();
		hr = DirectX::LoadFromWICFile(wstr.c_str(), 0, &meta, input_image);
		//hr = DirectX::LoadFromTGAFile(wstr.c_str(), &meta, input_image);
		if (FAILED(hr))
			return nullptr;

		auto texd = std::make_unique<TEXD>();

		auto stem = std::filesystem::path(path).stem().generic_string();
		RuntimeId id = stem;
		//try { id = std::stoull(stem, nullptr, 16); }
		//catch (std::exception e) { return nullptr; }
		if (!initilizeTEXDHeaderById(id, texd->header))
			return nullptr;

		hr = DirectX::GenerateMipMaps(*input_image.GetImage(0, 0, 0), 0, texd->header.mips_cnt, mip_chain);
		if (FAILED(hr))
			return nullptr;

		DirectX::ScratchImage out_image;
		switch (texd->header.format) {
		case Format::A8:
			out_image = std::move(mip_chain);
			break;
		case Format::R8G8:
		case Format::DXT1:
		case Format::DXT5:
		case Format::BC7:
		case Format::BC4:
			hr = DXCompress(mip_chain, out_image, texd->header.format);
			if (FAILED(hr)) {
				//Try CPU encoder as backup
				hr = DirectX::Compress(mip_chain.GetImages(),
					mip_chain.GetImageCount(),
					mip_chain.GetMetadata(),
					TEXDFormatToDXGIFormat(texd->header.format),
					DirectX::TEX_COMPRESS_DEFAULT,
					DirectX::TEX_THRESHOLD_DEFAULT,
					out_image);
				if (FAILED(hr))
					return nullptr;
			}
			break;
		case Format::BC5:
			for (int i = 2; i < mip_chain.GetPixelsSize(); i += 4)
				mip_chain.GetPixels()[i] = 0xFF;

			hr = DXCompress(mip_chain, out_image, texd->header.format);
			if (FAILED(hr)) {
				//Try CPU encoder as backup
				hr = DirectX::Compress(mip_chain.GetImages(),
					mip_chain.GetImageCount(),
					mip_chain.GetMetadata(),
					TEXDFormatToDXGIFormat(texd->header.format),
					DirectX::TEX_COMPRESS_DEFAULT,
					DirectX::TEX_THRESHOLD_DEFAULT,
					out_image);
				if (FAILED(hr))
					return nullptr;
			}
			break;
		}

		texd->pixels.resize(out_image.GetPixelsSize());
		memcpy_s(texd->pixels.data(), out_image.GetPixelsSize(), out_image.GetPixels(), out_image.GetPixelsSize());
		return texd;
	}
