#pragma once
#include <vector>

namespace GlacierFormats {

	class BinaryReader;
	class BinaryWriter;

	class CopyBones	{
	private:
		std::vector<int> copy_bones;

	public:
		CopyBones(BinaryReader* br, int count);

		int copyBoneCount() const;

		void serialize(BinaryWriter* bw) const;
	};

}
