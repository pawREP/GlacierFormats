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
			for (int i = 0; i < binder_header.size; ++i) {
				auto property = Property(br);
				binder.properties.emplace(property.name, property);
			}

			property_binders.push_back(std::move(binder));
		}
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

	template<typename T>
	void readPropData(Property::PropertyVariantType& variant, const SProperty& prop, BinaryReader& br) {
		if (prop.size == 1)
			variant = *reinterpret_cast<const T*>(&prop.data);
		else {
			br.seek(prop.data);
			std::vector<T> vec;
			for (int i = 0; i < prop.size; ++i)
				vec.push_back(br.read<T>());
			variant = vec;
		}
	}

	Property::Property(BinaryReader& br) {
		auto prop = br.read<SProperty>();
		name = prop.name;

		auto final_read_pos = br.tell();

		switch (prop.type) {
		case PROPERTY_TYPE::PT_FLOAT:
			readPropData<float>(data_variant, prop, br);
			break;
		case PROPERTY_TYPE::PT_INT:
			readPropData<int>(data_variant, prop, br);
			break;
		case PROPERTY_TYPE::PT_CSTRING://Can string properties be stored inline in SProperty???
			br.seek(prop.data);
			data_variant = br.readCString();
			break;
		case PROPERTY_TYPE::PT_SPROPERTY:
			throw;
		default:
			throw;
		}
		br.seek(final_read_pos);
	}

	bool MATI::PropertyBinder::contains(const char* name) const {
		auto it = properties.find(name);
		if (it == properties.end())
			return false;
		return true;
	}

}