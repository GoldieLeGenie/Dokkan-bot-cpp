#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>

struct Config {
    std::string username = "";
    std::string platform = "ios";
    std::string AdId = "";
    std::string UniqueId = "";
    std::string identifier = "";
    std::string access_token = "";
    std::string device_name = "";
    std::string device_model = "";
    std::string os_version = "";
    std::string secret = "";
    std::string aeskey = "iyMgxvi240Smc5oPikQugi0luUp8aQjcxMPO27n7CyPIIDZbfQbEgpWCYNHSTHB";
    int deck = 1;
    bool discord = false;
    std::string discord_id = "";
    bool premium = false;
    bool stamina_refill = true;
    std::string user_agent = "BNGI0221/23052616 CFNetwork/1209 Darwin/20.2.0";
    std::string APK_HASH = "5.28.0-e9206c5811687ff91884893a794411cb233bf19ebcabdefed10877e8ded3a5ce";
    std::string DATABASE_VER = "1748501573";
};


#endif // CONFIG_HPP
