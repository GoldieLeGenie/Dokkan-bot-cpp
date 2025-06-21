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
#include "user.hpp"
#include "farm.hpp"
#include <set>
#include <unordered_map>

nlohmann::json start_stage(Config& cfg, ClientSend& client, std::string stage_id, int difficulty, const std::string& kagi) {
    Cryption crypt(cfg.access_token, cfg.secret);
    
    std::string out_stage_name;

    if (!std::all_of(stage_id.begin(), stage_id.end(), ::isdigit)) {
        bool found = false;
        for (const auto& quest : db::get_quests()) {
            auto it = quest.find("name");
            if (it != quest.end() && it->second.find(stage_id) != std::string::npos) {
                stage_id = quest.at("id");
                out_stage_name = quest.at("name");
                found = true;
                break;
            }
        }

        if (!found) {
            std::cerr << "\033[1;31m[ERROR] Could not find stage name in databases.\033[0m\n";
            return "";
        }
    }
    else {
        bool found = false;
        for (const auto& quest : db::get_quests()) {
            if (quest.at("id") == stage_id) {
                out_stage_name = quest.at("name");
                found = true;
                break;
            }
        }

        if (!found) {
            std::cerr << "\033[1;31m[ERROR] Stage ID not found in database.\033[0m\n";
            return "";
        }
    }

    std::cout << "\033[1;36m[STAGE]\033[0m \033[1;32m" << out_stage_name
          << "\033[0m \033[1;37m| ID: \033[0m\033[1;34m" << stage_id
          << "\033[0m \033[1;37m| Difficulty: \033[0m\033[1;33m" << difficulty
          << "\033[0m\n";


    auto friend_data = get_friend(cfg, client, stage_id, difficulty, false);
    nlohmann::json sign;

    if (!friend_data["is_cpu"].get<bool>()) {
        sign = {
            {"difficulty", difficulty},
            {"friend_id", friend_data["friend_id"]},
            {"is_playing_script", true},
            {"selected_team_num", cfg.deck},
            {"support_leader", {
                {"awakening_route_id", nullptr },
                {"card_decoration_id", nullptr},
                {"card_id", friend_data["card_id"]},
                {"equipment_skills", friend_data["equipment_skill_items"]},
                {"exchangeable_item_id", nullptr},
                {"exp", friend_data["exp"]},
                {"id", friend_data["id"]},
                {"is_favorite", friend_data["is_favorite"]},
                {"is_released_potential", friend_data["is_released_potential"]},
                {"link_skill_lvs", friend_data["link_skill_lvs"]},
                {"optimal_awakening_step", nullptr},
                {"potential_parameters", friend_data["potential_parameters"]},
                {"released_rate", friend_data["released_rate"]},
                {"skill_lv", friend_data["skill_lv"]}
            }},
            {"support_user_card_id", friend_data["card_id"]}
        };
    }
    else {
        sign = {
            {"cpu_friend_id", friend_data["cpu_friend_id"]},
            {"difficulty", difficulty},
            {"is_playing_script", true},
            {"selected_team_num", cfg.deck},
            {"support_items", nlohmann::json::array()}
        };
    }

    std::string encrypted = crypt.encrypt_sign(sign.dump());
    nlohmann::json post_body = { {"sign", encrypted} };



    client.setHeader("Authorization", crypt.mac("POST", "/quests/" + stage_id + "/sugoroku_maps/start"));
    cpr::Response response = client.post("/quests/" + stage_id + "/sugoroku_maps/start", cpr::Body{ post_body.dump() });

    auto json = nlohmann::json::parse(response.text);
    if (json.contains("sign")) {
        return nlohmann::json::parse(crypt.decrypt_sign(json["sign"]));
    }

    if (json.contains("error")) {
        std::string code = json["error"]["code"];
        if (code == "act_is_not_enough") {
            refill_stamina_meat(cfg, client);

            client.setHeader("Authorization", crypt.mac("POST", "/quests/" + stage_id + "/sugoroku_maps/start"));
            response = client.post("/quests/" + stage_id + "/sugoroku_maps/start", cpr::Body{ post_body.dump() });

            return nlohmann::json::parse(crypt.decrypt_sign(nlohmann::json::parse(response.text)["sign"]));
        }
        else if (code == "invalid_area_conditions_potential_releasable") {
            std::cerr << "[API-ERROR] Event conditions not met.\n";
        }
        else if (code == "the_number_of_cards_must_be_less_than_or_equal_to_the_capacity") {
            std::cerr << "[API-ERROR] " << json["error"].dump() << "\n";
            nlohmann::json result = list_cards(cfg, client, true);  // autosell = true

            if (result.contains("r_cards") && result["r_cards"].is_array()) {
                std::vector<int64_t> card_ids_to_sell;

                for (const auto& id : result["r_cards"]) {
                    card_ids_to_sell.push_back(std::stoll(id.get<std::string>()));
                }

                if (!card_ids_to_sell.empty()) {
                    sell_cards(cfg, client, card_ids_to_sell);
                }
            }

            client.setHeader("Authorization", crypt.mac("POST", "/quests/" + stage_id + "/sugoroku_maps/start"));
            response = client.post("/quests/" + stage_id + "/sugoroku_maps/start", cpr::Body{ post_body.dump() });

            return nlohmann::json::parse(crypt.decrypt_sign(nlohmann::json::parse(response.text)["sign"]));
        }
        else {
            std::cerr << "[API-ERROR] " << json["error"].dump() << "\n";
           
        }
    }

    return {};
}




