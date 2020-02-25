#include "GlacierFormats.h"
#include <filesystem>
#include <string>
#include <iostream>

using namespace GlacierFormats;

struct Coverage {
	int total;

	int good; //Parsed without throwing exception
    int unsupported; //Exception thrown during parsing because of known limitations like unsupported features, compression formats etc.
    int failure; //Fatal exception thrown during parsing. This idicates logic errors like bad asserts, out of bound reads etc. 

	Coverage(int total) : total(total), good(0), unsupported(0), failure(0) {}

	void print() {
		std::string report = "Good:\t\t" + std::to_string(good) + " (" + std::to_string(100.f * static_cast<float>(good)/static_cast<float>(total)) + "%)" + "\n" +
			"Unsupported:\t" + std::to_string(unsupported) + " (" + std::to_string(100.f * static_cast<float>(unsupported) / static_cast<float>(total)) + "%)" + "\n" +
			"Bad:\t\t" + std::to_string(failure) + " (" + std::to_string(100.f * static_cast<float>(failure) / static_cast<float>(total)) + "%)" + "\n";

		printf("%s\n", report.c_str());
	}
};

void writeToBinaryFile(std::filesystem::path& path, char* data, size_t data_size) {
	std::ofstream ofs(path, std::ios::binary);
	ofs.write(data, data_size);
	ofs.close();
}

template<typename Source = BinaryReaderBufferSource>
BinaryReader getResourceReader(RuntimeId id) {
	std::unique_ptr<char[]> data = nullptr;
	auto data_size = ResourceRepository::instance()->getResource(id, data);
	auto source = std::make_unique<Source>(std::move(data), data_size);
	return BinaryReader(std::move(source));
}

//Gets the ids of all prim resources that don't utilize buffer or mesh reuse.
std::vector<RuntimeId> getPrimResourcesWithoutSharedBuffers() {
	std::vector<RuntimeId> out_ids;
	
	auto repo = ResourceRepository::instance();
	auto prim_ids = repo->getIdsByType("PRIM");

	for (const auto& prim_id : prim_ids) {
		BinaryReader br = getResourceReader<LoggedBinaryReaderSource<BinaryReaderBufferSource>>(prim_id);
		std::unique_ptr<PRIM> prim = nullptr;
		try {
			prim = GlacierResource<PRIM>::read(br, prim_id);
		}
		catch (const UnsupportedFeatureException& e) {
			continue;
		}

		const auto* br_source = dynamic_cast<const LoggedBinaryReaderSource<BinaryReaderBufferSource>*>(br.getSource());

		//Assert read coverage is 100%
		GLACIER_ASSERT_TRUE(br_source->converage() == 1.0f);

		//Select asserts without double read or in other words, assets that don't contains submeshes with shared resource buffers. 
		//We have to filter those since reserializing them perfectly is not possible for a varity of reasons.
		const auto& access_pattern = br_source->getAccessPattern();
		if (std::find_if(access_pattern.begin(), access_pattern.end(), [](const auto& c) -> bool {return c != 1; }) == access_pattern.end()) {
			
			//Serialize resource to buffer and compare length to original
			auto reserialized_prim_buffer = prim->serializeToBuffer();
			
			if (reserialized_prim_buffer.size() != br_source->size()) {
				printf("%s\n", static_cast<std::string>(prim_id).c_str());

				std::filesystem::path path = R"(E:\RE\projects\RE_Hitman2\PRIM2\Data\tmp)";

				std::unique_ptr<char[]> data_o = nullptr;
				auto data_size_o = ResourceRepository::instance()->getResource(prim_id, data_o);
			
				writeToBinaryFile(path / (static_cast<std::string>(prim_id) + "_o.PRIM"), data_o.get(), data_size_o);
				writeToBinaryFile(path / (static_cast<std::string>(prim_id) + "_r.PRIM"), reserialized_prim_buffer.data(), reserialized_prim_buffer.size());

				std::cin.ignore();
			}

			out_ids.push_back(prim_id);
		}
	}
	printf("end\n");
	exit(0);
	return out_ids;
}

