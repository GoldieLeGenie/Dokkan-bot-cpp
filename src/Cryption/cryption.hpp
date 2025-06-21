#ifndef CRYPTION_HPP
#define CRYPTION_HPP

#include <string>
#include <vector>
#include <map>

class Cryption {
public:
    static constexpr int BLOCK_SIZE = 16;
    std::string access_token;
    std::string secret;

    Cryption(const std::string& token, const std::string& sec);

    static std::map<std::string, std::string> guid();
    static std::string generate_uuid();
    static std::string basic(const std::string& identifier);
    std::string mac(const std::string& method, const std::string& action,
        const std::string& url = "https://ishin-global.aktsk.com",
        const std::string& port = "443");

    std::string encrypt_sign(const std::string& data);
    std::string decrypt_sign(const std::string& sign);
    static std::string base64_decode(const std::string& encoded); // Å© inaccessible

private:
    static std::string pad(const std::string& s);
    static std::string unpad(const std::string& s);
    static void get_key_and_iv(const std::string& password, const unsigned char* salt,
        unsigned char* key, unsigned char* iv);
    static std::string to_hex(const unsigned char* data, int len);
    static std::string base64_encode(const unsigned char* buffer, size_t length);
    static std::vector<unsigned char> base64_decode_binary(const std::string& encoded);
};

#endif // CRYPTION_HPP
