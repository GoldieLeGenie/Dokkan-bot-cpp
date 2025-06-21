#ifndef FARM_HPP
#define FARM_HPP

#include <nlohmann/json.hpp>
#include "config.hpp"  
#include "ClientSend.hpp"

int complete_all_quests(Config& cfg, ClientSend& client);
void refresh_client(Config& cfg, ClientSend& client);
nlohmann::json start_stage(Config& cfg, ClientSend& client, std::string stage_id, int difficulty, const std::string& kagi);
nlohmann::json finish_stage(Config& cfg, ClientSend& client, std::string stage_id, int difficulty, nlohmann::json dec_sign);
void complete_stage(Config& cfg, ClientSend& client, std::string stage_id, int difficulty, const std::string& kagi = "");
void complete_unfinished_zbattles(Config& cfg, ClientSend& client, bool use_kagi = false);


#endif // FARM_HPP











