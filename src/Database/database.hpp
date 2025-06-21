#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <vector>
#include <map>
#include <string>

#include "config.hpp"
#include "ClientSend.hpp"


namespace db {
    // Connexion à la base
    bool connect();
    void close();

    // Réinitialise tous les caches (à appeler après un update)
    void reset_caches();

    // Requête brute (non-cachée)
    std::vector<std::map<std::string, std::string>> query_all(const std::string& table);

    // Accès direct aux tables (non-caché)
    std::vector<std::map<std::string, std::string>> get_cards();
    std::vector<std::map<std::string, std::string>> get_leader_skills();
    std::vector<std::map<std::string, std::string>> get_link_skills();
    std::vector<std::map<std::string, std::string>> get_area_tabs();
    std::vector<std::map<std::string, std::string>> get_card_specials();
    std::vector<std::map<std::string, std::string>> get_specials();
    std::vector<std::map<std::string, std::string>> get_z_battle_stage_views();
    std::vector<std::map<std::string, std::string>> get_card_card_categories();
    std::vector<std::map<std::string, std::string>> get_treasure_items();
    std::vector<std::map<std::string, std::string>> get_support_items();
    std::vector<std::map<std::string, std::string>> get_potential_items();
    std::vector<std::map<std::string, std::string>> get_special_items();
    std::vector<std::map<std::string, std::string>> get_training_items();
    std::vector<std::map<std::string, std::string>> get_support_films();
    std::vector<std::map<std::string, std::string>> get_quests();
    std::vector<std::map<std::string, std::string>> get_rank_statuses();
    std::vector<std::map<std::string, std::string>> get_training_fields();
    std::vector<std::map<std::string, std::string>> get_sugoroku_maps();
    std::vector<std::map<std::string, std::string>> get_areas();
    std::vector<std::map<std::string, std::string>> get_awakening_items();

    // Accès optimisé avec cache (rapide)
    const std::vector<std::map<std::string, std::string>>& get_cards_cached();
    const std::vector<std::map<std::string, std::string>>& get_awakening_items_cached();
    const std::vector<std::map<std::string, std::string>>& get_link_skills_cached();

    
}

// Mise à jour de la base distante
void update_database(Config& cfg, ClientSend& client);

#endif // DATABASE_HPP
