#include "MATI.h"
#include "BinaryReader.hpp"
#include <assert.h>
#include "Exceptions.h"

namespace GlacierFormats {

	struct InstProp {
		SProperty nameProp;
		SProperty tagsProp;
		SProperty bindProp;
	};

	MATI::MATI(BinaryReader& br, RuntimeId id) : GlacierResource<MATI>(id) {
		uint32_t footer_offset = br.read<uint32_t>();
		br.align();

		br.seek(footer_offset);
		Header header = br.read<Header>();

		br.seek(header.instance_prop_offset);
		auto instance_prop = br.read<SProperty>();

		GLACIER_ASSERT_TRUE(instance_prop.name == "INST");
		GLACIER_ASSERT_TRUE(instance_prop.size == 3);

		br.seek(instance_prop.data);
		InstProp instance = br.read<InstProp>();

		GLACIER_ASSERT_TRUE(instance.nameProp.name == "NAME");
		GLACIER_ASSERT_TRUE(instance.tagsProp.name == "TAGS");
		GLACIER_ASSERT_TRUE(instance.bindProp.name == "BIND");

		//parse material name 
		br.seek(instance.nameProp.data);
		name = br.readCString();

		//parse tags
		br.seek(instance.tagsProp.data);
		tags = br.readCString();


		//parse BIND
		std::vector<SProperty> binder_headers(instance.bindProp.size);
		br.seek(instance.bindProp.data);
		br.read(binder_headers.data(), binder_headers.size());

		property_binders.reserve(instance.bindProp.size);
		for (auto& binder_header : binder_headers) {
			GLACIER_ASSERT_TRUE(binder_header.type == PROPERTY_TYPE::PT_SPROPERTY);

			PropertyBinder binder;
			binder.name = binder_header.name;
			binder.properties.reserve(binder_header.size);

			br.seek(binder_header.data);
			for (int i = 0; i < instance.bindProp.size; ++i) {
				auto property = br.read<SProperty>();
			}

			//TODO: FIx MATI property binder parsing

			//std::vector<SProperty> binder_properties(instance.bindProp.size);
			//br.seek(binder_header.data);
			//br.read(binder_properties.data(), binder_properties.size());
			//for (auto& property : binder_properties) {
			//	GLACIER_ASSERT_TRUE(property.type != PROPERTY_TYPE::PT_SPROPERTY);

			//	//TODO: Fix property parsing. It's broken since the reader/buffer release changes.
			//	//binder.properties.emplace(property.name, Property(property, data));
			//}


			//property_binders.push_back(binder);
		}

		//raw_data = br->release();
	}

	void MATI::serialize(BinaryWriter& bw) {
		std::runtime_error("Not implemented");
	}

	void MATI::print() const {
		/*printf("MATI: %s\n", name.c_str());
		for (const auto& binder : property_binders) {
			printf("\t%s\n", binder.name.c_str());
			for (const auto& prop : binder.properties) {
				if (prop.isType<int>()) {
					auto i_prop = prop.get<int>();
					printf("\t\t%s : ", prop.name().c_str());
					for (int i = 0; i < prop.size() - 1; ++i)
						printf("%d, ", i_prop[i]);
					printf("%d\n", i_prop[prop.size() - 1]);
				}
				else if (prop.isType<float>()) {
					auto f_prop = prop.get<float>();
					printf("\t\t%s : ", prop.name().c_str());
					for (int i = 0; i < prop.size() - 1; ++i)
						printf("%f, ", f_prop[i]);
					printf("%f\n", f_prop[prop.size() - 1]);
				}
				else if (prop.isType<char>()) {
					auto c_prop = prop.get<char>();
					printf("\t\t%s : \"%s\"\n", prop.name().c_str(), c_prop);
				}
			}
		}*/
	}

	Property::Property(SProperty* prop_, char* data_src) : prop(prop_) {
		switch (prop->type) {
		case PROPERTY_TYPE::PT_FLOAT:
			if (prop->size == 1)
				data_variant = reinterpret_cast<float*>(&prop->data);
			else
				data_variant = reinterpret_cast<float*>(&data_src[prop->data]);
			break;
		case PROPERTY_TYPE::PT_INT:
			if (prop->size == 1)
				data_variant = reinterpret_cast<int*>(&prop->data);
			else
				data_variant = reinterpret_cast<int*>(&data_src[prop->data]);
			break;
		case PROPERTY_TYPE::PT_CSTRING:
			data_variant = reinterpret_cast<char*>(&data_src[prop->data]);
			break;
		case PROPERTY_TYPE::PT_SPROPERTY:
			throw;
		default:
			throw;
		}
	}

	std::string Property::name() const {
		std::string name(prop->name, 4);
		std::reverse(name.begin(), name.end());
		return name;
	}

	uint32_t Property::size() const {
		return prop->size;
	}

	PROPERTY_TYPE Property::type() const {
		return prop->type;
	}

	//std::vector<Property>::const_iterator MATI::PropertyBinder::getPropertyByType(const char* type) const {
	//	auto it = std::find_if(properties.begin(), properties.end(), [&type](const Property& prop) -> bool {return prop.name == type; });
	//	assert(it == properties.end());
	//	return it;
	//}

	bool MATI::PropertyBinder::contains(const char* name) const {
		auto it = properties.find(name);
		if (it == properties.end())
			return false;
		return true;
	}

}