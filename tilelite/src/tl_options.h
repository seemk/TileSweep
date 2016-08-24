#pragma once

#include <unordered_map>
#include <string>

typedef std::unordered_map<std::string, std::string> tl_options;

tl_options parse_options(int argc, char** argv);
