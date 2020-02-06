#include "BORG.h"
#include "BinaryReader.hpp"
#include <array>
#include "Exceptions.h"
//#include "../thirdparty/glm-master/glm/matrix.hpp"
//#include "../thirdparty/glm-master/glm/mat4x4.hpp"
//#include "../thirdparty/glm-master/glm/gtx/quaternion.hpp"

using namespace GlacierFormats;

	//TODO: move those math functions
	inline float SIGN(float x) {
		return (x >= 0.0f) ? +1.0f : -1.0f;
	}

	inline float NORM(float a, float b, float c, float d) {
		return sqrt(a * a + b * b + c * c + d * d);
	}

	// quaternion = [w, x, y, z]'
	std::array<float, 4> rotMatToQuaternion(const std::array<float, 9>& m) {
		float r11 = m[0];
		float r12 = m[1];
		float r13 = m[2];
		float r21 = m[3];
		float r22 = m[4];
		float r23 = m[5];
		float r31 = m[6];
		float r32 = m[7];
		float r33 = m[8];
		float q0 = (r11 + r22 + r33 + 1.0f) / 4.0f;
		float q1 = (r11 - r22 - r33 + 1.0f) / 4.0f;
		float q2 = (-r11 + r22 - r33 + 1.0f) / 4.0f;
		float q3 = (-r11 - r22 + r33 + 1.0f) / 4.0f;
		if (q0 < 0.0f) {
			q0 = 0.0f;
		}
		if (q1 < 0.0f) {
			q1 = 0.0f;
		}
		if (q2 < 0.0f) {
			q2 = 0.0f;
		}
		if (q3 < 0.0f) {
			q3 = 0.0f;
		}
		q0 = sqrt(q0);
		q1 = sqrt(q1);
		q2 = sqrt(q2);
		q3 = sqrt(q3);
		if (q0 >= q1 && q0 >= q2 && q0 >= q3) {
			q0 *= +1.0f;
			q1 *= SIGN(r32 - r23);
			q2 *= SIGN(r13 - r31);
			q3 *= SIGN(r21 - r12);
		}
		else if (q1 >= q0 && q1 >= q2 && q1 >= q3) {
			q0 *= SIGN(r32 - r23);
			q1 *= +1.0f;
			q2 *= SIGN(r21 + r12);
			q3 *= SIGN(r13 + r31);
		}
		else if (q2 >= q0 && q2 >= q1 && q2 >= q3) {
			q0 *= SIGN(r13 - r31);
			q1 *= SIGN(r21 + r12);
			q2 *= +1.0f;
			q3 *= SIGN(r32 + r23);
		}
		else if (q3 >= q0 && q3 >= q1 && q3 >= q2) {
			q0 *= SIGN(r21 - r12);
			q1 *= SIGN(r31 + r13);
			q2 *= SIGN(r32 + r23);
			q3 *= +1.0f;
		}
		else {
			printf("coding error\n");
		}
		float r = NORM(q0, q1, q2, q3);
		q0 /= r;
		q1 /= r;
		q2 /= r;
		q3 /= r;

		std::array<float, 4> quat;
		quat[0] = q0;
		quat[1] = q1;
		quat[2] = q2;
		quat[3] = q3;

		return quat;
	}



	//Original parser for this class: runtime.animation.dll -> ZAnimationBoneDataInstaller::ZAnimationBoneDataInstaller(void)
	BORG::BORG(BinaryReader& br, RuntimeId id) : GlacierResource<BORG>(id) {
		uint32_t header_off = br.read<uint32_t>();

		br.seek(header_off);
		header = br.read<BORG::Header>();
		br.align();

		br.seek(header.bone_definitions);
		bone_definitions.resize(header.bone_count);
		for (auto& bone : bone_definitions) {
			bone = br.read<SBoneDefinition>();
		}

		br.seek(header.bind_pose);
		bind_pose.resize(header.bone_count);
		for (auto& svq : bind_pose) {
			svq = br.read<SVQ>();
			//TODO: consider reordering the struct and making members private instead of swapping like below.
			//Fix coordinate system.
			//std::swap(svq.localTransformation.j, svq.localTransformation.k);
			//std::swap(svq.position.y(), svq.position.z());
		}


		br.seek(header.bind_pose_inv_global_mats);
		bind_pose_inverse_global_mats.resize(header.bone_count);
		for (auto& mat : bind_pose_inverse_global_mats) {
			mat = Transform::Identity();
			br.read(&mat.data[0], 3);
			br.read(&mat.data[4], 3);
			br.read(&mat.data[8], 3);
			br.read(&mat.data[12], 3);
		}
	}

	std::unordered_map<std::string, int> GlacierFormats::BORG::getNameToBoneIndexMap() const {
		std::unordered_map<std::string, int> map;
		for (int id = 0; id < bone_definitions.size(); ++id) {
			const auto& def = bone_definitions[id];
			map.insert({ std::string(def.name), id });
		}
		return map;
	}

	void GlacierFormats::BORG::serialize(BinaryWriter& bw) {
		throw UnsupportedFeatureException("BORG serialization not supported");
	}

	int BORG::getBoneIdByName(const char* name) const {
		for (int i = 0; i < bone_definitions.size(); ++i) {
			if (std::strcmp(bone_definitions[i].name, name) == 0)
				return i;
		}
		return -1;
	}

	std::vector<IRig::Bone> BORG::getCanonicalBoneList() const {
		//NEW CODE
		//std::vector<IRig::Bone> ret;
		//ret.reserve(bone_definitions.size());

		//std::vector<Transform> globalbindMats(header.bone_count);
		//for (int bone_id = 0; bone_id < header.bone_count; ++bone_id) {
		//	globalbindMats[bone_id] = bind_pose_inverse_global_mats[bone_id].inverse();
		//}

		//std::vector<Transform> localTransform(header.bone_count);
		//for (int bone_id = 0; bone_id < header.bone_count; ++bone_id) {
		//	auto parent_id = bone_definitions[bone_id].parent_bone_id;
		//	if (parent_id == -1)
		//		localTransform[bone_id] = globalbindMats[bone_id];
		//	else {
		//		auto inv = globalbindMats[parent_id].inverse();
		//		auto mul = inv.multiply(globalbindMats[bone_id]);
		//		localTransform[bone_id] = mul;
		//	}
		//}

		//std::vector<std::array<float, 3>> localPositions(header.bone_count);
		//for (int bone_id = 0; bone_id < header.bone_count; ++bone_id) {
		//	memcpy_s(localPositions[bone_id].data(), 3 * sizeof(float), &localTransform[bone_id][12], 3 * sizeof(float));
		//}

		//std::vector<std::array<float, 9>> localRotations(header.bone_count);
		//for (int bone_id = 0; bone_id < header.bone_count; ++bone_id) {
		//	localRotations[bone_id][0] = localTransform[bone_id][0];
		//	localRotations[bone_id][1] = localTransform[bone_id][1];
		//	localRotations[bone_id][2] = localTransform[bone_id][2];

		//	localRotations[bone_id][3] = localTransform[bone_id][4];
		//	localRotations[bone_id][4] = localTransform[bone_id][5];
		//	localRotations[bone_id][5] = localTransform[bone_id][6];

		//	localRotations[bone_id][6] = localTransform[bone_id][8];
		//	localRotations[bone_id][7] = localTransform[bone_id][9];
		//	localRotations[bone_id][8] = localTransform[bone_id][10];
		//}

		//std::vector<std::array<float, 4>> localQuaternions(header.bone_count);
		//for (int bone_id = 0; bone_id < header.bone_count; ++bone_id) {
		//	localQuaternions[bone_id] = rotMatToQuaternion(localRotations[bone_id]);
		//}

		//for (int id = 0; id < bone_definitions.size(); ++id) {
		//	const auto& bone = bone_definitions[id];
		//	
		//	ret.emplace_back(id, bone.parent_bone_id, std::string(bone.name), localPositions[id].data(), localQuaternions[id].data());
		//}

		//return ret;

		////NEW CODE end 

		//TODO: Doesn't export rotations at all, 

		//std::vector<Quaternion> Rwc(bone_definitions.size());
		//Rwc[0] = bind_pose[0].localTransformation;

		//std::vector<Vec<float, 3>> positions;
		//positions.emplace_back(0.f, 0.f, 0.f);
		//for (int bone_id = 1; bone_id < bone_definitions.size(); ++bone_id) {
		//	const auto& bone = bone_definitions[bone_id];

		//	const Quaternion& Rwp = Rwc[bone.parent_bone_id];
		//	const Quaternion& Ric = bind_pose[bone_id].localTransformation;
		//	Rwc[bone_id] = Rwp * Ric;
		//	
		//	Quaternion Pwp = Quaternion(bind_pose[bone_id].position.xyz(), 0.f);
		//	Quaternion RwpConj = conjugate(Rwp);
		//	positions.push_back((Rwp * Pwp * RwpConj).xyz()); //quaternion rotation q.a.q'
		//	//std::swap(positions[bone_id].y(), positions[bone_id].z());
		//}

		//std::vector<IRig::Bone> ret;
		//ret.reserve(bone_definitions.size());

		//for (int id = 0; id < bone_definitions.size(); ++id) {
		//	const auto& bone = bone_definitions[id];
		//	ret.emplace_back(id, bone.parent_bone_id, std::string(bone.name), positions[id].data());
		//	//memcpy_s(ret.back().bind_pose, 16 * sizeof(float), bind
		//}

		//return ret;

		//Absolute pos variant
		std::vector<IRig::Bone> ret;
		ret.reserve(bone_definitions.size());

		std::vector<Transform> globalbindMats(header.bone_count);
		for (unsigned int bone_id = 0; bone_id < header.bone_count; ++bone_id) {
			globalbindMats[bone_id] = bind_pose_inverse_global_mats[bone_id].inverse();
		}

		std::vector < std::array<float, 3> > localPositions(header.bone_count);
		for (unsigned int bone_id = 0; bone_id < header.bone_count; ++bone_id) {
			auto parent_id = bone_definitions[bone_id].parent_bone_id;
			if (parent_id == -1) {
				localPositions[bone_id][0] = globalbindMats[bone_id][12];
				localPositions[bone_id][1] = globalbindMats[bone_id][13];
				localPositions[bone_id][2] = globalbindMats[bone_id][14];
			}
			else {
				localPositions[bone_id][0] = globalbindMats[bone_id][12] - globalbindMats[parent_id][12];
				localPositions[bone_id][1] = globalbindMats[bone_id][13] - globalbindMats[parent_id][13];
				localPositions[bone_id][2] = globalbindMats[bone_id][14] - globalbindMats[parent_id][14];
			}
		}

		for (int id = 0; id < bone_definitions.size(); ++id) {
			const auto& bone = bone_definitions[id];
			ret.emplace_back(id, bone.parent_bone_id, std::string(bone.name), localPositions[id].data());
			//memcpy_s(ret.back().bind_pose, 16 * sizeof(float), bind
		}

		//std::vector<Transform> localTransform(header.bone_count);
		//for (int bone_id = 0; bone_id < header.bone_count; ++bone_id) {
		//	auto parent_id = bone_definitions[bone_id].parent_bone_id;
		//	if (parent_id == -1)
		//		localTransform[bone_id] = globalbindMats[bone_id];
		//	else {
		//		auto inv = globalbindMats[parent_id].inverse();
		//		auto mul = inv.multiply(globalbindMats[bone_id]);
		//		localTransform[bone_id] = mul;
		//	}
		//}

		//for (int id = 0; id < bone_definitions.size(); ++id) {
		//	const auto& bone = bone_definitions[id];
		//	ret.emplace_back(id, bone.parent_bone_id, std::string(bone.name), &localTransform[id][12]);
		//	//memcpy_s(ret.back().bind_pose, 16 * sizeof(float), bind
		//}

		return ret;
	}

	std::vector<float> BORG::getInverseGlobalBindMatrices() const
	{
		auto size = bind_pose_inverse_global_mats.size() * 16;
		std::vector<float> ret(size);
		memcpy(ret.data(), bind_pose_inverse_global_mats.data(), size * sizeof(float));
		return ret;
	}

	void BORG::setFromCanonicalBoneList(const std::vector<IRig::Bone>& bones) {
		throw UnsupportedFeatureException("BORG::setFromCanonicalBoneList not implemented");
	}