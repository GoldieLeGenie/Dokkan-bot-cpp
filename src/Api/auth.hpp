#ifndef SIGNUP_HPP
#define SIGNUP_HPP

#include "config.hpp"  // ← Obligatoire pour que "Config" soit reconnu
#include "ClientSend.hpp"
#include <nlohmann/json.hpp>

void signup(Config& cfg, ClientSend& client);
void signin(Config& cfg, ClientSend& client, const std::string& identifier, bool reroll);
void tutorial(Config& cfg, ClientSend& client);

#endif // SIGNUP_HPP