#pragma once
#include <filesystem>
#include <fstream>
#include <vector>

namespace GlacierFormats {

	class IBinaryWriterSink {
	public:
		virtual void write(const char* read_buffer, int len) = 0;
		virtual void seek(int64_t offset) = 0;
		virtual int64_t tell() = 0;
		virtual void close() = 0;
		virtual std::vector<char> release() = 0;
	};

	class BinaryWriterFileSink : public IBinaryWriterSink {
	private:
		std::ofstream ofs;
	public:
		BinaryWriterFileSink(const std::filesystem::path file) {
			ofs.exceptions(std::ios::failbit | std::ios::badbit);
			ofs.open(file, std::ios::binary);
		}

		~BinaryWriterFileSink() {
			if (ofs.is_open())
				ofs.close();
		}

		void write(const char* read_buffer, int len) override final {
			ofs.write(read_buffer, len);
		}

		void seek(int64_t offset) override final {
			ofs.seekp(offset, std::ios::beg);
		}

		int64_t tell() override final {
			return static_cast<int64_t>(ofs.tellp());
		}

		void close() override final {
			ofs.close();
		}

		std::vector<char> release() override final {
			ofs.close();
			return std::vector<char>();
		}
	};

	class BinaryWriterBufferSink : public IBinaryWriterSink {
	private:
		std::vector<char> data;
		int64_t cur;

		void resizeIfNecessary(int write_len) {
			if (cur + write_len > data.size())
				data.resize(cur + write_len);
		}

	public:
		BinaryWriterBufferSink() : cur(0) {

		}

		void write(const char* read_buffer, int len) override final {
			if (!len)
				return;
			resizeIfNecessary(len);
			memcpy_s(&data[cur], len, read_buffer, len);
			cur += len;
		}

		void seek(int64_t offset) override final {
			if (offset > data.size())
				data.resize(offset);
			cur = offset;
		}

		int64_t tell() override final {
			return cur;
		}

		void close() override final {
			return;
		}
		
		std::vector<char> release() override final {
			return std::move(data);
		}
	};

	class BinaryWriter {
	private:
		std::unique_ptr<IBinaryWriterSink> sink;

	public:
		BinaryWriter(const std::filesystem::path& file) {
			sink = std::make_unique<BinaryWriterFileSink>(file);
		}

		BinaryWriter() {
			sink = std::make_unique<BinaryWriterBufferSink>();
		}

		template<typename T>
		void write(const T& value) {
			static_assert(std::is_trivially_copyable_v<T>);
			sink->write(reinterpret_cast<const char*>(&value), sizeof(T));
		}

		template<typename T>
		void write(const T* arr, int64_t arr_len) {
			static_assert(std::is_trivially_copyable_v<T>);
			sink->write(reinterpret_cast<const char*>(arr), sizeof(T)* arr_len);
		}

		template<unsigned int al = 0x10>
		void align() {
			char zero[al]{ 0 };
			uint64_t cur = sink->tell();
			uint64_t padding_len = (al - cur) % al;
			write(zero, padding_len);
		}

		int64_t tell() {
			return sink->tell();
		}

		void seek(int64_t offset) {
			sink->seek(offset);
		}

		void writeBEString(const std::string& str) {
			write(str.c_str(), str.length());
		}

		void writeLEString(const std::string& str) {
			std::string tmp_str = str;
			std::reverse(tmp_str.begin(), tmp_str.end());
			writeBEString(tmp_str);
		}

		void writeCString(const std::string& str) {
			writeBEString(str);
			write('\0');
		}

		std::vector<char> release() {
			return std::move(sink->release());
		}
	};

}