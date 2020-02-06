#pragma once
#include <memory>
#include <assert.h>
#include <filesystem>
#include "BinaryReader.hpp"
#include "BinaryWriter.hpp"
#include "GlacierTypes.h"

namespace GlacierFormats {

	//TODO: Make template param a concept
	template<typename T>
	class GlacierResource {

	private:
		static RuntimeId getRuntimeIdFromPath(const std::filesystem::path& path);

	protected:
		GlacierResource(RuntimeId id);

	public:
		RuntimeId id;

		virtual std::string name() const;

		static std::unique_ptr<T> readFromFile(const std::filesystem::path& file_path, RuntimeId id = 0) {
			if(!id)
				id = getRuntimeIdFromPath(file_path);
			BinaryReader br(file_path);
			return std::make_unique<T>(br, id);
		}

		static std::unique_ptr<T> readFromBuffer(std::unique_ptr<char[]> buf, size_t buf_len, RuntimeId id) {
			BinaryReader br(std::move(buf), buf_len);
			return std::make_unique<T>(br, id);
		}

		void serializeToFile(const std::filesystem::path& file_path) {
			BinaryWriter bw(file_path);
			reinterpret_cast<T*>(this)->serialize(bw);
		}

		int64_t serializeToBuffer(std::unique_ptr<char[]>& data) {
			BinaryWriter bw;
			reinterpret_cast<T*>(this)->serialize(bw);
			auto buffer = bw.release();
			data = std::make_unique<char[]>(buffer.size());
			memcpy_s(data.get(), buffer.size(), buffer.data(), buffer.size());
			return buffer.size();
		}
	};

	template<typename T>
	inline RuntimeId GlacierResource<T>::getRuntimeIdFromPath(const std::filesystem::path& path) {
		auto file_name = path.stem().generic_string();
		auto id = strtoull(file_name.c_str(), 0, 16);
		if (!id)
			throw std::runtime_error("Invalid file name passed to readFromFile. Valid file names have to be runtime ids in hex format. \"../0046388A1B39A619.PRIM\"");
		return id;
	}

	template<typename T>
	inline GlacierResource<T>::GlacierResource(RuntimeId id) : id(id) {

	}

	template<typename T>
	inline std::string GlacierResource<T>::name() const
	{
		char str_buf[0x16];
		sprintf_s(str_buf, "%I64X", static_cast<uint64_t>(id));
		return std::string(str_buf);
	}

}