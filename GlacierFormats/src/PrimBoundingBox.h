#pragma once
#include <vector>
#include <algorithm>
#include "Vector.h"
#include <type_traits>

namespace GlacierFormats {

	template<typename V>
	struct BoundingBox
	{
		V min;
		V max;

		BoundingBox() {
			for (auto& d : min)
				d = std::numeric_limits<V::value_type>::max();
			for (auto& d : max)
				d = std::numeric_limits<V::value_type>::min();
		}

		BoundingBox(const BoundingBox& bb) {
			min = bb.min;
			max = bb.max;
		}

		BoundingBox(const std::vector<V>& vec) : BoundingBox() {
			for (int i = 0; i < min.size(); ++i) {
				for (auto& v : vec) {
					min[i] = std::min(min[i], v[i]);
					max[i] = std::max(max[i], v[i]);
				}
			}
		}

		//TODO: Look the range compression again, it's likely slightly wrong. 
		template<typename T>
		void getIntegerRangeCompressionParameters(T* scale, T* bias) {

			for (int i = 0; i < min.size(); ++i)
				scale[i] = 0.5 * (max[i] - min[i]);

			for (int i = 0; i < min.size(); ++i)
				bias[i] = min[i] + scale[i];
		}

	};

	template<typename V>
	BoundingBox<V> operator+(const BoundingBox<V>& b0, const BoundingBox<V>& b1) {
		BoundingBox<V> bb;
		//TODO: IMpl
		for (int i = 0; i < bb.min.size(); ++i) {
			bb.min[i] = std::min(b0.min[i], b1.min[i]);
			bb.max[i] = std::max(b0.max[i], b1.max[i]);
		}
		return bb;
	}


}