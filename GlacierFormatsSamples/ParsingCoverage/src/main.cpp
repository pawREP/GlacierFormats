#include "GlacierFormats.h"
#include <filesystem>
#include <string>
#include <iostream>

using namespace GlacierFormats;

struct Coverage {
	int total;

	int good; //Parsed without throwing exception
    int unsupported; //Exception thrown during parsing because of known limitations like unsupported features, compression formats etc.
    int expected_failure; //Exception thrown during parsing because of known limitations like unsupported features, compression formats etc.
    int unexpected_failure; //Fatal exception thrown during parsing. This idicates logic errors like bad asserts, out of bound reads etc. 

	Coverage(int total) : total(total), good(0), unsupported(0), expected_failure(0), unexpected_failure(0) {}

	void print() {
		std::string report = "Good:\t\t" + std::to_string(good) + " (" + std::to_string(100.f * static_cast<float>(good)/static_cast<float>(total)) + "%)" + "\n" +
			"Unsupported:\t" + std::to_string(unsupported) + " (" + std::to_string(100.f * static_cast<float>(unsupported) / static_cast<float>(total)) + "%)" + "\n" +
			"Expected Bad:\t" + std::to_string(expected_failure) + " (" + std::to_string(100.f * static_cast<float>(expected_failure) / static_cast<float>(total)) + "%)" + "\n" +
			"Unexpected Bad:\t" + std::to_string(unexpected_failure) + " (" + std::to_string(100.f * static_cast<float>(unexpected_failure) / static_cast<float>(total)) + "%)" + "\n";

		printf("%s\n", report.c_str());
	}
};

template<typename Type> 
void testCoverage(const std::string& type_string) {
	auto repo = ResourceRepository::instance();
	auto ids = repo->getIdsByType(type_string.c_str());

	printf("Testing parser coverage of %I64u %s resources.\n\n", ids.size(), type_string.c_str());

	Coverage coverage(ids.size());
	int cnt = 0;
	for (const auto& id : ids) {
		try {
			std::unique_ptr<Type> resource = repo->getResource<Type>(id);
			coverage.good++;
		}
		catch (const UnsupportedFeatureException& e) {
			coverage.unsupported++;
		}
		catch (...) {
			coverage.unexpected_failure++;
		}
		if(cnt % (ids.size()/100) == 0)
			printf("%d, ", static_cast<int>(100.f *static_cast<float>(cnt)/static_cast<float>(ids.size())));
		++cnt;
	}
	printf("\n\n");

	coverage.print();
}


int main(int argc, char** argv) {
	//Initilize GlacierFormats with a runtime directory.
	GlacierInit();
	
	//testCoverage<PRIM>("PRIM");
	testCoverage<TEXD>("TEXD");

	std::cin.ignore();
}
