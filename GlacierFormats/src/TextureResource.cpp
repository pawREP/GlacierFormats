#include "TextureResource.h"
#include "BinaryReader.hpp"
#include "BinaryWriter.hpp"
#include "EncodingDevice.h"
#include "TEXD.h"
#include "TEXT.h"
#include "ResourceRepository.h"
#include <type_traits>

using namespace GlacierFormats;

//Returns the texture size scale factor between corresponding TEXT and TEXD resources.
//dimension(TEXD) / scale == dimensions(TEXD)
size_t textScaleFactor(int texd_w, int texd_h) {
	switch (texd_w * texd_h) {
	case 32768:
	case 65536:
		return 2;
	case 131072:
	case 262144:
		return 4;
	case 524288:
	case 1048576:
		return 8;
	case 2097152:
	case 4194304:
		return 16;
	case 8388608:
	case 16777216:
		return 32;
	default:
		throw;
	}
}

[[nodiscard]] bool initilizeTEXHeaderById(RuntimeId id, TextureHeader& header) {
	auto repo = ResourceRepository::instance();
	auto repository_default_texd = repo->getResource<TEXD>(id);
	if (!repository_default_texd)
		return false;
	header = repository_default_texd->header;

	return true;
}

//Returns max number of mips supported for given image dimensions.
size_t maxMipsCount(size_t width, size_t height) {
	size_t mipLevels = 1;

	while (height > 1 || width > 1) {
		if (height > 1)
			height >>= 1;

		if (width > 1)
			width >>= 1;

		++mipLevels;
	}
	return mipLevels;
}

//Returns total number of bytes required to contain an image with the given dimensions, mips levels and format.
size_t getTotalPixelsSize(int w, size_t h, int mips_levels, DXGI_FORMAT fmt) {
	size_t totalPixelSize = 0;

	for (size_t level = 0; level < mips_levels; ++level) {
		size_t rowPitch, slicePitch;
		HRESULT hr = DirectX::ComputePitch(fmt, w, h, rowPitch, slicePitch, 0);
		if (FAILED(hr))
			throw std::runtime_error("Unexpected failure in ComputePitch");

		totalPixelSize += uint64_t(slicePitch);

		if (h > 1)
			h >>= 1;

		if (w > 1)
			w >>= 1;
	}
	return totalPixelSize;
}

//Converts TextureFormat to DXGI_FORMAT_XXX
[[nodiscard]] DXGI_FORMAT toDxgiFormat(TextureFormat format) {
	switch (format) {
	case TextureFormat::R8G8B8A8:
		return DXGI_FORMAT_R8G8B8A8_UNORM;
	case TextureFormat::A8:
		return DXGI_FORMAT_A8_UNORM;
	case TextureFormat::R8G8:
		return DXGI_FORMAT_R8G8_UNORM;
	case TextureFormat::DXT1:
		return DXGI_FORMAT_BC1_UNORM;
	case TextureFormat::DXT5:
		return DXGI_FORMAT_BC3_UNORM;
	case TextureFormat::BC7:
		return DXGI_FORMAT_BC7_UNORM;
	case TextureFormat::BC4:
		return DXGI_FORMAT_BC4_UNORM;
	case TextureFormat::BC5:
		return DXGI_FORMAT_BC5_UNORM;
	default:
		return DXGI_FORMAT_UNKNOWN;
	}
}

template<typename T>
TextureResource<T>::TextureResource() : GlacierResource<T>(0xFFFFFFFFFFFFFFFF) {

}

template<typename T>
TextureResource<T>::TextureResource(const TextureResource& other) : 
	GlacierResource<T>(other.id), header(other.header), 
	texture_atlas_data(other.texture_atlas_data), 
	pixels(other.pixels) {
}

