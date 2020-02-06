#include "Exceptions.h"

GlacierFormats::UnsupportedFeatureException::UnsupportedFeatureException(const std::string& msg) : 
	msg("UnsupportedFeatureException: " + msg) 
{
}

const char* GlacierFormats::UnsupportedFeatureException::what() const {
	return msg.c_str();
}

GlacierFormats::AssertionException::AssertionException(const std::string& msg) :
	msg("AssertionException: " + msg)
{
}

const char* GlacierFormats::AssertionException::what() const {
	return msg.c_str();
}

GlacierFormats::InvalidArgumentsException::InvalidArgumentsException(const std::string& msg) :
	msg("InvalidArgumentsException: " + msg)
{
}

const char* GlacierFormats::InvalidArgumentsException::what() const {
	return msg.c_str();
}
