#include "PrimCopyBones.h"
#include "BinaryReader.hpp"
#include "BinaryWriter.hpp"

GlacierFormats::CopyBones::CopyBones(BinaryReader* br, int count) {
	copy_bones.resize(2 * count);
	for (auto& copy_bone : copy_bones)
		copy_bone = br->read<int>();
	br->align();
}

void GlacierFormats::CopyBones::serialize(BinaryWriter* bw) const {
	for (const auto& copy_bone : copy_bones)
		bw->write(copy_bone);
	bw->align();
}
