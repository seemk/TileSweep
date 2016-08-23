#pragma once

#include <unordered_map>
#include <string>

typedef std::unordered_map<std::string, std::string> tilelite_config;

tilelite_config parse_args(int argc, char** argv);
