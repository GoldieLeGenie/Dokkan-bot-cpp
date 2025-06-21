#include "config.hpp"
#include <iostream>
#include <nlohmann/json.hpp>
#include <cpr/cpr.h>
#include "apiPath.hpp"
#include "ClientSend.hpp"
#include "auth.hpp"
#include "cryption.hpp"
#include <cstdlib>
#include <iomanip> 

void signup(Config& cfg, ClientSend& client) {
    ApiUrl url;

    // Définir les infos de l'appareil selon la plateforme
    if (cfg.platform == "android") {
        cfg.device_name = "Asus";
        cfg.device_model = "ASUS_Z01QD";
        cfg.os_version = "7.1.2";
    }
    else {
        cfg.device_name = "iPhone";
        cfg.device_model = "iPhone XR";
        cfg.os_version = "16.0";
    }

    // Génération des identifiants uniques
    auto ids = Cryption::guid();
    cfg.AdId = ids["AdId"];
    cfg.UniqueId = ids["UniqueId"];

    // Construction du JSON de création de compte
    nlohmann::json user_acc = {
        {"user_account", {
            {"ad_id",        cfg.AdId},
            {"country",      "AU"},
            {"currency",     "AUD"},
            {"device",       cfg.device_name},
            {"device_model", cfg.device_model},
            {"os_version",   cfg.os_version},
            {"platform",     cfg.platform},
            {"unique_id",    cfg.UniqueId}
        }}
    };

    // Headers par défaut
    client.setDefaultHeaders({
        {"User-Agent",      cfg.user_agent},
        {"Accept",          "*/*"},
        {"Content-Type",    "application/json"},
        {"X-Platform",      cfg.platform},
        {"X-ClientVersion", cfg.APK_HASH}
        });

    // Première requête pour recevoir le captcha
    cpr::Response response = client.post(url.SIGNUP, cpr::Body{ user_acc.dump() });

    nlohmann::json json_data;
    try {
        json_data = nlohmann::json::parse(response.text);

        if (!json_data.contains("captcha_url") || !json_data.contains("captcha_session_key")) {
            std::cerr << "\033[1;31m[ERROR] Captcha could not be loaded...\033[0m\n";
            return;
        }

        std::string captchaUrl = json_data["captcha_url"];
        std::string captcha_session_key = json_data["captcha_session_key"];

        // Ouvre le navigateur selon l'OS
        std::string command;
    #ifdef _WIN32
            command = "start \"\" \"" + captchaUrl + "\"";
    #elif __APPLE__
            command = "open \"" + captchaUrl + "\"";
    #else
            command = "xdg-open \"" + captchaUrl + "\"";
    #endif

        int result = std::system(command.c_str());
        if (result != 0) {
            std::cerr << "[ERROR] Failed to open browser.\n";
        }

        std::cout << "\033[1;37mOpening captcha in browser. Press ENTER once you have solved it...\033[0m\n";
        std::cin.ignore();

        // Envoie des données avec le captcha résolu
        nlohmann::json data = {
            {"captcha_session_key", captcha_session_key},
            {"user_account", user_acc["user_account"]}
        };

        response = client.post(url.SIGNUP, cpr::Body{ data.dump() });

        auto json_result = nlohmann::json::parse(response.text);

        if (!json_result.contains("identifier")) {
            std::cerr << "\033[1;31m[ERROR] Signup failed. No identifier returned.\033[0m\n";
            return;
        }
        cfg.identifier = Cryption::base64_decode(json_result["identifier"]);
    }
    catch (const nlohmann::json::parse_error& e) {
        std::cerr << "[ERROR] An error occurred." << "\n";
        return;
    }
}



