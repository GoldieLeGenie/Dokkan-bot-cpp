#include "config.hpp"
#include <iostream>
#include <nlohmann/json.hpp>
#include <cpr/cpr.h>
#include "apiPath.hpp"
#include "ClientSend.hpp"
#include "auth.hpp"
#include "cryption.hpp"
#include <cstdlib>
#include <vector>
#include <algorithm> 
#include "database.hpp"

struct Carddatainfo {
    std::string id;
    std::string name;
    std::string rarity;
    std::string type;
    std::string image_url;
    std::string unique_id;
    bool awakened = false;
};



nlohmann::json get_user(Config& cfg, ClientSend& client, bool data) {
    ApiUrl url;
    Cryption crypt(cfg.access_token, cfg.secret);

    client.setHeader("Authorization", crypt.mac("GET", "/user"));

    cpr::Response response = client.get(url.NAME);

    if (response.status_code != 200) {
        std::cerr << "\033[1;31m[ERROR] Failed to get user data. HTTP "
            << response.status_code << "\033[0m\n";
        return {}; // retourne un objet JSON vide en cas d'erreur HTTP
    }

    nlohmann::json json_data;
    nlohmann::json user;

    try {
        json_data = nlohmann::json::parse(response.text);
        user = json_data["user"];
    }
    catch (const std::exception& e) {
        std::cerr << "\033[1;31m[ERROR] An error occurred." << "\033[0m\n";
        return {}; 
    }

    if (data) {
        std::map<std::string, std::string> user_info = {
            {"Account OS",           cfg.platform},
            {"Name",                 user["name"]},
            {"Rank",                 std::to_string(user["rank"].get<int>())},
            {"Stones",               std::to_string(user["stone"].get<int>())},
            {"Zeni",                 std::to_string(user["zeni"].get<int>())},
            {"User ID",              std::to_string(user["id"].get<uint64_t>())},
            {"Stamina",              std::to_string(user["act"].get<int>())},
            {"Total Card Capacity",  std::to_string(user["total_card_capacity"].get<int>())}
        };

        std::cout << "\033[1;36m======= [USER INFO] =======\033[0m\n";
        for (const auto& [key, value] : user_info) {
            std::cout << "[+] " << key << ": " << value << "\n";
        }
        std::cout << "\033[1;36m===========================\033[0m\n";
    }

    return user; // dans tous les cas, retourne l'objet user
}


std::vector<int64_t> get_gifts(Config& cfg, ClientSend& client) {
    ApiUrl url;
    Cryption crypt(cfg.access_token, cfg.secret);

    client.setHeader("Authorization", crypt.mac("GET", "/gifts"));

    cpr::Response response = client.get(url.GIFTS);

    if (response.status_code != 200) {
        std::cerr << "\033[1;31m[ERROR] Failed to fetch gifts." << response.status_code << "\033[0m\n";
        return {};
    }

    nlohmann::json json_data;
    try {
        json_data = nlohmann::json::parse(response.text);
    }
    catch (const std::exception& e) {
        std::cerr << "\033[1;31m[ERROR] An error occurred." << "\033[0m\n";
        return {};
    }

    std::vector<int64_t> gift_ids;
    for (const auto& gift : json_data["gifts"]) {
        gift_ids.push_back(gift["id"].get<int64_t>());
    }

    return gift_ids;
}

void accept_gifts(Config& cfg, ClientSend& client ) {
    std::vector<int64_t> gift_ids = get_gifts(cfg, client);


    if (gift_ids.empty()) {
        std::cout << "\033[1;31mNo gifts to accept.\033[0m\n";
        return;
    }

    ApiUrl url;
    Cryption crypt(cfg.access_token, cfg.secret);

    size_t chunk_size = 25;
    for (size_t i = 0; i < gift_ids.size(); i += chunk_size) {
        std::vector<int64_t>::iterator start = gift_ids.begin() + i;
        std::vector<int64_t>::iterator end = gift_ids.begin() + min(i + chunk_size, gift_ids.size());
        std::vector<int64_t> chunk(start, end);

        nlohmann::json data = { {"gift_ids", chunk} };

        client.setHeader("Authorization", crypt.mac("POST", "/gifts/accept"));

        cpr::Response response = client.post(url.GIFTS_ACCEPT, cpr::Body{ data.dump() });

        if (response.status_code == 200) {
            std::cout << "\033[1;32m[+] Gifts accepted!\033[0m\n";
        }
        else {
            std::cerr << "\033[1;31m[ERROR] Failed to accept gifts." << "\033[0m\n";
        }
    }
}

