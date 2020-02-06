#pragma once
#include <string>
#include <vector>
#include <array>

namespace GlacierFormats {

	class IRig
	{
	public:
		struct Bone {
			int id;
			int parent_id;
			std::string name;
			float position[3];
			//float rotation[4];//quaternion
			float bind_pose[16];
			std::array<float, 16> local_transform;

			Bone(int id, int parent_id, const ::std::string& name, const float* position_) :
				id(id), parent_id(parent_id), name(name) {
				memcpy_s(position, sizeof(position), position_, sizeof(position));

			}
		};

		static void convert(IRig* dst, const IRig* src);

		[[nodiscard]] virtual int getBoneIdByName(const char* name) const = 0;
		[[nodiscard]] virtual std::vector<IRig::Bone> getCanonicalBoneList() const = 0;
		[[nodiscard]] virtual std::vector<float> getInverseGlobalBindMatrices() const = 0;
		virtual void setFromCanonicalBoneList(const ::std::vector<IRig::Bone>& bones) = 0;

	};
}