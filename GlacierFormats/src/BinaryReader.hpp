#pragma once
#include "Exceptions.h"
#include <memory>
#include <fstream>
#include <filesystem>
#include <assert.h>
#include <sstream>
#include <algorithm>
#include <memory>
#include <numeric>


namespace GlacierFormats {

	class IBinaryReaderSource {
	public:
		virtual void read(char* dst, int64_t len) = 0;
		virtual void peek(char* dst, int64_t len) = 0;//Peak is defined at the IBinaryReaderSource interface level rather than in the Reader to make it accessable for source mixins. (see LoggedBinaryReaderSource)
		virtual void seek(int64_t offset) = 0;
		virtual int64_t tell() = 0; //can't be const because ifstream equivalent isn't const.
		virtual int64_t size() const = 0;

		virtual ~IBinaryReaderSource() {};
	};

	class BinaryReaderFileSource : public IBinaryReaderSource {
	private:
		const int64_t size_;
		std::ifstream ifs;

	public:
		BinaryReaderFileSource(std::filesystem::path path) : size_(std::filesystem::file_size(path)) {
			ifs.exceptions(std::ios::failbit | std::ios::badbit);
			ifs.open(path, std::ios::binary);
		}

		void read(char* dst, int64_t len) override {
			ifs.read(dst, len);
		}

		void peek(char* dst, int64_t len) override {
			auto o = tell();
			ifs.read(dst, len);
			seek(o);
		}

		void seek(int64_t offset) override final {
			ifs.seekg(offset, std::ios::beg);
		}

		int64_t tell() override final {
			return static_cast<int64_t>(ifs.tellg());
		}

		int64_t size() const override final {
			return size_;
		}
	};

	class BinaryReaderBufferSource : public IBinaryReaderSource {
	private:
		std::unique_ptr<char[]> owned_read_buffer;

		const char* read_buffer;
		int64_t buffer_size;
		int64_t cur;

	public:

		//Non owning constructor
		BinaryReaderBufferSource(const char* data, int64_t data_size) {
			owned_read_buffer = nullptr;
			read_buffer = data;
			buffer_size = data_size;
			cur = 0;
		}

		//Owning constructor
		BinaryReaderBufferSource(std::unique_ptr<char[]> data, int64_t data_size) {
			owned_read_buffer = std::move(data);
			read_buffer = owned_read_buffer.get();
			buffer_size = data_size;
			cur = 0;
		}

		void read(char* dst, int64_t len) override {
			if (cur + len > buffer_size)
				throw InvalidArgumentsException("Out of bounds read");
			memcpy_s(dst, len, &(read_buffer[cur]), len);
			cur += len;
		}

		void peek(char* dst, int64_t len) override {
			if (cur + len > buffer_size)
				throw InvalidArgumentsException("Out of bounds read");
			memcpy_s(dst, len, &(read_buffer[cur]), len);
		}

		void seek(int64_t offset) override final {
			if(offset > buffer_size)
				throw InvalidArgumentsException("Out of bounds seek");
			cur = offset;
		}

		int64_t tell() override final {
			return cur;
		}

		int64_t size() const override final {
			return buffer_size;
		}
	};

	template<typename Source>
	class LoggedBinaryReaderSource : public Source {
		static_assert(std::is_base_of_v<IBinaryReaderSource, Source>);

	public:
		std::vector<char> access_pattern;

		template<typename... Args>
		LoggedBinaryReaderSource(Args&&... args) : Source(std::forward<Args>(args)...) {
			access_pattern.resize(Source::size(), 0);
		}

		void read(char* dst, int64_t len) override final {
			auto cur = Source::tell();
			Source::read(dst, len);
			for (int i = cur; i < cur + len; ++i)
				access_pattern[i]++;
		}

		const std::vector<char>& getAccessPattern() const {
			return access_pattern;
		}

		float converage() const {
			auto read_count = std::count_if(access_pattern.begin(), access_pattern.end(), [](const char& c) { return c != 0; });
			return static_cast<float>(read_count) / static_cast<float>(access_pattern.size());
		}

	};

	class BinaryReader {
		std::unique_ptr<IBinaryReaderSource> source;

	public:
		enum class Endianness { LE, BE };

		BinaryReader(BinaryReader&& br) noexcept {
			source = std::move(br.source);
		}

		BinaryReader(const std::filesystem::path& path) {
			source = std::make_unique<BinaryReaderFileSource>(path);
		}

		//Doesn't transfer ownership of data.
		BinaryReader(const char* data, int64_t data_size) {
			source = std::make_unique<BinaryReaderBufferSource>(data, data_size);
		}

		BinaryReader(std::unique_ptr<char[]> data, int64_t data_size) {
			source = std::make_unique<BinaryReaderBufferSource>(std::move(data), data_size);
		}

		BinaryReader(std::unique_ptr<IBinaryReaderSource> source) : source(std::move(source)) {};

		~BinaryReader() {
			int a = 0;
		}

		int64_t tell() {
			return source->tell();
		}

		void seek(size_t pos) {
			source->seek(pos);
		}

		int64_t size() const {
			return source->size();
		}

		template<typename T>
		void read(T* arr, int64_t len) {
			source->read(reinterpret_cast<char*>(arr), sizeof(T) * len);
		}

		template<typename T>
		void peek(T* arr, int64_t len) {
			source->peek(reinterpret_cast<char*>(arr), sizeof(T)* len);
		}

		template<typename T>
		T read() {
			T value;
			read(&value, 1);
			return value;
		};

		template<typename T>
		T peek() {
			T value;
			peek(&value, 1);
			return value;
		}

		template<unsigned int len, Endianness en>
		std::string readString() {
			std::string str(len, '\0');
			read(str.data(), len);
			if constexpr (en == Endianness::LE)
				std::reverse(str.begin(), str.end());
			return str;
		}

		std::string readCString() {
			char c = read<char>();
			std::vector<char> read_buffer(1, c);
			while (c != 0) {
				c = read<char>();
				read_buffer.push_back(c);
			}
			return std::string(read_buffer.begin(), read_buffer.end() - 1);
		}

		template<unsigned int alignment = 0x10>
		void align() {
			char zero[alignment];
			uint64_t pos = tell();
			uint64_t padding_len = (alignment - pos) % alignment;
			read(zero, padding_len);

			auto it = std::find_if(std::begin(zero), &zero[padding_len], [](const char& c) { return c != 0; });

			GLACIER_ASSERT_TRUE(("BinaryReader, invalid padding error", it == &zero[padding_len]));
		}

		const IBinaryReaderSource* getSource() const {
			return source.get();
		}
	};
}