#include "MaterialNodeTree.h"
#include "BinaryReader.hpp"
#include "BinaryWriter.hpp"
#include "Hash.h"

using namespace GlacierFormats;

class GlacierFormats::SerializationContext {
private:
	std::unordered_map<size_t, uint32_t> records;
public:
	template<typename T>
	uint32_t get(const T& data) const;

	//Try to insert data record into serialization context. Retruns true if record was inserted.
	template<typename T>
	bool insert_if(const T& data, uint32_t offset);
};

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

template<typename T>
Property::PropertyVariantType readPropData(const SProperty& prop, BinaryReader& br) {
	Property::PropertyVariantType variant;

	if (prop.size == 1) {
		variant = *reinterpret_cast<const T*>(&prop.data);
	}
	else {
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

TypeIDString Property::name() const {
	return name_;
}

void Property::serializeData(BinaryWriter& bw, SerializationContext& ctx) const {
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
		if constexpr (std::is_same_v<T, int>) {
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

void Property::setData(const PropertyVariantType& data) {
	variant = data;
}

const Property::PropertyVariantType& Property::data() const {
	return variant;
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
	std::visit([&bw, &ctx, &ret](const auto& var) {
		using T = std::decay_t<decltype(var)>;
		if constexpr (std::is_same_v<T, PropertyBinder>) {
			ret = var.serialize(bw, ctx);
		}
	}, variant);
	return ret;
}

TypeIDString PropertyNode::name() const {
	std::string name;
	std::visit([&name](auto& var) {
		using T = std::decay_t<decltype(var)>;
		if constexpr (
			std::is_same_v<T, PropertyBinder> ||
			std::is_same_v<T, Property>) {
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

	for (const auto& child : gatherNodes<Property>())
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

PropertyNode& PropertyBinder::operator[](size_t idx) {
	return children[idx];
}

const PropertyNode& PropertyBinder::at(size_t idx) const {
	return children.at(idx);
}

size_t PropertyBinder::size() const {
	return children.size();
}

GlacierFormats::MaterialPropertyNodeTree::MaterialPropertyNodeTree(BinaryReader& br) {
	root_ = PropertyNode(br);
}

PropertyNode& GlacierFormats::MaterialPropertyNodeTree::root() {
	return root_;
}

const Property& GlacierFormats::MaterialPropertyNodeTree::nameProperty(){
	return root().get<PropertyBinder>().at(0).get<Property>();
}

const Property& GlacierFormats::MaterialPropertyNodeTree::tagsProperty(){
	return root().get<PropertyBinder>().at(1).get<Property>();
}

const PropertyBinder& GlacierFormats::MaterialPropertyNodeTree::bindPropertyBinder() {
	return root().get<PropertyBinder>().at(2).get<PropertyBinder>();

}

uint32_t GlacierFormats::MaterialPropertyNodeTree::serialize(BinaryWriter& bw) {
	SerializationContext ctx;
	return root_.serialize(bw, ctx);
}
