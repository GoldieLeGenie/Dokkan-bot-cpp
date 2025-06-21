#ifndef USER_HPP
#define USER_HPP

#include <nlohmann/json.hpp>
#include "config.hpp"  
#include "ClientSend.hpp"


nlohmann::json get_user(Config& cfg, ClientSend& client, bool data);
std::vector<int64_t> get_gifts(Config& cfg, ClientSend& client);
void accept_gifts(Config& cfg, ClientSend& client);
void refill_stamina_meat(Config& cfg, ClientSend& client);
void refill_stamina_stone(Config& cfg, ClientSend& client);
void sell_cards(Config& cfg, ClientSend& client, const std::vector<int64_t>& card_list);
nlohmann::json get_events(Config& cfg, ClientSend& client, bool view);
nlohmann::json get_friend(Config& cfg, ClientSend& client, const std::string& stage_id, int difficulty, bool zbattle);
nlohmann::json user_areas(Config& cfg, ClientSend& client);
std::vector<nlohmann::json> get_user_areas(Config& cfg, ClientSend& client);
std::vector<nlohmann::json> get_clash(Config& cfg, ClientSend& client);
void change_team(Config& cfg, ClientSend& client, const std::vector<int64_t>& chosen_card_ids);
std::vector<int> events(Config& cfg, ClientSend& client);
nlohmann::json list_cards(Config& cfg, ClientSend& client, bool autosell);

#endif // USER_HPP