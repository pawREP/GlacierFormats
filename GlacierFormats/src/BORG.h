#pragma once
#include "GlacierResource.h"
#include "Matrix4.h"
#include "Quaternion.h"
#include "Vector.h"
#include "IRig.h"
#include <unordered_map>

namespace GlacierFormats {

	class BinaryReader;

	//TODO: Finish BORG implementation
	class BORG : public GlacierResource<BORG>, public IRig
	{

	private:
#pragma pack(push,1)
		struct Header {
			uint32_t bone_count;
			uint32_t animated_bone_count_q;
			uint32_t bone_definitions;
			uint32_t bind_pose;
			uint32_t bind_pose_inv_global_mats;
			uint32_t bone_constraints_header;
			uint32_t pose_bone_header;
			uint32_t invert_global_bones;
			uint32_t bone_map;
		};

		struct SBoneDefinition {
			Vec<float, 3> position;
			int32_t parent_bone_id;
			Vec<float, 3> size;
			char name[34];
			short body_part;
		};

		struct SVQ {
			Quaternion localTransformation;
			Vec<float, 4> position;
		};

#pragma pack(pop)

	public:
		BORG(BinaryReader& br, RuntimeId id);

		Header header;
		std::vector<SBoneDefinition> bone_definitions;
		std::vector<SVQ> bind_pose;
		std::vector<Transform> bind_pose_inverse_global_mats;

		std::unordered_map<std::string, int> getNameToBoneIndexMap() const;

		void serialize(BinaryWriter& bw);

		//IRig functions
		[[nodiscard]] virtual int getBoneIdByName(const char* name) const override final;
		[[nodiscard]] std::vector<IRig::Bone> getCanonicalBoneList() const override final;
		[[nodiscard]] std::vector<float> getInverseGlobalBindMatrices() const override final;
		void setFromCanonicalBoneList(const std::vector<IRig::Bone>& bones) override final;
	};
}