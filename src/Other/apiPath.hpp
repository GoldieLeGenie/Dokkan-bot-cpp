#ifndef APIPATH_HPP
#define APIPATH_HPP

#include <string>

struct ApiUrl {
	std::string HOSTURL = "https://ishin-global.aktsk.com";
	std::string PING = "/ping";
	std::string SIGNUP = "/auth/sign_up";
	std::string SIGNIN = "/auth/sign_in";
	std::string TUTORIAL_GASHA = "/tutorial/gasha";
	std::string TUTORIAL = "/tutorial";
	std::string TUTORIAL_FINISH = "/tutorial/finish";
	std::string NAME = "/user";
	std::string STAMINA_STONE = "/user/recover_act_with_stone";
	std::string STAMINA_MEAT = "/user/recover_act_with_items";
	std::string TUTORIAL_PUT_FOWARD = "/missions/put_forward";
	std::string TUTORIAL_APOLOGIES = "/apologies/accept";
	std::string DATABASE = "/client_assets/database";
	std::string MISSIONS = "/missions";
	std::string ACCEPT_MISSIONS = "/missions/accept";
	std::string GIFTS_ACCEPT = "/gifts/accept";
	std::string CARDS = "/cards";
	std::string SELL_CARDS = "/cards/sell";
	std::string DAILY_LOG = "/resources/home?apologies=true&banners=true&bonus_schedules=true&budokai=true&comeback_campaigns=true&gifts=true&login_bonuses=true&rmbattles=true";
	std::string LOG_BONUS = "/login_bonuses/accept";
	std::string GIFTS = "/gifts";
	std::string UPDATED_CARDS = "/cards?user_card_updated_at=";
	std::string EVENTS = "/events";
	std::string AREA = "/user_areas";
	std::string CLASH = "/rmbattles";
	std::string TEAM = "/teams";
};

#endif