nlohmann::json finish_stage(Config& cfg, ClientSend& client, std::string stage_id , int difficulty, nlohmann::json dec_sign) {
    using namespace std::chrono;
    Cryption crypt(cfg.access_token, cfg.secret);

    std::vector<nlohmann::json> steps;
    std::vector<int> defeated;

    if (dec_sign.contains("sugoroku") && dec_sign["sugoroku"].is_object()) {
        const auto& sugoroku = dec_sign["sugoroku"];

        if (sugoroku.contains("events") && sugoroku["events"].is_object()) {
            for (const auto& [key, value] : sugoroku["events"].items()) {
                steps.push_back(key);
                if (value.contains("content") && value["content"].is_object() &&
                    value["content"].contains("battle_info") && value["content"]["battle_info"].is_array()) {

                    for (const auto& battle : value["content"]["battle_info"]) {
                        if (battle.contains("round_id"))
                            defeated.push_back(battle["round_id"]);
                    }
                }
            }
        }
        else {
            std::cerr << "\033[1;31m[ERROR] An error occurred impossible to finish stage.\033[0m\n";
            return {};
        }
    }
    else {
        std::cerr << "\033[1;31m[ERROR] An error occurred impossible to finish stage.\033[0m\n";
        return {};
    }


    int finish_time = static_cast<int>(duration_cast<seconds>(system_clock::now().time_since_epoch()).count()) + 2000;
    int start_time = finish_time - (6200000 + rand() % 2000000);
    int damage = 500000 + rand() % 500000;

    std::string prefix = stage_id.substr(0, 3);
    if (prefix == "711" || prefix == "185" || prefix == "186" || prefix == "187") {
        damage = 100000000 + rand() % 1000000;
    }

    nlohmann::json sign_finish = {
        {"actual_steps", steps},
        {"difficulty", difficulty},
        {"elapsed_time", finish_time - start_time},
        {"energy_ball_counts_in_boss_battle", std::vector<int>{4, 6, 0, 6, 4, 3, 0, 0, 0, 0, 0, 0, 0}},
        {"has_player_been_taken_damage", false},
        {"is_cheat_user", false},
        {"is_cleared", true},
        {"is_defeated_boss", true},
        {"is_player_special_attack_only", true},
        {"max_damage_to_boss", damage},
        {"min_turn_in_boss_battle", static_cast<int>(defeated.size())},
        {"passed_round_ids", defeated},
        {"quest_finished_at_ms", finish_time},
        {"quest_started_at_ms", start_time},
        {"steps", steps},
        {"token", dec_sign["token"]}
    };

    std::string encrypted = crypt.encrypt_sign(sign_finish.dump());
    nlohmann::json signed_body = { {"sign", encrypted} };

    client.setHeader("Authorization", crypt.mac("POST", "/quests/" + stage_id + "/sugoroku_maps/finish"));
    cpr::Response response = client.post("/quests/" + stage_id + "/sugoroku_maps/finish", cpr::Body{ signed_body.dump() });

    if (response.status_code != 200) {
        std::cerr << "\033[1;31m[ERROR] An error occurred impossible to finish stage." << "\033[0m" << response.text << "\n";
        return {};
    }

    nlohmann::json json = nlohmann::json::parse(response.text);
    auto dec_result = nlohmann::json::parse(crypt.decrypt_sign(json["sign"]));

    int stones = 0;
    if (dec_result.contains("quest_clear_rewards")) {

        for (const auto& item : dec_result["quest_clear_rewards"]) {
            if (item["item_type"] == "Point::Stone") {
                stones += item["amount"];
            }
        }
    }

    std::unordered_map<std::string, std::string> awakening_map;
    for (const auto& row : db::get_awakening_items_cached()) {
        awakening_map[row.at("id")] = row.at("name");
    }

    std::unordered_map<std::string, std::string> card_map;
    for (const auto& row : db::get_cards_cached()) {
        card_map[row.at("id")] = row.at("name");
    }

    std::map<std::string, int> items_collected;

    try {
        // Map de nom → quantité
        std::unordered_map<std::string, int> items_collected;
        // Map de nom → couleur ANSI
        std::unordered_map<std::string, std::string> item_colors;

        for (const auto& item : dec_result["items"]) {
            int quantity = item["quantity"];
            std::string item_id = std::to_string(item["item_id"].get<int>());
            std::string name;
            std::string color;

            const std::string& type = item["item_type"];

            if (type == "AwakeningItem" && awakening_map.count(item_id)) {
                name = awakening_map[item_id];
                color = "\033[1;36m"; // Cyan
            }
            else if (type == "Card" && card_map.count(item_id)) {
                name = card_map[item_id];
                color = "\033[1;35m"; // Magenta
            }
            else if (type == "SupportItem") {
                name = "SupportItem";
                color = "\033[1;33m"; // Jaune
            }
            else if (type == "TrainingItem") {
                name = "TrainingItem";
                color = "\033[1;32m"; // Vert
            }
            else if (type == "SupportFilm") {
                name = "SupportFilm";
                color = "\033[1;34m"; // Bleu
            }
            else {
                name = type;
                color = "\033[1;37m"; // Blanc
            }

            items_collected[name] += quantity;
            item_colors[name] = color;
        }

        if (!items_collected.empty()) {
            std::cout << "\033[1;35m[Items Collected]\033[0m\n";
            for (const auto& [name, count] : items_collected) {
                std::string color = item_colors.count(name) ? item_colors[name] : "\033[0m";
                std::cout << color << "  - " << name << " x" << count << "\033[0m\n";
            }
        }

        if (stones > 0)
            std::cout << "\033[1;33mDragon Stones: x" << stones << "\033[0m\n";

        std::cout << "\033[1;34mZeni Earned: " << dec_result["zeni"].get<int>() << "\033[0m\n";
        std::cout << "\033[1;32m=====================================\033[0m\n\n";
    }
    catch (...) {
        std::cerr << "\033[1;31m[ERROR] Trouble parsing items.\033[0m\n";
    }
}