void refill_stamina_stone(Config& cfg, ClientSend& client ) {

    ApiUrl url;
    nlohmann::json user = get_user(cfg, client, false);
    int stones = user["stone"].get<int>();

    if (stones < 1) {
        std::cerr << "\033[1;31m[ERROR] No stones left to refill stamina.\033[0m\n";
        return;
    }

    Cryption crypt(cfg.access_token, cfg.secret);
    client.setHeader("Authorization", crypt.mac("PUT", "/user/recover_act_with_stone"));

    cpr::Response response = client.put(url.STAMINA_STONE, cpr::Body{ "" });
    if (response.status_code == 200) {
        std::cout << "\033[1;32m[+] Stamina restored with dragon stone!\033[0m\n";
    }
    else {
        std::cerr << "\033[1;31m[ERROR] " << response.text << "\033[0m\n";
    }
}

void refill_stamina_meat(Config& cfg, ClientSend& client){

    ApiUrl url;
    Cryption crypt(cfg.access_token, cfg.secret);

    std::vector<nlohmann::json> data_list = {
        {{"act_item_id", 1}, {"quantity", 1}},
        {{"act_item_id", 2}, {"quantity", 1}},
        {{"act_item_id", 3}, {"quantity", 1}}
    };

    
    bool success = false;

    for (const auto& data : data_list) {
        client.setHeader("Authorization", crypt.mac("PUT", "/user/recover_act_with_items"));
        cpr::Response response = client.put(url.STAMINA_MEAT, cpr::Body{ data.dump() });

        if (response.status_code == 200) {
            std::cout << "\033[1;32m[+] Stamina recovered with items!\033[0m\n";
            success = true;
            break;
        }
    }

    if (!success) {
        std::cout << "\033[1;33m[ERROR] No items for stamina, restoring with stone...\033[0m\n";
        refill_stamina_stone(cfg,client);
    }
    else {
        refill_stamina_meat(cfg, client);
    }
}


void sell_cards(Config& cfg, ClientSend& client, const std::vector<int64_t>& card_list) {
    std::vector<int64_t> cards_to_sell;
    ApiUrl url;
    Cryption crypt(cfg.access_token, cfg.secret);



    for (size_t i = 0; i < card_list.size(); ++i) {
        cards_to_sell.push_back(card_list[i]);

        if ((i + 1) % 99 == 0) {
            nlohmann::json data = { {"card_ids", cards_to_sell} };
            client.setHeader("Authorization", crypt.mac("POST", "/cards/sell"));
            cpr::Response r = client.post(url.SELL_CARDS, cpr::Body{ data.dump() });

            std::cout << "\033[1;37mSold x" << cards_to_sell.size() << " cards\033[0m\n";

            try {
                nlohmann::json json_data = nlohmann::json::parse(r.text);
                if (json_data.contains("error")) {
                    std::cerr << "\033[1;31m[ERROR] " << json_data["error"] << "\033[0m\n";
                    return;
                }
            }
            catch (...) {
                std::cerr << "\033[1;31m[ERROR] Failed to parse response JSON\033[0m\n";
                return;
            }

            cards_to_sell.clear();
        }
    }

    if (!cards_to_sell.empty()) {
        nlohmann::json data = { {"card_ids", cards_to_sell} };

        client.setHeader("Authorization", crypt.mac("POST", "/cards/sell"));

        cpr::Response r = client.post(url.SELL_CARDS, cpr::Body{ data.dump() });
        std::cout << "\033[1;37mSold x" << cards_to_sell.size() << " cards\033[0m\n";


        try {
            nlohmann::json json_data = nlohmann::json::parse(r.text);
            if (json_data.contains("error")) {
                std::cerr << "\033[1;31m[ERROR] " << json_data["error"] << "\033[0m\n";
            }
        }
        catch (...) {
            std::cerr << "\033[1;31m[ERROR] Failed to parse response JSON\033[0m\n";
        }
    }
}