void tutorial(Config& cfg, ClientSend& client) {
    ApiUrl url;
    Cryption crypt(cfg.access_token, cfg.secret);

    nlohmann::json progress = { {"progress", "60101"} };

    // Étape 1 : set headers globaux + 1er POST
    client.setDefaultHeaders({
        {"User-Agent",      cfg.user_agent},
        {"Accept",          "*/*"},
        {"Content-Type",    "application/json"},
        {"Authorization",   crypt.mac("POST", "/tutorial/gasha")},
        {"X-Platform",      cfg.platform},
        {"X-ClientVersion", cfg.APK_HASH},
        {"X-AssetVersion",  "////"},
        {"X-DatabaseVersion", "////"}
        });

    cpr::Response response = client.post(url.TUTORIAL_GASHA, cpr::Body{ progress.dump() });
    std::cout << "\033[1;34mTutorial Progress: 1/6\033[0m" << std::endl;

    // Étape 2 : PUT /tutorial
    progress = { {"progress", "999"} };

    client.setHeader("Authorization", crypt.mac("PUT", "/tutorial"));
    response = client.put(url.TUTORIAL, cpr::Body{ progress.dump() });
    std::cout << "\033[1;34mTutorial Progress: 2/6\033[0m" << std::endl;

    // Étape 3 : PUT /tutorial/finish
    client.setHeader("Authorization", crypt.mac("PUT", "/tutorial/finish"));
    response = client.put(url.TUTORIAL_FINISH, cpr::Body{ "" });
    std::cout << "\033[1;34mTutorial Progress: 3/6\033[0m" << std::endl;

    // Étape 4 : PUT /user (nom)
    nlohmann::json user = { {"user", {{"name", "Goku"}}} };
    client.setHeader("Authorization", crypt.mac("PUT", "/user"));
    response = client.put(url.NAME, cpr::Body{ user.dump() });
    std::cout << "\033[1;34mTutorial Progress: 4/6\033[0m" << std::endl;

    // Étape 5 : POST /missions/put_forward
    client.setHeader("Authorization", crypt.mac("POST", "/missions/put_forward"));
    response = client.post(url.TUTORIAL_PUT_FOWARD, cpr::Body{ "" });
    std::cout << "\033[1;34mTutorial Progress: 5/6\033[0m" << std::endl;

    // Étape 6 : PUT /apologies/accept
    client.setHeader("Authorization", crypt.mac("PUT", "/apologies/accept"));
    response = client.put(url.TUTORIAL_APOLOGIES, cpr::Body{ "" });
    std::cout << "\033[1;34mTutorial Progress: 6/6\033[0m" << std::endl;

    // Finalisation : PUT /user (is_ondemand)
    nlohmann::json data = { {"user", {{"is_ondemand", true}}} };
    client.setHeader("Authorization", crypt.mac("PUT", "/user"));
    response = client.put(url.NAME, cpr::Body{ data.dump() });

    std::cout << "\033[1;32mTUTORIAL COMPLETE!\033[0m" << std::endl;
    return;
}

void signin(Config& cfg, ClientSend& client, const std::string& identifier, bool reroll) {
    ApiUrl url;
    Cryption crypt(cfg.access_token, cfg.secret);

    std::string basic_accpw = crypt.basic(identifier);

    nlohmann::json data;
    if (reroll) {
        auto ids = Cryption::guid(); 
        cfg.AdId = ids["AdId"];
        cfg.UniqueId = ids["UniqueId"];
        data = {
            {"ad_id", cfg.AdId},
            {"unique_id", cfg.UniqueId}
        };
    }
    else {
        data = {
            {"ad_id", cfg.AdId},
            {"unique_id", cfg.UniqueId}
        };
    }

    client.setDefaultHeaders({
        {"User-Agent",      cfg.user_agent},
        {"Accept",          "*/*"},
        {"Content-Type",    "application/json"},
        {"Authorization",   "Basic " + basic_accpw},
        {"X-Language",      "en"},
        {"X-UserCountry",   "US"},
        {"X-UserCurrency",  "USD"},
        {"X-Platform",      cfg.platform},
        {"X-ClientVersion", cfg.APK_HASH}
        });

    cpr::Response response = client.post(url.SIGNIN, cpr::Body{ data.dump() });

    nlohmann::json json_data;
    try {
        json_data = nlohmann::json::parse(response.text);
    }
    catch (const std::exception& e) {
        std::cerr << "\033[1;31m[ERROR] An error occurred." << "\033[0m\n";
        return;
    }

    if (json_data.contains("access_token") && json_data.contains("secret")) {
        cfg.access_token = json_data["access_token"];
        cfg.secret = json_data["secret"];

        Cryption new_crypt(cfg.access_token, cfg.secret);

        client.setDefaultHeaders({
            {"User-Agent",        cfg.user_agent},
            {"Accept",            "*/*"},
            {"Content-Type",      "application/json"},
            {"Authorization",     new_crypt.mac("POST", "/tutorial/gasha")},
            {"X-Platform",        cfg.platform},
            {"X-ClientVersion",   cfg.APK_HASH},
            {"X-Language",        "en"},
            {"X-UserCountry",     "US"},
            {"X-UserCurrency",    "USD"},
            {"X-AssetVersion",    "////"},
            {"X-DatabaseVersion", "////"}
            });

        std::cout << "\033[1;32mSIGN IN COMPLETE\033[0m" << std::endl;
    }
    else {
        std::cerr << "\033[1;31m[ERROR] Signin failed — missing token/secret.\033[0m\n";
    }
}



