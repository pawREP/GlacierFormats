#include "MATI.h"
#include "BinaryReader.hpp"
#include <assert.h>
#include "Exceptions.h"
#include "Hash.h"
#include "bit_cast.h"

namespace GlacierFormats {

	enum class PROPERTY_TYPE {
		PT_FLOAT = 0,
		PT_CSTRING = 1,
		PT_INT = 2,
		PT_SPROPERTY = 3,
	};

	struct SProperty {
		TypeIDString name;
		uint32_t data;
		uint32_t size;
		PROPERTY_TYPE type;
	};

	MATI::MATI(BinaryReader& br, RuntimeId id) : GlacierResource<MATI>(id) {
		uint32_t footer_offset = br.read<uint32_t>();
		br.align();

		br.seek(footer_offset);
		header = br.read<Header>();

		br.seek(header.type);
		type_ = br.readCString();

		br.seek(header.instance_prop_offset);
		root = PropertyNode(br);
	}

	void MATI::serialize(BinaryWriter& bw) { 
		bw.write(0);//footer offset placeholder
		bw.align();

		//Traverse and serialize the property graph depth first
		SerializationContext ctx;
		auto root_binder_off = root.serialize(bw, ctx);

		//Seems a bit weird that the type string precedes the instance property but what can you do...
		bw.writeCString(type_.c_str());
		bw.align();
		SProperty inst{ root.name(), root_binder_off, 3, PROPERTY_TYPE::PT_SPROPERTY };
		bw.write(inst);

		GLACIER_ASSERT_TRUE(header.instance_prop_offset == (bw.tell() - sizeof(SProperty)));
		GLACIER_ASSERT_TRUE(header.type == (bw.tell() - 2 * sizeof(SProperty)));
		int header_offset = bw.tell();
		bw.write(header);

		bw.seek(0);
		bw.write(header_offset);
	}

	const std::string& MATI::type() const {
		return type_;
	}

	std::string MATI::materialName() const {
		const auto& mat_name_prop = root.get<PropertyBinder>().begin()->get<Property>();
		GLACIER_ASSERT_TRUE(mat_name_prop.name() == "NAME");
		return mat_name_prop.get<std::string>();
	}

	template<typename T>
	Property::PropertyVariantType readPropData(const SProperty& prop, BinaryReader& br) {
		Property::PropertyVariantType variant;

		if (prop.size == 1) {
			variant = *reinterpret_cast<const T*>(&prop.data);
		} else {
			br.seek(prop.data);
			std::vector<T> vec;
			for (int i = 0; i < prop.size; ++i)
				vec.push_back(br.read<T>());
			variant = vec;
		}

		return variant;
	}

	Property::Property() {};

	Property::Property(BinaryReader& br, const SProperty& sprop) {
		name_ = sprop.name;

		auto br_home = br.tell();

		switch (sprop.type) {
		case PROPERTY_TYPE::PT_FLOAT:
			variant = readPropData<float>(sprop, br);
			break;
		case PROPERTY_TYPE::PT_INT:
			variant = readPropData<int>(sprop, br);
			break;
		case PROPERTY_TYPE::PT_CSTRING://TODO: Check if cstring properties can be stored inline in SProperty, like int, float
			br.seek(sprop.data);
			variant = br.readCString();
			break;
		default:
			throw;
		}
		br.seek(br_home);
	}

	TypeIDString Property::name() const	{
		return name_;
	}

	void Property::serializeData(BinaryWriter& bw, SerializationContext& ctx) const	{
		std::visit([&bw, &ctx](const auto& var) {
			using T = std::decay_t<decltype(var)>;
			if constexpr (
				std::is_same_v<T, int> ||
				std::is_same_v<T, float>) {
				//Data is inlined in SProperty
			}
			else if constexpr (
				std::is_same_v<T, std::vector<int>> ||
				std::is_same_v<T, std::vector<float>>) {

				if (ctx.insert_if(var, bw.tell())) {
					for (const auto& v : var)
						bw.write(v);
					bw.align();
				}
			}
			else if constexpr (std::is_same_v<T, std::string>) {
				if (ctx.insert_if(var, bw.tell())) {
					bw.writeCString(var);//TODO: Check if strings in MATI are really terminated
					bw.align();
				}
			}
			else {
				GLACIER_UNREACHABLE;
			}
		}, variant);
	}

	void Property::serializeMeta(BinaryWriter& bw, SerializationContext& ctx) const {
		std::visit([&bw, &ctx, this](const auto& var) {
			using T = std::decay_t<decltype(var)>;
			if constexpr (std::is_same_v<T, int>){
				SProperty sprop{ name(), var, 1, PROPERTY_TYPE::PT_INT };
				bw.write(sprop);
			}
			else if constexpr (std::is_same_v<T, float>) {
				uint32_t value = bit_cast<uint32_t, float>(var);
				SProperty sprop{ name(), value, 1, PROPERTY_TYPE::PT_FLOAT };
				bw.write(sprop);
			}
			else if constexpr (std::is_same_v<T, std::vector<int>>) {
				SProperty sprop{ name(), ctx.get(var), var.size(), PROPERTY_TYPE::PT_INT };
				bw.write(sprop);
			}
			else if constexpr (std::is_same_v<T, std::vector<float>>) {
				SProperty sprop{ name(), ctx.get(var), var.size(), PROPERTY_TYPE::PT_FLOAT };
				bw.write(sprop);
			}
			else if constexpr (std::is_same_v<T, std::string>) {
				SProperty sprop{ name(), ctx.get(var), var.size() + 1, PROPERTY_TYPE::PT_CSTRING };
				bw.write(sprop);
			}
			else {
				GLACIER_UNREACHABLE;
			}
		}, variant);
	}

