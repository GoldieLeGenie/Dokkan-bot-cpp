#include "database.hpp"
#include <sqlcipher/sqlite3.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <curl/curl.h>
#include "pysqlsimplecipher/decryptor.hpp"
#include "config.hpp"
#include "ClientSend.hpp"
#include <nlohmann/json.hpp>
#include "apiPath.hpp"
#include "cryption.hpp"
#include <optional>

namespace db {

    static sqlite3* db_ptr = nullptr;

    static std::optional<std::vector<std::map<std::string, std::string>>> cards_cache;
    static std::optional<std::vector<std::map<std::string, std::string>>> awakening_items_cache;
    static std::optional<std::vector<std::map<std::string, std::string>>> link_skills_cache;

    bool connect() {
        if (db_ptr) return true;
        if (!std::filesystem::exists("glb.db")) {
            std::cerr << "[!] Database file glb.db does not exist.\n";
            return false;
        }
        if (sqlite3_open("glb.db", &db_ptr) != SQLITE_OK) {
            std::cerr << "[!] Failed to open database: " << sqlite3_errmsg(db_ptr) << "\n";
            return false;
        }
        return true;
    }

    void close() {
        if (db_ptr) sqlite3_close(db_ptr);
        db_ptr = nullptr;
    }

    void reset_caches() {
        cards_cache.reset();
        awakening_items_cache.reset();
    }

    std::vector<std::map<std::string, std::string>> query_all(const std::string& table) {
        std::vector<std::map<std::string, std::string>> rows;
        std::string sql = "SELECT * FROM " + table + ";";
        sqlite3_stmt* stmt = nullptr;

        if (sqlite3_prepare_v2(db_ptr, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            std::cerr << "[!] An error occurred: " << sqlite3_errmsg(db_ptr) << "\n";
            return rows;
        }

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            std::map<std::string, std::string> row;
            for (int i = 0; i < sqlite3_column_count(stmt); ++i) {
                std::string col = sqlite3_column_name(stmt, i);
                const char* val = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i));
                row[col] = val ? val : "";
            }
            rows.push_back(std::move(row));
        }

        sqlite3_finalize(stmt);
        return rows;
    }

#define DEFINE_TABLE_ACCESSOR(name, table) \
    std::vector<std::map<std::string, std::string>> name() { return query_all(table); }

    DEFINE_TABLE_ACCESSOR(get_cards, "cards");
    DEFINE_TABLE_ACCESSOR(get_leader_skills, "leader_skills");
    DEFINE_TABLE_ACCESSOR(get_link_skills, "link_skills");
    DEFINE_TABLE_ACCESSOR(get_area_tabs, "area_tabs");
    DEFINE_TABLE_ACCESSOR(get_card_specials, "card_specials");
    DEFINE_TABLE_ACCESSOR(get_specials, "specials");
    DEFINE_TABLE_ACCESSOR(get_z_battle_stage_views, "z_battle_stage_views");
    DEFINE_TABLE_ACCESSOR(get_card_card_categories, "card_card_categories");
    DEFINE_TABLE_ACCESSOR(get_treasure_items, "treasure_items");
    DEFINE_TABLE_ACCESSOR(get_support_items, "support_items");
    DEFINE_TABLE_ACCESSOR(get_potential_items, "potential_items");
    DEFINE_TABLE_ACCESSOR(get_special_items, "special_items");
    DEFINE_TABLE_ACCESSOR(get_training_items, "training_items");
    DEFINE_TABLE_ACCESSOR(get_support_films, "support_films");
    DEFINE_TABLE_ACCESSOR(get_quests, "quests");
    DEFINE_TABLE_ACCESSOR(get_rank_statuses, "rank_statuses");
    DEFINE_TABLE_ACCESSOR(get_training_fields, "training_fields");
    DEFINE_TABLE_ACCESSOR(get_sugoroku_maps, "sugoroku_maps");
    DEFINE_TABLE_ACCESSOR(get_areas, "areas");
    DEFINE_TABLE_ACCESSOR(get_awakening_items, "awakening_items");

    const std::vector<std::map<std::string, std::string>>& get_cards_cached() {
        if (!cards_cache.has_value())
            cards_cache = get_cards();
        return *cards_cache;
    }

    const std::vector<std::map<std::string, std::string>>& get_awakening_items_cached() {
        if (!awakening_items_cache.has_value())
            awakening_items_cache = get_awakening_items();
        return *awakening_items_cache;
    }

    const std::vector<std::map<std::string, std::string>>& get_link_skills_cached() {
        if (!link_skills_cache.has_value())
            link_skills_cache = get_link_skills();
        return *link_skills_cache;
    }

} // namespace db

static size_t write_file(void* ptr, size_t size, size_t nmemb, void* stream) {
    std::ofstream* file = static_cast<std::ofstream*>(stream);
    size_t totalSize = size * nmemb;
    file->write(static_cast<char*>(ptr), totalSize);
    return totalSize;
}

void update_database(Config& cfg, ClientSend& client) {
    ApiUrl url;
    Cryption crypt(cfg.access_token, cfg.secret);
    client.setHeader("Authorization", crypt.mac("GET", "/client_assets/database"));

    cpr::Response response = client.get(url.DATABASE);
    if (response.status_code != 200) {
        std::cerr << "\033[1;31m[ERROR] Failed to check database version.\033[0m\n";
        return;
    }

    nlohmann::json json_data;
    try {
        json_data = nlohmann::json::parse(response.text);
    }
    catch (const std::exception& e) {
        std::cerr << "[ERROR] Invalid JSON format: " << e.what() << "\n";
        return;
    }

    if (!json_data.contains("version") || !json_data.contains("url")) {
        std::cerr << "[ERROR] Invalid response format.\n";
        return;
    }

    std::string new_version;
    try {
        if (json_data["version"].is_string())
            new_version = json_data["version"].get<std::string>();
        else if (json_data["version"].is_number_integer())
            new_version = std::to_string(json_data["version"].get<int>());
        else if (json_data["version"].is_number_float())
            new_version = std::to_string(json_data["version"].get<double>());
        else {
            std::cerr << "[ERROR] Unsupported type for version field.\n";
            return;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "[ERROR] Could not read version field: " << e.what() << "\n";
        return;
    }

    if (cfg.DATABASE_VER != new_version) {
        std::cout << "\033[1;33mDatabase out of date...\nUpdating...\033[0m\n";
        cfg.DATABASE_VER = new_version;

        std::string db_url = json_data["url"].get<std::string>();
        CURL* curl = curl_easy_init();
        if (curl) {
            std::ofstream file("enc_glb.db", std::ios::binary);
            curl_easy_setopt(curl, CURLOPT_URL, db_url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_file);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &file);
            CURLcode res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);
            file.close();

            if (res != CURLE_OK) {
                std::cerr << "\033[1;31m[ERROR] Failed to download database file: " << curl_easy_strerror(res) << "\033[0m\n";
                return;
            }

            db::close();
            std::cout << "\033[1;37mDecrypting database...\033[0m\n";
            decrypt_database("enc_glb.db");

            if (!db::connect()) {
                std::cerr << "\033[1;31m[ERROR] Could not reconnect to database after update.\033[0m\n";
                return;
            }

            db::reset_caches(); // reset all static caches

            std::cout << "\033[1;32mUpdate complete!\033[0m\n";
        }
        else {
            std::cerr << "\033[1;31m[ERROR] Failed to initialize CURL.\033[0m\n";
        }
    }
    else {
        std::cout << "\033[1;32mNo update needed!\033[0m\n";
    }
}
