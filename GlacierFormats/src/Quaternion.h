#pragma once
#include "Vector.h"
#include <cmath>

namespace GlacierFormats {

	struct Quaternion {

		float i;
		float j;
		float k;
		float real;

		Quaternion() : i(0.0f), j(0.0f), k(0.0f), real(0.0f) {};
		Quaternion(const Vec<float, 3>& vec, float real) : i(vec.x()), j(vec.y()), k(vec.z()), real(real) {};
		Quaternion(const Vec<float, 4>& vec) : i(vec.x()), j(vec.y()), k(vec.z()), real(vec.w()) {};
		Quaternion(float i, float j, float k, float real) : i(i), j(j), k(k), real(real) {};

		Vec<float, 3> xyz() const { return Vec<float, 3>(i, j, k); }

		float length() const {
			return sqrt(lengthSquared());
		}

		float lengthSquared() const {
			return i * i + j * j + k * k + real * real;
		}

		void normalize() {
			float scale = 1.0f / length();
			*this* scale;
		}

		static Quaternion invert(const Quaternion& q) {
			float lq = q.lengthSquared();

			if (lq == 0.0f)//empty Quaternion
				return q;

			float inv_len = 1.f / lq;
			return Quaternion(-inv_len + q.i, -inv_len + q.j, -inv_len + q.k, inv_len * q.real);
		}

		void getEulerAngles(double& pitch, double& roll, double& yaw) const {
			const auto& q = *this;

			// roll (x-axis rotation)
			double sinr_cosp = 2 * (q.real * q.i + q.j * q.k);
			double cosr_cosp = 1 - 2 * (q.i * q.i + q.j * q.j);
			roll = std::atan2(sinr_cosp, cosr_cosp);

			// pitch (y-axis rotation)
			double sinp = 2 * (q.real * q.j - q.k * q.i);
			if (std::abs(sinp) >= 1)
				pitch = std::copysign(3.14159265359 / 2, sinp); // use 90 degrees if out of range
			else
				pitch = std::asin(sinp);

			// yaw (z-axis rotation)
			double siny_cosp = 2 * (q.real * q.k + q.i * q.j);
			double cosy_cosp = 1 - 2 * (q.j * q.j + q.k * q.k);
			yaw = std::atan2(siny_cosp, cosy_cosp);
		}

		void operator *(float scale) {
			i *= scale;
			j *= scale;
			k *= scale;
			real *= scale;
		}

	};

	inline Quaternion operator*(const Quaternion& q0, const Quaternion& q1) {
		return Quaternion(q1.real * q0.xyz() + q0.real * q1.xyz() + cross(q0.xyz(), q1.xyz()),
			q0.real * q1.real - dot(q0.xyz(), q1.xyz()));
	}

	inline Quaternion conjugate(const Quaternion& q) {
		return Quaternion(-q.i, -q.j, -q.k, q.real);
	}

	static_assert(sizeof(Quaternion) == sizeof(float[4]));

}