int complete_all_quests(Config& cfg, ClientSend& client) {
    auto all_data = user_areas(cfg, client);

    std::vector<nlohmann::json> maps;
    if (all_data.contains("user_areas") && all_data["user_areas"].is_array()) {
        for (const auto& area : all_data["user_areas"]) {
            if (area.contains("user_sugoroku_maps") && area["user_sugoroku_maps"].is_array()) {
                for (const auto& map : area["user_sugoroku_maps"]) {
                    if (map.contains("cleared_count") && map["cleared_count"] == 0 &&
                        map.contains("sugoroku_map_id") && map["sugoroku_map_id"].is_number_integer()) {

                        int map_id = map["sugoroku_map_id"];
                        if (map_id > 100 && map_id < 999999) {
                            maps.push_back(map);
                        }
                    }
                }
            }
        }
    }

    if (maps.empty()) {
        std::cout << "\033[1;31mNo quests to complete.\033[0m\n";
        return 0;
    }

    while (true) {
        std::set<std::string> done_quests;

        for (const auto& map : maps) {
            std::string map_id_str = std::to_string(map["sugoroku_map_id"].get<int>());
            if (map_id_str.length() < 2) continue;

            std::string quest_id = map_id_str.substr(0, map_id_str.size() - 1);
            if (done_quests.count(quest_id)) continue;
            done_quests.insert(quest_id);

            int max_difficulty = -1;
            for (const auto& smap : db::get_sugoroku_maps()) {
                if (smap.at("quest_id") == quest_id) {
                    int diff = std::stoi(smap.at("difficulty"));
                    if (diff > max_difficulty) {
                        max_difficulty = diff;
                    }
                }
            }

            if (max_difficulty >= 0) {
                complete_stage(cfg, client, quest_id, max_difficulty);
            }
        }

        std::vector<nlohmann::json> new_maps;
        auto new_data = user_areas(cfg, client);

        if (new_data.contains("user_areas") && new_data["user_areas"].is_array()) {
            for (const auto& area : new_data["user_areas"]) {
                if (area.contains("user_sugoroku_maps") && area["user_sugoroku_maps"].is_array()) {
                    for (const auto& map : area["user_sugoroku_maps"]) {
                        if (map.contains("cleared_count") && map["cleared_count"] == 0 &&
                            map.contains("sugoroku_map_id") && map["sugoroku_map_id"].is_number_integer()) {

                            int map_id = map["sugoroku_map_id"];
                            if (map_id > 100 && map_id < 999999) {
                                new_maps.push_back(map);
                            }
                        }
                    }
                }
            }
        }

        if (new_maps == maps) break;
        maps = new_maps;
        refresh_client(cfg, client);
    }

    return 1;
}


