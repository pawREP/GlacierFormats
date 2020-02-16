#pragma once
#include <cinttypes>
#include <cstring>
#include <type_traits>

namespace GlacierFormats {

	template<typename T, unsigned int Dim>
	struct Vec {
	private:
		T data_[Dim];

	public:
		using value_type = T;

		static size_t size() {
			return Dim;
		}

		[[nodiscard]] T* begin() noexcept {
			return &data_[0];
		}

		[[nodiscard]] const T* begin() const noexcept {
			return &data_[0];
		}

		[[nodiscard]] T* end() noexcept {
			return &data_[Dim];
		}

		[[nodiscard]] const T* end() const noexcept {
			return &data_[Dim];
		}

		[[nodiscard]] T* data() noexcept {
			return begin();
		}

		[[nodiscard]] const T* data() const noexcept {
			return begin();
		}

		Vec() noexcept {
			memset(data_, 0, size());
		}

		Vec(const Vec& o) = default;

		explicit Vec(const T& x) noexcept {
			static_assert(Dim == 1);
			data_[0] = x;
		}

		Vec(const T& x, const T& y) noexcept {
			static_assert(Dim == 2);
			data_[0] = x;
			data_[1] = y;
		}

		Vec(const T& x, const T& y, const T& z) noexcept {
			static_assert(Dim == 3);
			data_[0] = x;
			data_[1] = y;
			data_[2] = z;
		}

		Vec(const T& x, const T& y, const T& z, const T& w) noexcept {
			static_assert(Dim == 4);
			data_[0] = x;
			data_[1] = y;
			data_[2] = z;
			data_[3] = w;
		}

		[[nodiscard]] T& x() noexcept {
			static_assert(Dim > 0);
			return data_[0];
		}

		[[nodiscard]] const T& x() const noexcept {
			static_assert(Dim > 0);
			return data_[0];
		}

		[[nodiscard]] T& y() noexcept {
			static_assert(Dim > 1);
			return data_[1];
		}

		[[nodiscard]] const T& y() const noexcept {
			static_assert(Dim > 1);
			return data_[1];
		}

		[[nodiscard]] T& z() noexcept {
			static_assert(Dim > 2);
			return data_[2];
		}

		[[nodiscard]] const T& z() const noexcept {
			static_assert(Dim > 2);
			return data_[2];
		}

		[[nodiscard]] T& w() noexcept {
			static_assert(Dim > 3);
			return data_[3];
		}

		[[nodiscard]] const T& w() const noexcept {
			static_assert(Dim > 3);
			return data_[3];
		}

		[[nodiscard]] T& operator[](uint32_t idx) {
			return data_[idx];
		}

		[[nodiscard]] const T& operator[](uint32_t idx) const {
			return data_[idx];
		}

		void operator-=(const Vec& o) noexcept {
			for (int i = 0; i < Dim; ++i)
				this[i] -= o[i];
		}

		void operator+=(const Vec& o) noexcept {
			for (int i = 0; i < Dim; ++i)
				this->operator[](i) += o[i];
		}

		void operator*=(const Vec& o) noexcept {
			for (int i = 0; i < Dim; ++i)
				this[i] *= o[i];
		}

		void operator/=(const Vec& o) noexcept {
			for (int i = 0; i < Dim; ++i)
				this[i] /= o[i];
		}

		[[nodiscard]] bool operator==(const Vec& o) const noexcept {
			for (int i = 0; i < Dim; ++i)
				if (data_[i] != o[i])
					return false;
			return true;
		}

		[[nodiscard]] bool operator!=(const Vec& o) const noexcept {
			return (*this == o);
		}

		template<typename ScaleT>
		[[nodiscard]] Vec operator*(ScaleT s) noexcept {
			static_assert(std::is_convertible_v<ScaleT, Vec::value_type>);
			Vec r = *this;
			for (auto v : r)
				v *= static_cast<Vec::value_type>(s);
			return r;
		}

		[[nodiscard]] Vec<T, 3> xyz() const noexcept {
			static_assert(Dim >= 3);
			return Vec<T, 3>(x(), y(), z());
		}

	};

	template<typename T, unsigned int Dim>
	[[nodiscard]] Vec<T, Dim> operator-(const Vec<T, Dim>& v0, const Vec<T, Dim>& v1) noexcept {
		Vec<T, Dim> r = v0;
		for (int i = 0; i < v0.size(); ++i)
			r[i] -= v1[i];
		return r;
	}

	template<typename T, unsigned int Dim>
	[[nodiscard]] Vec<T, Dim> operator+(const Vec<T, Dim>& v0, const Vec<T, Dim>& v1) noexcept {
		Vec<T, Dim> r = Vec<T, Dim>(v0);
		for (int i = 0; i < Dim; ++i)
			r[i] += v1[i];
		return r;
	}

	template<typename T, unsigned int Dim>
	[[nodiscard]] Vec<T, Dim> operator*(const Vec<T, Dim>& v0, const Vec<T, Dim>& v1) noexcept {
		Vec<T, Dim> r = Vec<T, Dim>(v0);
		for (int i = 0; i < Dim; ++i)
			r[i] *= v1[i];
		return r;
	}

	template<typename T, unsigned int Dim>
	[[nodiscard]] Vec<T, Dim> operator/(const Vec<T, Dim>& v0, const Vec<T, Dim>& v1) noexcept {
		auto r = Vec<T, Dim>(v0);
		for (int i = 0; i < Dim; ++i)
			r[i] /= v1[i];
		return r;
	}


	template<typename T, unsigned int Dim, typename ScaleT>
	[[nodiscard]] Vec<T, Dim> operator*(ScaleT s, const Vec<T, Dim>& vec) noexcept {
		Vec<T, Dim> r = vec;
		for (auto& v : r)
			v *= s;
		return r;
	}

	template<typename T>
	[[nodiscard]] constexpr Vec<T, 3> cross(const Vec<T, 3>& v0, const Vec<T, 3>& v1) noexcept {
		Vec<T, 3> v;
		v[0] = v0[1] * v1[2] - v0[2] * v1[1];
		v[1] = v0[2] * v1[0] - v0[0] * v1[2];
		v[2] = v0[0] * v1[1] - v0[1] * v1[0];
		return v;
	}

	template<typename T>
	[[nodiscard]] constexpr T dot(const Vec<T, 3>& v0, const Vec<T, 3>& v1) noexcept {
		T sum{};
		for (size_t i = 0; i < v0.size(); ++i)
			sum += v0[i] * v1[i];
		return sum;
	}
}