#pragma once
#include "GlacierResource.h"
#include "TypeIDString.h"
#include "MaterialNodeTree.h"

#include <variant>
#include <unordered_map>
#include <optional>

namespace GlacierFormats {

    class BinaryReader;
    class Property;

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
        std::unique_ptr<MaterialPropertyNodeTree> properties;

        MATI(BinaryReader& br, RuntimeId id);
        void serialize(BinaryWriter& bw);
        
        const std::string& materialType() const;
        std::string instanceName() const;
        std::string instanceTags() const;
    };
}