	PropertyNode::PropertyNode() {
	}

	PropertyNode::PropertyNode(BinaryReader& br) {
		const auto sprop = br.read<SProperty>();

		const auto br_home = br.tell();

		switch (sprop.type) {
		case PROPERTY_TYPE::PT_FLOAT:
		case PROPERTY_TYPE::PT_INT:
		case PROPERTY_TYPE::PT_CSTRING://TODO: Check if cstring properties can be stored inline in SProperty, like int, float
			variant = Property(br, sprop);
			break;
		case PROPERTY_TYPE::PT_SPROPERTY:
			variant = PropertyBinder(br, sprop);
			break;
		default:
			throw;
		}
		br.seek(br_home);
	}

	uint32_t PropertyNode::serialize(BinaryWriter& bw, SerializationContext& ctx) const {
		uint32_t ret = 0;
		std::visit([&bw, &ctx,&ret](const auto& var) {
			using T = std::decay_t<decltype(var)>;
			if constexpr (std::is_same_v<T, PropertyBinder>){
				ret = var.serialize(bw, ctx);
			}
		}, variant);
		return ret;
	}

	TypeIDString PropertyNode::name() const {
		std::string name;
		std::visit([&name](auto& var){
			using T = std::decay_t<decltype(var)>;
			if constexpr (
				std::is_same_v<T, PropertyBinder> ||
				std::is_same_v<T, Property> ) {
				name = var.name();
			}
			else {
				name = "";
			}
		}, variant);

		return name;
	}

	PropertyBinder::PropertyBinder(BinaryReader& br, const SProperty& sprop) {
		auto br_home = br.tell();

		name_ = sprop.name;

		br.seek(sprop.data);
		for (int i = 0; i < sprop.size; ++i) {
			children.emplace_back(br);
			property_binder_map[children.back().name()] = &children.back();
		}

		br.seek(br_home);
	}

	uint32_t PropertyBinder::serialize(BinaryWriter& bw, SerializationContext& ctx) const {
		std::vector<uint32_t> offsets(children.size(), 0);

		for(const auto& child : gatherNodes<Property>())
			child->get<Property>().serializeData(bw, ctx);

		for (int i = 0; i < children.size(); ++i) {
			const auto& child = children[i];
			if (child.is<Property>())
				continue;
			offsets[i] = child.get<PropertyBinder>().serialize(bw, ctx);
		}

		int binder_start_offset = bw.tell();
		for (const auto& data_node : gatherNodes<Property>()) {
			data_node->get<Property>().serializeMeta(bw, ctx);
		}

		for (int i = 0; i < children.size(); ++i) {
			const auto& child = children[i];
			if (child.is<Property>())
				continue;
			const auto& binder_node = child.get<PropertyBinder>();
			SProperty sprop{ binder_node.name(), offsets[i], binder_node.children.size(), PROPERTY_TYPE::PT_SPROPERTY };
			bw.write(sprop);
		}

		return binder_start_offset;
	}

	TypeIDString PropertyBinder::name() const {
		return name_;
	}

	template<typename T>
	std::vector<const PropertyNode*> PropertyBinder::gatherNodes() const {
		std::vector<const PropertyNode*> data_nodes;
		for (const auto& child : children) {
			if (child.is<T>())
				data_nodes.push_back(&child);
		}
		return data_nodes;
	}

	template std::vector<const PropertyNode*> PropertyBinder::gatherNodes<Property>() const;
	template std::vector<const PropertyNode*> PropertyBinder::gatherNodes<PropertyBinder>() const;

	std::vector<PropertyNode>::const_iterator PropertyBinder::begin() const {
		return children.begin();
	}

	std::vector<PropertyNode>::iterator PropertyBinder::begin() {
		return children.begin();
	}

	std::vector<PropertyNode>::const_iterator PropertyBinder::end() const {
		return children.end();
	}

	std::vector<PropertyNode>::iterator PropertyBinder::end() {
		return children.end();
	}

	template<typename T>
	uint32_t SerializationContext::get(const T& data) const {
		const auto hash = hash::fnv1a(data);
		auto it = records.find(hash);
		if (it != records.end())
			return it->second;
		GLACIER_UNREACHABLE;
	}

	template uint32_t SerializationContext::get<int>(const int& data) const;
	template uint32_t SerializationContext::get<float>(const float& data) const;
	template uint32_t SerializationContext::get<std::vector<int>>(const std::vector<int>& data) const;
	template uint32_t SerializationContext::get<std::vector<float>>(const std::vector<float>& data) const;
	template uint32_t SerializationContext::get<std::string>(const std::string& data) const;

	template<typename T>
	bool SerializationContext::insert_if(const T& data, uint32_t offset) {
		const auto hash = hash::fnv1a(data);
		auto it = records.find(hash);
		if (it != records.end())
			return false;

		records[hash] = offset;
		return true;
	}

	template bool SerializationContext::insert_if<int>(const int& data, uint32_t offset);
	template bool SerializationContext::insert_if<float>(const float& data, uint32_t offset);
	template bool SerializationContext::insert_if<std::vector<int>>(const std::vector<int>& data, uint32_t offset);
	template bool SerializationContext::insert_if<std::vector<float>>(const std::vector<float>& data, uint32_t offset);
	template bool SerializationContext::insert_if<std::string>(const std::string& data, uint32_t offset);



}