std::vector<int> events(Config& cfg, ClientSend& client) {
    ApiUrl url;
    Cryption crypt(cfg.access_token, cfg.secret);

    std::vector<int> event_ids;

    client.setHeader("Authorization", crypt.mac("GET", "/events"));

    cpr::Response response = client.get(url.EVENTS);
    if (response.status_code != 200) {
        std::cerr << "\033[1;31m[ERROR] Failed to fetch events." << "\033[0m\n";
        return event_ids;
    }

    nlohmann::json json_data;
    try {
        json_data = nlohmann::json::parse(response.text);
    }
    catch (const std::exception& e) {
        std::cerr << "\033[1;31m[ERROR] An error occurred." << "\033[0m\n";
        return event_ids;
    }

    if (!json_data.contains("events") || !json_data["events"].is_array()) {
        std::cerr << "\033[1;31m[ERROR] An error occurred.\033[0m\n";
        return event_ids;
    }

    for (const auto& event : json_data["events"]) {
        if (event.contains("id")) {
            event_ids.push_back(event["id"].get<int>());
        }
    }

    std::sort(event_ids.begin(), event_ids.end());

    event_ids.erase(std::remove(event_ids.begin(), event_ids.end(), 135), event_ids.end());

    return event_ids;
}


nlohmann::json get_friend(Config& cfg, ClientSend& client, const std::string& stage_id, int difficulty, bool zbattle) {
    Cryption crypt(cfg.access_token, cfg.secret);
 

    std::string endpoint;

    if (!zbattle) {
        endpoint = "/quests/" + stage_id + "/briefing?difficulty=" + std::to_string(difficulty) +
            "&force_update=&team_num=" + std::to_string(cfg.deck);
    }
    else {
        endpoint = "/z_battles/" + stage_id + "/briefing?force_update=&team_num=" + std::to_string(cfg.deck);
    }

    client.setHeader("Authorization", crypt.mac("GET", endpoint));

    cpr::Response response = client.get(endpoint);
    if (response.status_code != 200) {
        std::cerr << "\033[1;31m[ERROR] Failed to get friend data." << "\033[0m\n";
        return {};
    }

    nlohmann::json json_data;
    try {
        json_data = nlohmann::json::parse(response.text);
    }
    catch (const std::exception& e) {
        std::cerr << "\033[1;31m[ERROR] An error occurred." << "\033[0m\n";
        return {};
    }

    if (zbattle) return json_data;

    if (json_data.contains("is_cpu_only") && json_data["is_cpu_only"].get<bool>()) {
        if (difficulty >= 0 && difficulty <= 5 && json_data.contains("cpu_supporters")) {
            return {
                {"is_cpu", true},
                {"cpu_friend_id", json_data["cpu_supporters"][0]["id"]},
                {"difficulty", difficulty},
                {"is_playing_script", true},
                {"selected_team_num", cfg.deck},
                {"support_items", nlohmann::json::array()}
            };
        }
    }


    const auto& supporter = json_data["supporters"][0];
    const auto& leader = supporter["leader"];

    return {
        {"is_cpu", false},
        {"friend_id", supporter["id"]},
        {"card_id", leader["card_id"]},
        {"unlocked_square_statuses", leader["unlocked_square_statuses"]},
        {"equipment_skill_items", leader["equipment_skill_items"]},
        {"exp", leader["exp"]},
        {"id", leader["id"]},
        {"is_favorite", leader["is_favorite"]},
        {"is_released_potential", leader["is_released_potential"]},
        {"link_skill_lvs", leader["link_skill_lvs"]},
        {"skill_lv", leader["skill_lv"]},
        {"released_rate", leader["released_rate"]},
        {"potential_parameters", leader["potential_parameters"]}
    };
}


nlohmann::json get_events(Config& cfg, ClientSend& client, bool view) {
    ApiUrl url;
    
    Cryption crypt(cfg.access_token, cfg.secret);

    client.setHeader("Authorization", crypt.mac("GET", "/events"));
    cpr::Response response = client.get(url.EVENTS);

    if (response.status_code != 200) {
        std::cerr << "\033[1;31m[ERROR] Failed to fetch events." << "\033[0m\n";
        return {};
    }

    nlohmann::json json_data;
    try {
        json_data = nlohmann::json::parse(response.text);
    }
    catch (const std::exception& e) {
        std::cerr << "\033[1;31m[ERROR] An error occurred." << "\033[0m\n";
        return {};
    }

    if (view) {
        std::vector<nlohmann::json> areas;
        auto events = json_data["events"];

        for (const auto& event : events) {
            std::string event_id = std::to_string(event["id"].get<int>());
            std::string area_id = std::to_string(event["area_id"].get<int>());

            std::string category = "Unknown";

            // Chercher dans la DB
            auto all_areas = db::get_areas();
            for (const auto& area : all_areas) {
                if (area.at("id") == area_id) {
                    category = area.at("category");
                    break;
                }
            }

            nlohmann::json item = {
                {"id", event["id"]},
                {"image_url", event["banner_image"]},
                {"category", category}
            };
            areas.push_back(item);
        }

        return areas;
    }

    return json_data;
}

