#pragma once
#include "GlacierResource.h"
#include "TypeIDString.h"
#include <variant>
#include <unordered_map>

namespace GlacierFormats {

    class BinaryReader;

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
        using PropertyVariantType = std::variant<std::string, int, float, std::vector<int>, std::vector<float >>;
    
    private:
        PropertyVariantType data_variant;

    public:

        Property(BinaryReader& br);

        std::string name;
        
        template<typename T>
        bool isType() const {
            return std::holds_alternative<T>(data_variant);
        }

        template<typename T>
        T& get() {
            return std::get<T>(data_variant);
        }

        template<typename T>
        const T& get() const {
            return std::get<T>(data_variant);
        }

    };

    class MATI : public GlacierResource<MATI> {
    private:
        struct Header {
            uint32_t name_offset;
            uint32_t unk0[6];
            uint32_t instance_prop_offset;
            uint32_t unk1[4]; //Some of this might be padding.
        };

        struct PropertyBinder {
            TypeIDString name;

            //TODO: flat_map would be a better fit here.
            std::unordered_map<TypeIDString, Property> properties;

            bool contains(const char* name) const;

        };

        //std::unique_ptr<char[]> raw_data;

    public:
        MATI(BinaryReader& br, RuntimeId id);

        void serialize(BinaryWriter& bw);

        std::string name;
        std::string tags;
        std::vector<PropertyBinder> property_binders;

        void print() const; //debug print 
    };


}