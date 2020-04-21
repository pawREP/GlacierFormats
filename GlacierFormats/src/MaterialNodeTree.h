#pragma once
#include "TypeIDString.h"

#include <variant>
#include <string>

namespace GlacierFormats {

    class BinaryReader;
    class BinaryWriter;
    class SerializationContext;
    
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

    class Property {
    public:
        using PropertyVariantType = std::variant<std::monostate, std::string, int, float, std::vector<int>, std::vector<float>>;

    private:
        TypeIDString name_;
        PropertyVariantType variant;

    public:

        Property();
        Property(BinaryReader& br, const SProperty& sprop);

        TypeIDString name() const;

        void serializeData(BinaryWriter& bw, SerializationContext& ctx) const;
        void serializeMeta(BinaryWriter& bw, SerializationContext& ctx) const;

        void setData(const PropertyVariantType& data);
        const PropertyVariantType& data() const;

        template<typename T>
        bool is() const {
            return std::holds_alternative<T>(variant);
        }

        template<typename T>
        T& get() {
            return std::get<T>(variant);
        }

        template<typename T>
        const T& get() const {
            return std::get<T>(variant);
        }
    };

    class PropertyNode;

    class PropertyBinder {
        TypeIDString name_;

        std::vector<PropertyNode> children;
        std::unordered_map<TypeIDString, PropertyNode*> property_binder_map;

    public:
        PropertyBinder(BinaryReader& br, const SProperty& sprop);
        uint32_t serialize(BinaryWriter& bw, SerializationContext& ctx) const;

        TypeIDString name() const;

        template<typename NodeType>
        std::vector<const PropertyNode*> gatherNodes() const;

        std::vector<PropertyNode>::const_iterator begin() const;
        std::vector<PropertyNode>::iterator begin();
        std::vector<PropertyNode>::const_iterator end() const;
        std::vector<PropertyNode>::iterator end();

        PropertyNode& operator[](size_t idx);
        const PropertyNode& at(size_t idx) const;

        size_t size() const;
    };

    class PropertyNode {
    private:
        using PropertyNodeVariant = std::variant<std::monostate, PropertyBinder, Property>;
        PropertyNodeVariant variant;

    public:
        PropertyNode();
        PropertyNode(BinaryReader& br);

        uint32_t serialize(BinaryWriter& bw, SerializationContext& ctx) const;

        TypeIDString name() const;

        template<typename T>
        bool is() const {
            return std::holds_alternative<T>(variant);
        }

        template<typename T>
        T& get() {
            return std::get<T>(variant);
        }

        template<typename T>
        const T& get() const {
            return std::get<T>(variant);
        }
    };

    class MaterialPropertyNodeTree {
    private:
        PropertyNode root_;

    public:
        MaterialPropertyNodeTree(BinaryReader& br);

        PropertyNode& root();

        const Property& nameProperty();
        const Property& tagsProperty();
        const PropertyBinder& bindPropertyBinder();

        uint32_t serialize(BinaryWriter& bw);
    };

}