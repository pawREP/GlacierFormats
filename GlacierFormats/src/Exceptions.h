#pragma once
#include <exception>
#include <string>

namespace GlacierFormats {

	class UnsupportedFeatureException : public std::exception {
	private:
		std::string msg;
	public:
		UnsupportedFeatureException(const std::string& msg);
		const char* what() const override final;
	};

	class AssertionException : public std::exception {
	private:
		std::string msg;
	public:
		AssertionException(const std::string& msg);
		const char* what() const override final;
	};

	class InvalidArgumentsException : public std::exception {
	private:
		std::string msg;
	public:
		InvalidArgumentsException(const std::string& msg);
		const char* what() const override final;
	};

#define GLACIER_ASSERT_TRUE(expr) if(!(expr)) throw AssertionException(#expr);
}

