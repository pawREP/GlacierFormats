#pragma once
#include <memory>
#include <cinttypes>

namespace GlacierFormats {

	class BinaryReader;
	class BinaryWriter;

	class LinkTable
	{
		uint32_t data_size;
		std::unique_ptr<char[]> data;

	public:
		LinkTable(BinaryReader* br);
		void serialize(BinaryWriter* bw) const;
	};

}