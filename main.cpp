#include <iostream>
#include "auth.hpp"
#include "config.hpp"
#include "apiPath.hpp"
#include "ClientSend.hpp"
#include "user.hpp"
#include "database.hpp"
#include "farm.hpp"
#include "utils.hpp"
#include <nlohmann/json.hpp>

int main() {
   
    Config cfg;
    ApiUrl url;
    ClientSend client(url.HOSTURL);
    /*signup(cfg, client);
    create_file(cfg.AdId, cfg.UniqueId, cfg.platform, cfg.identifier);*/
    load_save_file(cfg, "da");
    signin(cfg, client, cfg.identifier, false);
    update_database(cfg, client);
    if (!db::connect()) {
        std::cerr << "\033[1;31m[ERROR] Could not connect to the local database.\033[0m\n";
        return 0;
    }

   
    /*nlohmann::json result = get_events(cfg, client, false);*/ 
    /*complete_unfinished_zbattles(cfg, client, false); done*/
    /*complete_all_quests(cfg, client); done */
    //nlohmann::json result = list_cards(cfg, client, true) done;  
    //std::vector<int64_t> card_ids_to_sell; done
    //sell_cards(cfg, client, card_ids_to_sell); done
    //signup(cfg, client); done
    //create_file(cfg.AdId, cfg.UniqueId, cfg.platform, cfg.identifier); done
    //tutorial(cfg, client); done
    //get_user(cfg, client, true); done
    //accept_gifts(cfg, client); done 
 /*   user_areas(cfg, client); done  */ 
    //refill_stamina_meat(cfg, client); done
    //refill_stamina_stone(cfg, client); done
 
    //get_friend(cfg,client, "stage_id", 2, false); done
    //auto results = get_events(cfg,client, true);
    //for (const auto& event : results) {
    //    std::cout << "ID: " << event["id"] << " | Category: " << event["category"] << "\n";
    //}
    return 0;
}