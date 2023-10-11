#pragma once

#include <string>
#include <stdexcept>

#define THROW(x) { throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__) + " " + std::string(__FUNCTION__) + "(): " + x); }