void complete_stage(Config& cfg, ClientSend& client, std::string stage_id, int difficulty, const std::string& kagi) {
    try {
        auto dec_sign_s = start_stage(cfg, client, stage_id, difficulty, kagi);
        auto dec_sign_f = finish_stage(cfg, client, stage_id, difficulty, dec_sign_s);

        if (!dec_sign_f.contains("user_items") || !dec_sign_f["user_items"].is_object()) {
            return;
        }

        if (!dec_sign_f["user_items"].contains("cards") || !dec_sign_f["user_items"]["cards"].is_array()) {
            return;
        }

        const auto& cards = dec_sign_f["user_items"]["cards"];
        if (cards.empty()) return;

        std::vector<int64_t> card_ids_to_sell;
        const auto& all_cards = db::get_cards_cached();

        for (const auto& card : cards) {
            if (!card.contains("card_id") || !card.contains("id")) continue;

            int card_id = card["card_id"];
            std::string id_str = std::to_string(card_id);
            std::string rarity_str = "9"; 

            for (const auto& db_card : all_cards) {
                if (db_card.at("id") == id_str) {
                    rarity_str = db_card.at("rarity");
                    break;
                }
            }

            if (rarity_str == "0") {
                card_ids_to_sell.push_back(card["id"].get<int64_t>());
            }
        }

        if (!card_ids_to_sell.empty()) {
            sell_cards(cfg, client, card_ids_to_sell);
        }
    }
    catch (const std::exception& e) {
        std::cerr << "\033[1;31m[ERROR] Exception in complete_stage:\033[0m " << "\n";
    }
    catch (...) {
        std::cerr << "\033[1;31m[ERROR] Unknown error occurred in complete_stage.\033[0m\n";
    }
}