nlohmann::json user_areas(Config& cfg, ClientSend& client) {
    ApiUrl url;

    Cryption crypt(cfg.access_token, cfg.secret);

    client.setHeader("Authorization", crypt.mac("GET", "/user_areas"));

    cpr::Response response = client.get(url.AREA);
    if (response.status_code != 200) {
        std::cerr << "\033[1;31m[ERROR] Failed to fetch user areas." << "\033[0m\n";
        return {};
    }

    try {
        return nlohmann::json::parse(response.text);
    }
    catch (const std::exception& e) {
        std::cerr << "\033[1;31m[ERROR] An error occurred." << "\033[0m\n";
        return {};
    }
}

std::vector<nlohmann::json> get_user_areas(Config& cfg, ClientSend& client) {
    ApiUrl url;
    std::vector<nlohmann::json> maps;


    Cryption crypt(cfg.access_token, cfg.secret);
    client.setHeader("Authorization", crypt.mac("GET", "/user_areas"));
    
    cpr::Response response = client.get(url.AREA);
    if (response.status_code != 200) {
        std::cerr << "\033[1;31m[ERROR] Failed to fetch user areas." <<  "\033[0m\n";
        return maps;
    }

    nlohmann::json json_data;
    try {
        json_data = nlohmann::json::parse(response.text);
    }
    catch (const std::exception& e) {
        std::cerr << "\033[1;31m[ERROR] An error occurred." << "\033[0m\n";
        return maps;
    }

    for (const auto& user_area : json_data["user_areas"]) {
        for (const auto& map : user_area["user_sugoroku_maps"]) {
            int id = map["sugoroku_map_id"];
            if (map["cleared_count"] == 0 && id > 100 && id < 999999) {
                maps.push_back(map);
            }
        }
    }

    return maps;
}

std::vector<nlohmann::json> get_clash(Config& cfg, ClientSend& client) {
    std::cout << "Fetching current clash please wait..." << std::endl;
    ApiUrl url;
    Cryption crypt(cfg.access_token, cfg.secret);
    
    client.setHeader("Authorization", crypt.mac("GET", "/rmbattles"));

    cpr::Response response = client.get(url.CLASH);

    if (response.status_code != 200) {
        std::cerr << "\033[1;31m[ERROR] Failed to fetch Clash data.\033[0m\n";
        return {};
    }

    nlohmann::json json_data;
    try {
        json_data = nlohmann::json::parse(response.text);
    }
    catch (const std::exception& e) {
        std::cerr << "\033[1;31m[ERROR] An error occurred." << "\033[0m\n";
        return {};
    }

    return json_data;
}

void change_team(Config& cfg, ClientSend& client,const std::vector<int64_t>& chosen_card_ids) {
    
    ApiUrl url;
    Cryption crypt(cfg.access_token, cfg.secret);

    nlohmann::json team_data = {
        {"selected_team_num", 1},
        {"user_card_teams", {
            {
                {"num", cfg.deck},
                {"user_card_ids", chosen_card_ids}
            }
        }}
    };

    client.setHeader("Authorization", crypt.mac("POST", "/teams"));

    cpr::Response res = client.post(url.TEAM, team_data.dump());

    try {
        nlohmann::json json_response = nlohmann::json::parse(res.text);
        if (json_response.contains("error")) {
            std::cerr << "\033[1;31m[ERROR] " << json_response["error"] << "\033[0m\n";
        }
        else {
            std::cout << "\033[1;32mDeck updated successfully!\033[0m\n";
        }
    }
    catch (const std::exception& e) {
        std::cerr << "\033[1;31m[ERROR] An error occurred." << "\033[0m\n";
    }
}

