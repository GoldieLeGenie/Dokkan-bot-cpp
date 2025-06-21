#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <filesystem>
#include "utils.hpp"
#include "config.hpp"

bool load_save_file(Config& cfg, const std::string& filename) {
    std::string file_name = filename;

    if (file_name.empty()) {
        std::cout << "Enter save file name: ";
        std::getline(std::cin, file_name);
    }

    std::string file_path = "saves/" + file_name;
    std::ifstream file(file_path);
    if (!file.is_open()) {
        std::cerr << "File not found.\n";
        return false;
    }

    std::map<std::string, std::string> data;
    std::string line;
    while (std::getline(file, line)) {
        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
            std::string key = line.substr(0, colon_pos);
            std::string value = line.substr(colon_pos + 1);

            // Trim whitespace
            key.erase(0, key.find_first_not_of(" \t\r\n"));
            key.erase(key.find_last_not_of(" \t\r\n") + 1);
            value.erase(0, value.find_first_not_of(" \t\r\n"));
            value.erase(value.find_last_not_of(" \t\r\n") + 1);

            data[key] = value;
        }
    }

    // Remplir le cfg si les clés existent
    if (data.count("AdId"))       cfg.AdId = data["AdId"];
    if (data.count("platform"))   cfg.platform = data["platform"];
    if (data.count("UniqueId"))   cfg.UniqueId = data["UniqueId"];
    if (data.count("identifier")) cfg.identifier = data["identifier"];

    return true;
}


void create_file(const std::string& AdId, const std::string& UniqueId, const std::string& platform, const std::string& identifier) {
    namespace fs = std::filesystem;

    if (!fs::exists("saves")) {
        fs::create_directories("saves");
    }

    std::string filename;
    std::cout << "Enter a name for the file: ";
    std::getline(std::cin, filename);

    if (filename.empty() || filename.find_first_not_of(" \t") == std::string::npos) {
        std::cout << "The file name cannot be empty or contain only spaces." << std::endl;
        create_file(AdId, UniqueId, platform, identifier);
        return;
    }

    std::string filepath = "saves/" + filename;
    if (fs::exists(filepath)) {
        std::cout << "A file with this name already exists. Please choose another name." << std::endl;
        create_file(AdId, UniqueId, platform, identifier);
        return;
    }

    try {
        std::ofstream file(filepath);
        file << "AdId: " << AdId << "\n";
        file << "UniqueId: " << UniqueId << "\n";
        file << "platform: " << platform << "\n";
        file << "identifier: " << identifier << "\n";
        file.close();

        std::cout << "--------------------------------------------\n";
        std::cout << "Written details to file: " << filename << "\n";
        std::cout << "If " << filename << " is deleted your account will be lost if you haven't linked it!\n";
        std::cout << "--------------------------------------------\n";
    }
    catch (const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << "\n";
    }
}