#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <map>
#include "config.hpp"

void create_file(const std::string& AdId, const std::string& UniqueId, const std::string& platform, const std::string& identifier);
bool load_save_file(Config& cfg, const std::string& filename);

#endif // UTILS_HPP
