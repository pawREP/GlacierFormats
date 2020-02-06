#pragma once

namespace GlacierFormats {

#pragma pack(push,1)
	struct SPrimHeader {
		enum EPrimType : short {
			PTOBJECTHEADER = 1,
			PTMESH = 2,
			PTSHAPE = 5,
		};

		char draw_destination;
		char pack_type;
		EPrimType type;
	};

	struct SPrims : public SPrimHeader {

	};

	struct SPrimObjectHeader : public SPrims {
		enum PROPERTY_FLAGS {
			HAS_BONES = 0x01,
			HAS_FRAMES = 0x02,
			IS_LINKED_OBJECT = 0x04,
			IS_WEIGHTED_OBJECT = 0x08,
			USE_BOUNDS = 0x0100,
			HAS_HIRES_POSITIONS = 0x0200,
		};

		PROPERTY_FLAGS property_flags;
		int bone_rig_resource_index;
		int num_objects;
		int object_table;
		float min[3];
		float max[3];
	};

	struct SPrimObject : public SPrimHeader {

		enum SUBTYPE : char {
			SUBTYPE_STANDARD = 0,
			SUBTYPE_LINKED = 1,
			SUBTYPE_WEIGHTED = 2,
			SUBTYPE_STANDARDUV2 = 3,
			SUBTYPE_STANDARDUV3 = 4,
			SUBTYPE_STANDARDUV4 = 5,
			SUBTYPE_SPEEDTREE = 6,
		};

		enum class PROPERTY_FLAGS : char {
			PROPERTY_XAXISLOCKED = 0x01,
			PROPERTY_YAXISLOCKED = 0x02,
			PROPERTY_ZAXISLOCKED = 0x04,
			PROPERTY_HIRES_POSITIONS = 0x08,
			PROPERTY_PS3_EDGE = 0x10,
			PROPERTY_COLOR1 = 0x20,
			PROPERTY_ISNOPHYSICSPROP = 0x40,
		};

		SUBTYPE sub_type; //PRIM_OBJECT_SUB_TYPE
		PROPERTY_FLAGS properties;
		unsigned char lod_mask;
		char variant_id;
		char bias;
		char offset;
		short material_id;
		int wire_color;
		int unk2;
		float min[3];
		float max[3];
	};

	struct SPrimMesh : public SPrimObject {
		int sub_mesh_table;
		float pos_scale[4];
		float pos_bias[4];
		float uv_scale[2];
		float uv_bias[2];
		int unk16;
		float unk17;
		int m_unk_tabl1_offset;
		int m_unk_tabl2_offset;
	};

	struct SPrimMeshWeighted : public SPrimMesh {
		unsigned int bone_info;
		unsigned int num_copy_bones;
		unsigned int copy_bones;
		unsigned int bone_indices;
	};

	struct SPrimSubMesh : SPrimObject {
		int num_vertex;
		int vertex_buffer;
		int num_indices;
		int unkown;
		int index_buffer;
		int collision;
		int cloth;
		int num_uv_channels;
		int sm_unk_tabl2_offset;
	};

#pragma pack(pop)

}