template<typename Type> 
void testCoverage(const std::string& type_string) {
	auto repo = ResourceRepository::instance();
	auto ids = repo->getIdsByType(type_string.c_str());

	printf("Testing parser coverage of %I64u %s resources.\n\n", ids.size(), type_string.c_str());

	Coverage coverage(ids.size());
	int cnt = 0;
	int double_read_cnt = 0;
	for (const auto& id : ids) {
		//printf("%d\t: 0x%s\n", cnt, static_cast<std::string>(id).c_str());
		try {
			//Save original to file.

			std::unique_ptr<char[]> data_o = nullptr;
			auto data_size_o = repo->getResource(id, data_o);
			std::ofstream ofs(R"(E:\RE\projects\RE_Hitman2\PRIM2\Data\tmp\)" + static_cast<std::string>(id) + "_o.PRIM", std::ios::binary);
			ofs.write(data_o.get(), data_size_o);
			ofs.close();

			if (id == 0x001807cbfb9d1ef0)
				int bre = 0;

			auto refs = repo->getResourceReferences(id);
			std::vector<std::string> ref_types;
			for (const auto& ref : refs) {
				ref_types.push_back(repo->getResourceType(ref.id));
			}

			//Parse and reserialize to file
			auto source = std::make_unique<LoggedBinaryReaderSource<BinaryReaderBufferSource>>(data_o.get(), data_size_o);
			BinaryReader br(std::move(source));
			std::unique_ptr<Type> resource = GlacierResource<Type>::read(br, id);

			//check overage
			//const auto* src = dynamic_cast<const LoggedBinaryReaderSource<BinaryReaderBufferSource>*>(br.getSource());
			//auto read_coverage = src->converage();
			//const auto& access_pattern = src->getAccessPattern();

			//if (std::find_if(access_pattern.begin(), access_pattern.end(), [](const auto& c) -> bool {return c != 1; }) != access_pattern.end()) {
			//	//PRIM has double read.
			//	++double_read_cnt;
			//}

			////If coverage isn't 100% print file and first loc of first unparsed byte
			//if (read_coverage != 1.0) {
			//	auto first = std::find(access_pattern.begin(), access_pattern.end(), 0);
			//	auto offset = first - access_pattern.begin();

			//	GLACIER_DEBUG_PRINT("id: %s\ncoverage: %f\nunparsed byte off: %I64X\n", 
			//		static_cast<std::string>(id).c_str(), 
			//		read_coverage, 
			//		offset);
			//}

			if (id == 0x001807cbfb9d1ef0)
				int bre = 0;

			//Reserialize
			std::unique_ptr<char[]> data_r = nullptr;
			auto data_size_r = resource->serializeToBuffer(data_r);
			std::ofstream ofsr(R"(E:\RE\projects\RE_Hitman2\PRIM2\Data\tmp\)" + static_cast<std::string>(id) + "_r.PRIM", std::ios::binary);
			ofsr.write(data_r.get(), data_size_r);
			ofsr.close();

			//if (data_size_r != data_size_o)
			//	printf("%s\n", static_cast<std::string>(id).c_str());
			//else
			//printf("%s ", static_cast<std::string>(id).c_str());

			//std::unique_ptr<Type> resource_r = GlacierResource<Type>::readFromBuffer(std::move(data_r), data_size_r, id);
			coverage.good++;

		}
		catch (const UnsupportedFeatureException& e) {
			printf("%s: %s\n", static_cast<std::string>(id).c_str(), e.what());
			coverage.unsupported++;
		}
		++cnt;
	}
	printf("\n\n");

	coverage.print();
}


int main(int argc, char** argv) {
	//Initilize GlacierFormats with a runtime directory.
	GlacierInit();
	
	auto hash = Util::meshInstallDebugHookHash(0x55744ce96820e2);//Church confession c

	auto ids = getPrimResourcesWithoutSharedBuffers();

	testCoverage<PRIM>("PRIM");
	//testCoverage<TEXD>("TEXD");
	//testCoverage<MATI>("MATI");

	std::cout << "done!\n";
	std::cin.ignore();
}