nlohmann::json list_cards(Config& cfg, ClientSend& client, bool autosell) {
    ApiUrl url;
    Cryption crypt(cfg.access_token, cfg.secret);

    try {
        client.setHeader("Authorization", crypt.mac("GET", "/cards"));
        cpr::Response res = client.get(url.CARDS);

        if (res.status_code != 200) {
            std::cerr << "\033[1;31m[ERROR] Failed to fetch cards: " << res.text << "\033[0m\n";
            return {};
        }

        nlohmann::json response = nlohmann::json::parse(res.text);
        if (!response.contains("cards") || !response["cards"].is_array()) {
            std::cerr << "\033[1;31m[ERROR] Malformed response: missing 'cards'\033[0m\n";
            return {};
        }

        const auto& server_cards = response["cards"];
        const auto& all_cards = db::get_cards_cached();

        // === AUTOSELL MODE ===
        if (autosell) {
            nlohmann::json r_cards_ids = nlohmann::json::array();

            for (const auto& card : server_cards) {
                std::string card_id = std::to_string(card["card_id"].get<int>());
                std::string unique_id = std::to_string(card["id"].get<int64_t>());

                const std::map<std::string, std::string>* db_card_ptr = nullptr;
                for (const auto& row : all_cards) {
                    if (row.at("id") == card_id) {
                        db_card_ptr = &row;
                        break;
                    }
                }
                if (!db_card_ptr) continue;

                int rarity_int = std::stoi(db_card_ptr->at("rarity"));
                if (rarity_int == 0 || rarity_int == 1) {  // N or R
                    r_cards_ids.push_back(unique_id);
                }
            }

            return { {"r_cards", r_cards_ids} };
        }

        std::map<std::string, int> rarity_order = { {"LR", 1}, {"UR", 2}, {"SSR", 3}, {"SR", 4}, {"R", 5}, {"N", 6} };
        std::map<std::string, int> type_order = { {"[AGL]", 1}, {"[TEQ]", 2}, {"[INT]", 3}, {"[STR]", 4}, {"[PHY]", 5} };

        std::vector<Carddatainfo> normal_cards;
        std::vector<Carddatainfo> awakened_cards;
        normal_cards.reserve(3000);
        awakened_cards.reserve(3000);

        for (const auto& card : server_cards) {
            std::string card_id = std::to_string(card["card_id"].get<int>());
            std::string unique_id = std::to_string(card["id"].get<int64_t>());

            const std::map<std::string, std::string>* db_card_ptr = nullptr;
            for (const auto& row : all_cards) {
                if (row.at("id") == card_id) {
                    db_card_ptr = &row;
                    break;
                }
            }
            if (!db_card_ptr) continue;
            const auto& db_card = *db_card_ptr;

            int rarity_int = std::stoi(db_card.at("rarity"));
            std::string rarity = "Unknown";
            switch (rarity_int) {
            case 0: rarity = "N"; break;
            case 1: rarity = "R"; break;
            case 2: rarity = "SR"; break;
            case 3: rarity = "SSR"; break;
            case 4: rarity = "UR"; break;
            case 5: rarity = "LR"; break;
            }

            std::string type = "Unknown";
            std::string element = db_card.at("element");
            if (!element.empty()) {
                switch (element.back()) {
                case '0': type = "[AGL]"; break;
                case '1': type = "[TEQ]"; break;
                case '2': type = "[INT]"; break;
                case '3': type = "[STR]"; break;
                case '4': type = "[PHY]"; break;
                }
            }

            std::string base_img = "https://raw.githubusercontent.com/daye10/smile/master/assets/characters/thumb_merged/merged_";
            std::string image_url = base_img + card_id + ".png";
            bool awakened = false;

            if (rarity_int >= 3 && rarity_int <= 5 && !card_id.empty() && card_id.back() == '1') {
                std::string mod_id = card_id.substr(0, card_id.size() - 1) + "0";
                image_url = base_img + mod_id + "_awakened.png";
                awakened = true;
            }

            Carddatainfo c{ card_id, db_card.at("name"), rarity, type, image_url, unique_id, awakened };
            if (awakened) awakened_cards.push_back(std::move(c));
            else normal_cards.push_back(std::move(c));
        }

        auto sorter = [&](std::vector<Carddatainfo>& list) {
            std::sort(list.begin(), list.end(), [&](const Carddatainfo& a, const Carddatainfo& b) {
                int ra = rarity_order[a.rarity];
                int rb = rarity_order[b.rarity];
                if (ra != rb) return ra < rb;
                return type_order[a.type] < type_order[b.type];
                });
            };

        sorter(normal_cards);
        sorter(awakened_cards);

        auto to_json_list = [](const std::vector<Carddatainfo>& cards) {
            nlohmann::json j = nlohmann::json::array();
            for (const auto& c : cards) {
                j.push_back({
                    {"ID", c.id},
                    {"Name", c.name},
                    {"Rarity", c.rarity},
                    {"Type", c.type},
                    {"ImageURL", c.image_url}
                    });
            }
            return j;
            };

        return {
            {"cards", to_json_list(normal_cards)},
            {"cards2", to_json_list(awakened_cards)}
        };
    }
    catch (const std::exception& e) {
        std::cerr << "\033[1;31m[ERROR] An error occurred." << "\n";
        return {};
    }
}