void complete_unfinished_zbattles(Config& cfg, ClientSend& client, bool use_kagi) {
    nlohmann::json events = get_events(cfg, client, false);
    Cryption crypt(cfg.access_token, cfg.secret);

    try {
        if (!events.contains("z_battle_stages") || !events["z_battle_stages"].is_array()) {
            std::cerr << "\033[1;31m[ERROR] Missing or invalid 'z_battle_stages'.\033[0m\n";
            return;
        }

        for (const auto& event : events["z_battle_stages"]) {
            int event_id = event["id"].get<int>();
            std::string enemy_name = "Unknown";

            for (const auto& row : db::get_z_battle_stage_views()) {
                if (row.at("z_battle_stage_id") == std::to_string(event_id)) {
                    enemy_name = row.at("enemy_name");
                    break;
                }
            }

            nlohmann::json response = user_areas(cfg, client);
            int level = 1;
            if (response.contains("user_z_battles")) {
                for (const auto& zb : response["user_z_battles"]) {
                    if (zb["z_battle_stage_id"].get<int>() == event_id) {
                        level = zb["max_clear_level"].get<int>() + 1;
                        break;
                    }
                }
            }

            while (level < 31) {

                std::cout << "\033[1;36m[ZBATTLE]\033[0m \033[1;32m" << enemy_name
                    << "\033[0m \033[1;37m| ID: \033[0m\033[1;34m" << event_id
                    << "\033[0m \033[1;37m| Level: \033[0m\033[1;33m" << level
                    << "\033[0m\n";
                
                nlohmann::json supporter_data = get_friend(cfg, client, std::to_string(event_id), 0, true);
                if (!supporter_data.contains("supporters") || supporter_data["supporters"].empty()) {
                    std::cerr << "[ERROR] No supporter found for Z-Battle " << event_id << "\n";
                    break;
                }

                auto friend_data = supporter_data["supporters"][0];
                nlohmann::json sign_data = {
                    {"friend_id", friend_data["id"]},
                    {"level", level},
                    {"selected_team_num", cfg.deck},
                    {"support_leader", friend_data["leader"]},
                    {"support_user_card_id", friend_data["leader"]["card_id"]}
                };

                std::string enc_sign = crypt.encrypt_sign(sign_data.dump());
                nlohmann::json signed_body = { {"sign", enc_sign} };

                std::string start_path = "/z_battles/" + std::to_string(event_id) + "/start";
                client.setHeader("Authorization", crypt.mac("POST", start_path));
                cpr::Response start_res = client.post(start_path, signed_body.dump());

                if (start_res.status_code != 200 && cfg.stamina_refill) {
                    auto err = nlohmann::json::parse(start_res.text);
                    if (err.contains("error") && err["error"]["code"] == "act_is_not_enough") {
                        refill_stamina_meat(cfg, client);
                        client.setHeader("Authorization", crypt.mac("POST", start_path));
                        start_res = client.post(start_path, signed_body.dump());
                    }
                }

                auto dec_sign = nlohmann::json::parse(crypt.decrypt_sign(nlohmann::json::parse(start_res.text)["sign"]));
                std::string token = dec_sign["token"];

                int finish_time = time(nullptr) + 2000;
                int start_time = finish_time - (6200000 + rand() % 2000000);

                nlohmann::json finish_sign = {
                    {"elapsed_time", finish_time - start_time},
                    {"is_cleared", true},
                    {"level", level},
                    {"token", token},
                    {"used_items", nlohmann::json::array()},
                    {"z_battle_finished_at_ms", finish_time},
                    {"z_battle_started_at_ms", start_time}
                };

                nlohmann::json finish_body = {
                    {"s", ""},
                    {"sign", crypt.encrypt_sign(finish_sign.dump())},
                    {"t", nullptr},
                    {"total_damage_to_enemy", 12000000 + rand() % 8000000},
                    {"total_damage_to_player", 50 + rand() % 50000},
                    {"used_special_views", nlohmann::json::array()}
                };

                std::string finish_path = "/z_battles/" + std::to_string(event_id) + "/finish";
                client.setHeader("Authorization", crypt.mac("POST", finish_path));
                auto finish_res = client.post(finish_path, finish_body.dump());

                try {
                    auto dec_finish = nlohmann::json::parse(crypt.decrypt_sign(nlohmann::json::parse(finish_res.text)["sign"]));

                    // Maps de référence
                    std::unordered_map<std::string, std::string> awakening_map;
                    for (const auto& row : db::get_awakening_items_cached()) {
                        awakening_map[row.at("id")] = row.at("name");
                    }

                    std::unordered_map<std::string, std::string> card_map;
                    for (const auto& row : db::get_cards_cached()) {
                        card_map[row.at("id")] = row.at("name");
                    }

                    std::unordered_map<std::string, int> items_collected;
                    std::unordered_map<std::string, std::string> item_colors;
                    int stones = 0;

                    for (const auto& item : dec_finish["items"]) {
                        int quantity = item["quantity"];
                        std::string item_id = std::to_string(item["item_id"].get<int>());
                        std::string type = item["item_type"];
                        std::string name, color;

                        if (type == "Point::Stone") {
                            stones += quantity;
                            continue;
                        }
                        else if (type == "AwakeningItem" && awakening_map.count(item_id)) {
                            name = awakening_map[item_id];
                            color = "\033[1;36m";
                        }
                        else if (type == "Card" && card_map.count(item_id)) {
                            name = card_map[item_id];
                            color = "\033[1;35m";
                        }
                        else if (type == "SupportItem") {
                            name = "SupportItem";
                            color = "\033[1;33m";
                        }
                        else if (type == "TrainingItem") {
                            name = "TrainingItem";
                            color = "\033[1;32m";
                        }
                        else if (type == "SupportFilm") {
                            name = "SupportFilm";
                            color = "\033[1;34m";
                        }
                        else {
                            name = type;
                            color = "\033[1;37m";
                        }

                        items_collected[name] += quantity;
                        item_colors[name] = color;
                    }

                    if (!items_collected.empty()) {
                        std::cout << "\033[1;35m[Items Collected]\033[0m\n";
                        for (const auto& [name, count] : items_collected) {
                            std::string color = item_colors.count(name) ? item_colors[name] : "\033[0m";
                            std::cout << color << "  - " << name << " x" << count << "\033[0m\n";
                        }
                    }

                    if (stones > 0)
                        std::cout << "\033[1;33mDragon Stones: x" << stones << "\033[0m\n";

                    std::cout << "\033[1;32m=====================================\033[0m\n\n";
                }
                catch (const std::exception& e) {
                    std::cerr << "\033[1;31m[ERROR] Trouble parsing items: \033[0m" << "\n";
                }

                level++;
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "\033[1;31m[ERROR] " << "\nTrouble processing Z-Battles.\033[0m\n";
    }
}



//void complete_all_events(Config& cfg, ClientSend& client) {
//    nlohmann::json events = get_events(cfg, client, false);
//
//}






void refresh_client(Config& cfg, ClientSend& client) {

    signin(cfg, client, cfg.identifier,false);

    std::cout << "\033[1;32m[+] Refreshed Token\033[0m\n";
    return;
}