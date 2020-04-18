#pragma once
#include "GlacierResource.h"
#include "TypeIDString.h"
#include <variant>
#include <unordered_map>
#include <optional>

namespace GlacierFormats {

    class BinaryReader;
    class Property;
    class SProperty;

    class SerializationContext {
    private:
        std::unordered_map<size_t, uint32_t> records;
    public:
        template<typename T>
        uint32_t get(const T& data) const;

        //Try to insert data record into serialization context. Retruns true if record was inserted.
        template<typename T>
        bool insert_if(const T& data, uint32_t offset);
        

    };

    class Property { 
    public:
        using PropertyVariantType = std::variant<std::string, int, float, std::vector<int>, std::vector<float>>;

    private:
        TypeIDString name_;
        PropertyVariantType variant;

    public:

        Property();
        Property(BinaryReader& br, const SProperty& sprop);

        TypeIDString name() const;

        void serializeData(BinaryWriter& bw, SerializationContext& ctx) const;
        void serializeMeta(BinaryWriter& bw, SerializationContext& ctx) const;
        
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

        template<typename T>
        std::vector<const PropertyNode*> gatherNodes() const;

        std::vector<PropertyNode>::const_iterator begin() const;
        std::vector<PropertyNode>::iterator begin();
        std::vector<PropertyNode>::const_iterator end() const;
        std::vector<PropertyNode>::iterator end();
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

    class MATI : public GlacierResource<MATI> {
    private:
        struct Header {
            uint32_t type;
            uint32_t texture_count;
            uint32_t unk0[5];
            uint32_t instance_prop_offset;
            uint32_t unk1[4]; //Some of this might be padding.
        } header;

        std::string type_;

    public:
        PropertyNode root;

        MATI(BinaryReader& br, RuntimeId id);
        void serialize(BinaryWriter& bw);
        
        const std::string& type() const;
        std::string materialName() const;
    };
}