template<typename T>
TextureResource<T>::TextureResource(BinaryReader& br, RuntimeId id) : GlacierResource<T>(id){
	header = br.read<TextureHeader>();

	if (header.texture_atlas_data_size) {
		texture_atlas_data.resize(header.texture_atlas_data_size);
		br.read(texture_atlas_data.data(), header.texture_atlas_data_size);
	}

	auto pixel_data_size = br.size() - br.tell();
	pixels.resize(pixel_data_size);
	br.read(pixels.data(), pixels.size());

	if constexpr (std::is_same_v<T, TEXT>) {
		//TET header data corresponds to the pixel data found in the TEXD dependency, not the TEXT file itself.
		auto scale = textScaleFactor(header.width, header.height);
		auto text_w = header.width / scale;
		auto text_h = header.height / scale;
		auto text_mips_levels = maxMipsCount(text_w, text_h);

		auto soll_size = getTotalPixelsSize(text_w, text_h, text_mips_levels, toDxgiFormat(header.format));
		auto is_size = pixels.size();
		if (is_size != soll_size)
			throw std::runtime_error("Pixel size mismatch");
	}
}

TextureHeader generateTexHeader(TextureType type, int width, int height, TextureFormat fmt) {
	TextureHeader h;
	h.magic = 1;
	h.type = type;

	auto mips_count = maxMipsCount(width, height);
	auto dxgi_format = toDxgiFormat(fmt);
	h.file_size = getTotalPixelsSize(width, height, mips_count, dxgi_format);

	h.flags = 0;
	h.width = width;
	h.height = height;
	h.format = fmt;
	h.mips_cnt = mips_count;
	h.default_mip = 1;
	h.interpret_as = 0;
	h.dimensions = 0x40;
	h.mips_interpol_mode = 0;
	std::fill_n(h.mips_data_sizes, 0x0E, 0);
	std::fill_n(h.mips_data_sizes_dup, 0x0E, 0);
	h.texture_atlas_data_size = 0;
	h.texture_atlas_data_offset = 0x90;

	return h;
}

void initilizeDdsHeader(const TextureHeader& header, DirectX::DDS_HEADER& dds_header, DirectX::DDS_HEADER_DXT10& dds_header_dxt10) {
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
	case TextureFormat::R8G8:
		dds_header.ddspf = DirectX::DDSPF_R8G8_B8G8;
		break;
	case TextureFormat::A8:
		assert((header.type == TextureType::Color));
		dds_header.ddspf = DirectX::DDSPF_A8;
		break;
	case TextureFormat::DXT1:
		dds_header.ddspf = DirectX::DDSPF_DXT1;
		break;
	case TextureFormat::DXT5:
		dds_header.ddspf = DirectX::DDSPF_DXT5;
		break;
	case TextureFormat::BC4:
		dds_header.ddspf = DirectX::DDSPF_BC4_UNORM;
		break;
	case TextureFormat::BC5:
		dds_header.ddspf = DirectX::DDSPF_BC5_UNORM;
		break;
	case TextureFormat::BC7:
		//throw;
		dds_header.ddspf = DirectX::DDSPF_DX10;
		dds_header_dxt10.dxgiFormat = DXGI_FORMAT_BC7_UNORM;
		dds_header_dxt10.resourceDimension = DirectX::DDS_DIMENSION_TEXTURE2D;
		dds_header_dxt10.miscFlag = 0;
		dds_header_dxt10.arraySize = 1;
		dds_header_dxt10.miscFlags2 = 0;
		break;
	default:
		GLACIER_UNREACHABLE;
	}
}

//template<typename T>
//void TextureResource<T>::regularizeHeader(TextureHeader& header) {
//	size_t w = header.width;
//	size_t h = header.height;
//	auto fmt = TEXDFormatToDXGIFormat(header.format);
//	if (!fmt)
//		int bre = 9;
//	size_t mips_levels;
//	getImageParameters(fmt, pixels.size(), w, h, mips_levels);
//	header = generateTexHeader(header.type, w, h, header.format);
//}

template<typename T>
void TextureResource<T>::regularizeHeader(TextureHeader& header) {
	auto scale = textScaleFactor(header.width, header.height);
	header.width /= scale;
	header.height /= scale;
	header.mips_cnt = maxMipsCount(header.width, header.height);

}

template<typename T>
HRESULT TextureResource<T>::getScratchImage(DirectX::TexMetadata* meta, DirectX::ScratchImage& image) const {
	DirectX::Blob blob;
	saveToDDSBuffer(blob);

	return DirectX::LoadFromDDSMemory(blob.GetBufferPointer(), blob.GetBufferSize(), 0, meta, image);
}

