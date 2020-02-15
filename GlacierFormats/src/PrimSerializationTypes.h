#pragma once

namespace GlacierFormats {

#pragma pack(push,1)
	struct SPrimHeader {
		enum class EPrimType : short {
			PTOBJECTHEADER = 1,
			PTMESH = 2,
			PTSHAPE = 5,
		};

		char draw_destination;	//Unused? Always 0
		char pack_type;			//Unused? Always 0
		EPrimType type;			//Prim type.
	};

	struct SPrims : public SPrimHeader {

	};

	struct SPrimObjectHeader : public SPrims {
		enum class PROPERTY_FLAGS {
			HAS_BONES = 0x01,
			HAS_FRAMES = 0x02,
			IS_LINKED_OBJECT = 0x04,
			IS_WEIGHTED_OBJECT = 0x08,
			USE_BOUNDS = 0x0100,
			HAS_HIRES_POSITIONS = 0x0200,
		};

		PROPERTY_FLAGS property_flags;	//Properties
		int bone_rig_resource_index;	//Index of BORG resource. Either -1 or 0. There are no meshes with multple BORG resources. (TODO: Needs verification by full scan.)
		int num_objects;				//Number of mesh objects in the object table
		int object_table;				//Offset to table of mesh objects
		float min[3];					//Global bounding box min
		float max[3];					//Global bounding box max
	};

	struct SPrimObject : public SPrimHeader {

		enum class SUBTYPE : char {
			SUBTYPE_STANDARD = 0,		//Static mesh
			SUBTYPE_LINKED = 1,			//Rigged mesh
			SUBTYPE_WEIGHTED = 2,		//Rigged and skinned mesh
			SUBTYPE_STANDARDUV2 = 3,	//Unused
			SUBTYPE_STANDARDUV3 = 4,	//Unused
			SUBTYPE_STANDARDUV4 = 5,	//Unused
			SUBTYPE_SPEEDTREE = 6,		//Meshes generate by SpeedTree middleware
		};

		enum class PROPERTY_FLAGS : char {
			PROPERTY_XAXISLOCKED = 0x01,	
			PROPERTY_YAXISLOCKED = 0x02,
			PROPERTY_ZAXISLOCKED = 0x04,
			PROPERTY_HIRES_POSITIONS = 0x08,	//Set for meshes that use 32 bit precision position data, instead of the default 16 bit precision.
			PROPERTY_PS3_EDGE = 0x10,
			PROPERTY_COLOR1 = 0x20,				//Set for meshes that do NOT contain vertex color data.
			PROPERTY_ISNOPHYSICSPROP = 0x40,
		};

		SUBTYPE sub_type;				//Object subtype
		PROPERTY_FLAGS properties;		//Properties
		unsigned char lod_mask;			//Lod mask. One bit per distance range. Lower bits indicate closer ranges.
		char variant_id;				//Variant id. Used for models that have multiple variants that share the same mesh, like miami race cars in Hitman 2.
		char bias;
		char offset;
		short material_id;				//Index of the MATI reference used for this mesh
		int wire_color;					//Color used by wire-frame debug render.
		int unk2;						//Unknown. Assert(0);
		float min[3];					//Bounding box min
		float max[3];					//Bounding box max
	};

	struct SPrimMesh : public SPrimObject {

		enum class CLOTH_FLAGS {
			//...
			USE_SMOLL_CLOTH_BLOCK = 0x80 //When set, indicates that the submesh uses the small cloth block variant instead
										 //of the large, 0x14 bytes per vertex one. Check PrimClothData.cpp for details.
			//...
		};

		int sub_mesh_table;			//Offset, yable of submeshes.
		float pos_scale[4];			//Scale used to decompress vertex position data
		float pos_bias[4];			//Bias used to decompress vertex position data
		float uv_scale[2];			//Scale used to decompress uv coordinates
		float uv_bias[2];			//Bias used to decompress uv coordinates
		CLOTH_FLAGS cloth_flags;	//Bitfield used for cloth properties. 
		int unk17;					
		int m_unk_tabl1_offset;
		int m_unk_tabl2_offset;
	};

	struct SPrimMeshWeighted : public SPrimMesh {
		unsigned int bone_indices;
		unsigned int num_copy_bones;
		unsigned int copy_bones;
		unsigned int bone_indices_;
	};

	struct SPrimSubMesh : SPrimObject {
		int num_vertex;				//Number of vertices in the submesh
		int vertex_buffer;			//Offest to vertex buffer
		int num_indices;			//Number of indices in index buffer
		int num_indices_ex;			//Number of additional indices in index buffer extension. This is rarely used in meshes with very large index buffers. ~10/30000
		int index_buffer;			//Offset of index buffer
		int collision;				//Offset to collision data
		int cloth;					//Offset to cloth data
		int num_uv_channels;		//Number of UV channels. Very few meshes use more than one. ~20/30000
		int sm_unk_tabl2_offset;	
	};

#pragma pack(pop)

}