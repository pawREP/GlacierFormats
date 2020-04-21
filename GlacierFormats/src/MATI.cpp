#include "MATI.h"
#include "BinaryReader.hpp"
#include <assert.h>
#include "Exceptions.h"
#include "Hash.h"
#include "bit_cast.h"

namespace GlacierFormats {

	MATI::MATI(BinaryReader& br, RuntimeId id) : GlacierResource<MATI>(id) {
		uint32_t footer_offset = br.read<uint32_t>();
		br.align();

		br.seek(footer_offset);
		header = br.read<Header>();

		br.seek(header.type);
		type_ = br.readCString();

		br.seek(header.instance_prop_offset);
		properties = std::make_unique<MaterialPropertyNodeTree>(br);
	}

	void MATI::serialize(BinaryWriter& bw) { 
		bw.write(0);//footer offset placeholder
		bw.align();

		auto root_binder_off = properties->serialize(bw);

		//Seems a bit weird that the type string precedes the instance property but what can you do...
		bw.writeCString(type_.c_str());
		bw.align();
		SProperty inst{ "INST", root_binder_off, 3, PROPERTY_TYPE::PT_SPROPERTY };
		bw.write(inst);

		int header_offset = bw.tell();
		bw.write(header);

		bw.seek(0);
		bw.write(header_offset);
	}

	const std::string& MATI::materialType() const {
		return type_;
	}

	std::string MATI::instanceName() const {
		return properties->nameProperty().get<std::string>();
	}

	std::string MATI::instanceTags() const {
		return properties->tagsProperty().get<std::string>();
	}





}