template<typename T>
std::string TextureResource<T>::name() const {
	return GlacierResource::name();
}

template<typename T>
void TextureResource<T>::saveToDDSBuffer(DirectX::Blob& blob) const {
	DirectX::DDS_HEADER dds_header{};
	DirectX::DDS_HEADER_DXT10 dds_header_dxt10{};

	initilizeDdsHeader(header, dds_header, dds_header_dxt10);

	size_t dds_file_buffer_size = 0;
	dds_file_buffer_size += sizeof(DirectX::DDS_MAGIC);
	dds_file_buffer_size += sizeof(DirectX::DDS_HEADER);
	if (header.format == TextureFormat::BC7)
		dds_file_buffer_size += sizeof(DirectX::DDS_HEADER_DXT10);
	dds_file_buffer_size += pixels.size();

	blob.Initialize(dds_file_buffer_size);
	char* dds_file_buffer = reinterpret_cast<char*>(blob.GetBufferPointer());

	int buf_off = 0;//TODO: Use BW
	memcpy_s(&dds_file_buffer[buf_off], sizeof(DirectX::DDS_MAGIC), &DirectX::DDS_MAGIC, sizeof(DirectX::DDS_MAGIC));
	buf_off += sizeof(DirectX::DDS_MAGIC);

	memcpy_s(&dds_file_buffer[buf_off], sizeof(dds_header), &dds_header, sizeof(dds_header));
	buf_off += sizeof(dds_header);

	if (header.format == TextureFormat::BC7) {
		memcpy_s(&dds_file_buffer[buf_off], sizeof(dds_header_dxt10), &dds_header_dxt10, sizeof(dds_header_dxt10));
		buf_off += sizeof(dds_header_dxt10);
	}

	memcpy_s(&dds_file_buffer[buf_off], pixels.size(), pixels.data(), pixels.size());
}

template<typename T>
bool TextureResource<T>::saveToDDSFile(const std::filesystem::path& path) const {
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

[[nodiscard]] DirectX::ScratchImage compress(const DirectX::ScratchImage& in, DXGI_FORMAT format) {
	DirectX::ScratchImage out;
	HRESULT hr = DirectX::Compress(EncodingDevice(), in.GetImages(), in.GetImageCount(), in.GetMetadata(), format, DirectX::TEX_COMPRESS_PARALLEL, DirectX::TEX_THRESHOLD_DEFAULT, out);
	if (FAILED(hr))
		hr = DirectX::Compress(in.GetImages(), in.GetImageCount(), in.GetMetadata(), format, DirectX::TEX_COMPRESS_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT, out);
	if (FAILED(hr))
		throw std::runtime_error("DDS compression failed");
	return out;
}

[[nodiscard]] DirectX::ScratchImage compress(const DirectX::ScratchImage& in, TextureFormat format) {
	return compress(in, toDxgiFormat(format));
}

template<typename T>
bool TextureResource<T>::saveToTGAFile(const std::filesystem::path& dir) const {
	HRESULT hr;

	DirectX::TexMetadata meta;
	DirectX::ScratchImage orig_image;
	DirectX::ScratchImage conv_image;

	DirectX::ScratchImage* export_image = &orig_image;

	hr = getScratchImage(&meta, orig_image);
	if (FAILED(hr))
		return false;

	switch (header.format) {
	case TextureFormat::A8:
		break;
	case TextureFormat::R8G8:
		hr = DirectX::Convert(*orig_image.GetImage(0, 0, 0), DXGI_FORMAT_R8G8B8A8_UNORM, 0, 0.f, conv_image);
		if (FAILED(hr))
			return false;

		//Fix blue channel
		for (int i = 2; i < conv_image.GetPixelsSize(); i += 4)
			conv_image.GetPixels()[i] = 0xFF;
		break;
	case TextureFormat::DXT1://Color maps, 1-bit alpha (mask)
	case TextureFormat::DXT5://Packed color, full alpha
	case TextureFormat::BC7://High res color + full alpha
		hr = DirectX::Decompress(*orig_image.GetImage(0, 0, 0), DXGI_FORMAT_R8G8B8A8_UNORM, conv_image);
		if (FAILED(hr))
			return false;
		break;
	case TextureFormat::BC4://8-bit grayscale
		hr = DirectX::Decompress(*orig_image.GetImage(0, 0, 0), DXGI_FORMAT_A8_UNORM, conv_image);
		if (FAILED(hr))
			return false;
		break;
	case TextureFormat::BC5://2-channel normal maps
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
	case TextureFormat::A8:
		export_image = &orig_image;
		break;
	case TextureFormat::R8G8:
	case TextureFormat::DXT1://Color maps, 1-bit alpha (mask)
	case TextureFormat::DXT5://Packed color, full alpha
	case TextureFormat::BC7://High res color + full alpha
	case TextureFormat::BC4://8-bit grayscale
	case TextureFormat::BC5://2-channel normal maps
		export_image = &conv_image;
		break;
	default:
		throw;
	}

	auto full_path = dir / (name() + ".tga");
	std::wstring wpath = full_path.generic_wstring();
	hr = DirectX::SaveToTGAFile(*export_image->GetImage(0, 0, 0), wpath.c_str(), &meta);
	if (FAILED(hr))
		return false;

	return true;
}

template<typename T>
bool TextureResource<T>::saveToPNGFile(const std::filesystem::path& path) const {
	HRESULT hr;	//TODO: factor common code between PNG and TGA export

	DirectX::TexMetadata meta;
	DirectX::ScratchImage orig_image;
	DirectX::ScratchImage conv_image;

	DirectX::ScratchImage* export_image = &orig_image;

	hr = getScratchImage(&meta, orig_image);
	if (FAILED(hr))
		return false;

	switch (header.format) {
	case TextureFormat::A8:
		break;
	case TextureFormat::R8G8:
		hr = DirectX::Convert(*orig_image.GetImage(0, 0, 0), DXGI_FORMAT_R8G8B8A8_UNORM, 0, 0.f, conv_image);
		if (FAILED(hr))
			return false;

		//Fix blue channel
		for (int i = 2; i < conv_image.GetPixelsSize(); i += 4)
			conv_image.GetPixels()[i] = 0xFF;
		break;
	case TextureFormat::DXT1://Color maps, 1-bit alpha (mask)
	case TextureFormat::DXT5://Packed color, full alpha
	case TextureFormat::BC7://High res color + full alpha
		hr = DirectX::Decompress(*orig_image.GetImage(0, 0, 0), DXGI_FORMAT_R8G8B8A8_UNORM, conv_image);
		if (FAILED(hr))
			return false;
		break;
	case TextureFormat::BC4://8-bit grayscale
		hr = DirectX::Decompress(*orig_image.GetImage(0, 0, 0), DXGI_FORMAT_A8_UNORM, conv_image);
		if (FAILED(hr))
			return false;
		break;
	case TextureFormat::BC5://2-channel normal maps
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
	case TextureFormat::A8:
		export_image = &orig_image;
		break;
	case TextureFormat::R8G8:
	case TextureFormat::DXT1://Color maps, 1-bit alpha (mask)
	case TextureFormat::DXT5://Packed color, full alpha
	case TextureFormat::BC7://High res color + full alpha
	case TextureFormat::BC4://8-bit grayscale
	case TextureFormat::BC5://2-channel normal maps
		export_image = &conv_image;
		break;
	default:
		throw;
	}

	auto full_path = path / name() / ".png";
	std::wstring wpath = full_path.generic_wstring();
	REFGUID container_format = DirectX::GetWICCodec(DirectX::WIC_CODEC_PNG);
	hr = DirectX::SaveToWICFile(*export_image->GetImage(0, 0, 0), 0, container_format, wpath.c_str());
	if (FAILED(hr))
		return false;

	return true;
}

template<typename T>
void TextureResource<T>::serialize(BinaryWriter& bw) {
	bw.write(header);
	bw.write(pixels.data(), pixels.size());
}


template<typename T>
std::unique_ptr<T> TextureResource<T>::loadFromTGAFile(const std::filesystem::path& path) {
	HRESULT hr;

	DirectX::TexMetadata meta;
	DirectX::ScratchImage input_image;
	DirectX::ScratchImage mip_chain;

	std::wstring wstr = path.generic_wstring();
	hr = DirectX::LoadFromTGAFile(wstr.c_str(), &meta, input_image);
	if (FAILED(hr))
		return nullptr;

	auto texd = std::make_unique<T>();

	auto stem = std::filesystem::path(path).stem().generic_string();
	RuntimeId id = stem;
	//try {id = std::stoull(stem, nullptr, 16);} catch (std::exception e) { return nullptr; }
	if (!initilizeTEXHeaderById(id, texd->header))
		return nullptr;

	//TODO:regularize Header

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
	case TextureFormat::A8:
		out_image = std::move(mip_chain);
		break;
	case TextureFormat::R8G8:
	case TextureFormat::DXT1:
	case TextureFormat::DXT5:
	case TextureFormat::BC7:
	case TextureFormat::BC4:
		out_image = compress(mip_chain, texd->header.format);
		break;
	case TextureFormat::BC5:
		for (int i = 2; i < mip_chain.GetPixelsSize(); i += 4)
			mip_chain.GetPixels()[i] = 0xFF;
		out_image = compress(mip_chain, texd->header.format);
		break;
	}

	//assert(out_image.GetImageCount() <= (sizeof(TEXHeader::mips_data_sizes) / sizeof(TEXHeader::mips_data_sizes[0])));

	texd->pixels.resize(out_image.GetPixelsSize());
	memcpy_s(texd->pixels.data(), out_image.GetPixelsSize(), out_image.GetPixels(), out_image.GetPixelsSize());


	//update header;
	//texd->header.file_size = sizeof(TEXHeader) + texd->pixels.size();
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

template<typename T>
std::unique_ptr<T> TextureResource<T>::loadFromPNGFile(const std::filesystem::path& path) {
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

	auto texd = std::make_unique<T>();

	auto stem = std::filesystem::path(path).stem().generic_string();
	RuntimeId id = stem;
	//try { id = std::stoull(stem, nullptr, 16); }
	//catch (std::exception e) { return nullptr; }
	if (!initilizeTEXHeaderById(id, texd->header))
		return nullptr;

	hr = DirectX::GenerateMipMaps(*input_image.GetImage(0, 0, 0), 0, texd->header.mips_cnt, mip_chain);
	if (FAILED(hr))
		return nullptr;

	DirectX::ScratchImage out_image;
	switch (texd->header.format) {
	case TextureFormat::A8:
		out_image = std::move(mip_chain);
		break;
	case TextureFormat::R8G8:
	case TextureFormat::DXT1:
	case TextureFormat::DXT5:
	case TextureFormat::BC7:
	case TextureFormat::BC4:
		out_image = compress(mip_chain, texd->header.format);
		break;
	case TextureFormat::BC5:
		for (int i = 2; i < mip_chain.GetPixelsSize(); i += 4)
			mip_chain.GetPixels()[i] = 0xFF;
		out_image = compress(mip_chain, texd->header.format);
		break;
	}

	texd->pixels.resize(out_image.GetPixelsSize());
	memcpy_s(texd->pixels.data(), out_image.GetPixelsSize(), out_image.GetPixels(), out_image.GetPixelsSize());
	return texd;
}

template TextureResource<TEXD>::TextureResource();
template TextureResource<TEXD>::TextureResource(const TextureResource& other);
template TextureResource<TEXD>::TextureResource(BinaryReader& br, RuntimeId id);
template HRESULT TextureResource<TEXD>::getScratchImage(DirectX::TexMetadata* meta, DirectX::ScratchImage& image) const;
template std::string TextureResource<TEXD>::name() const;
template void TextureResource<TEXD>::saveToDDSBuffer(DirectX::Blob& blob) const;
template bool TextureResource<TEXD>::saveToDDSFile(const std::filesystem::path& path) const;
template bool TextureResource<TEXD>::saveToTGAFile(const std::filesystem::path& dir) const;
template bool TextureResource<TEXD>::saveToPNGFile(const std::filesystem::path& path) const;
template void TextureResource<TEXD>::serialize(BinaryWriter& bw);
template std::unique_ptr<TEXD> TextureResource<TEXD>::loadFromTGAFile(const std::filesystem::path& path);
template std::unique_ptr<TEXD> TextureResource<TEXD>::loadFromPNGFile(const std::filesystem::path& path);

template TextureResource<TEXT>::TextureResource();
template TextureResource<TEXT>::TextureResource(const TextureResource& other);
template TextureResource<TEXT>::TextureResource(BinaryReader& br, RuntimeId id);
template HRESULT TextureResource<TEXT>::getScratchImage(DirectX::TexMetadata* meta, DirectX::ScratchImage& image) const;
template std::string TextureResource<TEXT>::name() const;
template void TextureResource<TEXT>::saveToDDSBuffer(DirectX::Blob& blob) const;
template bool TextureResource<TEXT>::saveToDDSFile(const std::filesystem::path& path) const;
template bool TextureResource<TEXT>::saveToTGAFile(const std::filesystem::path& dir) const;
template bool TextureResource<TEXT>::saveToPNGFile(const std::filesystem::path& path) const;
template void TextureResource<TEXT>::serialize(BinaryWriter& bw);
template std::unique_ptr<TEXT> TextureResource<TEXT>::loadFromTGAFile(const std::filesystem::path& path);
template std::unique_ptr<TEXT> TextureResource<TEXT>::loadFromPNGFile(const std::filesystem::path& path);


DirectX::ScratchImage decompress(const DirectX::ScratchImage& img) {
	DirectX::ScratchImage out;
	HRESULT hr = DirectX::Decompress(*img.GetImage(0, 0, 0), DXGI_FORMAT_R8G8B8A8_UNORM, out);
	if (FAILED(hr))
		std::runtime_error("Failed to decompress TEXD scratch image");
	return out;
}

DirectX::ScratchImage resize(const DirectX::ScratchImage& img, int w, int h) {
	DirectX::ScratchImage out;
	HRESULT hr = DirectX::Resize(*img.GetImage(0, 0, 0), w, h, DirectX::TEX_FILTER_DEFAULT, out);
	if (FAILED(hr))
		std::runtime_error("Scratch Image resizing failed");
	return out;
}

DirectX::ScratchImage buildMips(const DirectX::ScratchImage& img) {
	DirectX::ScratchImage out;
	auto mips_levels = maxMipsCount(img.GetMetadata().width, img.GetMetadata().height);
	HRESULT hr = DirectX::GenerateMipMaps(*img.GetImage(0, 0, 0), DirectX::TEX_FILTER_DEFAULT, mips_levels, out);
	if (FAILED(hr))
		std::runtime_error("Failed to generate mips");
	return out;
}


//Generates a low res TEXT texture resource from a TEXD texture resource
void GlacierFormats::initializeTextFromTexd(const TEXD& texd, TEXT& text) {
	if (texd.texture_atlas_data.size())
		throw std::runtime_error("TEXT generation is not supported for TEXDs with texture atlas information");

	text.header = texd.header;

	auto scale = textScaleFactor(texd.header.width, texd.header.height);
	auto text_w = texd.header.width / scale;
	auto text_h = texd.header.height / scale;
	
	DirectX::ScratchImage texd_image;
	DirectX::TexMetadata meta;
	HRESULT hr = texd.getScratchImage(&meta, texd_image);
	if (FAILED(hr))
		std::runtime_error("Failed to convert TEXD to scratch image");

	auto source_format = meta.format;
	if (DirectX::IsCompressed(source_format))
		texd_image = decompress(texd_image);
	texd_image = resize(texd_image, text_w, text_h);
	texd_image = buildMips(texd_image);
	if (DirectX::IsCompressed(source_format))
		texd_image = compress(texd_image, source_format);

	text.pixels.resize(texd_image.GetPixelsSize());
	std::copy(texd_image.GetPixels(), texd_image.GetPixels() + texd_image.GetPixelsSize(), text.pixels.data());
}

