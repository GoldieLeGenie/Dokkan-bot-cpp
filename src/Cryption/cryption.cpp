#include "cryption.hpp"
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/md5.h>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <sstream>
#include <iomanip>
#include <random>
#include <algorithm>
#include <nlohmann/json.hpp>
#include <random>
#include <iostream>

Cryption::Cryption(const std::string& token, const std::string& sec)
    : access_token(token), secret(sec) {}

std::string Cryption::generate_uuid() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    std::uniform_int_distribution<> dis2(8, 11);

    std::stringstream ss;
    ss << std::hex;
    for (int i = 0; i < 8; i++) ss << dis(gen);
    ss << "-";
    for (int i = 0; i < 4; i++) ss << dis(gen);
    ss << "-4"; // UUID version 4
    for (int i = 0; i < 3; i++) ss << dis(gen);
    ss << "-";
    ss << dis2(gen); // UUID variant
    for (int i = 0; i < 3; i++) ss << dis(gen);
    ss << "-";
    for (int i = 0; i < 12; i++) ss << dis(gen);
    return ss.str();
}

std::map<std::string, std::string> Cryption::guid() {
    std::string adid = generate_uuid();
    std::string u1 = generate_uuid();
    std::string u2 = generate_uuid().substr(0, 8); // début d’un autre UUID
    return {
        {"AdId", adid},
        {"UniqueId", u1 + ":" + u2}
    };
}

std::string Cryption::basic(const std::string& identifier) {
    std::string temp = identifier;
    if (identifier.find(':') == std::string::npos) {
        std::string clean = identifier;
        clean.erase(std::remove(clean.begin(), clean.end(), '\n'), clean.end());
        while (clean.size() < 159) {
            clean += '\x08';
        }
        std::string decoded = base64_decode(clean);
        auto pos = decoded.find(':');
        if (pos != std::string::npos) {
            temp = decoded.substr(pos + 1) + ":" + decoded.substr(0, pos);
        }
    }
    else {
        auto pos = identifier.find(':');
        temp = identifier.substr(pos + 1) + ":" + identifier.substr(0, pos);
    }
    return base64_encode(reinterpret_cast<const unsigned char*>(temp.c_str()), temp.length());
}

std::string Cryption::mac(const std::string& method, const std::string& action, const std::string& url, const std::string& port) {
    // Génère timestamp UNIX
    std::string ts = std::to_string(std::time(nullptr));

    // Calcule MD5(ts)
    unsigned char md5res[MD5_DIGEST_LENGTH];
    MD5(reinterpret_cast<const unsigned char*>(ts.c_str()), ts.size(), md5res);

    // Convertit le résultat MD5 en hexadécimal (comme .hexdigest() en Python)
    std::ostringstream hex_stream;
    for (int i = 0; i < MD5_DIGEST_LENGTH; ++i) {
        hex_stream << std::hex << std::setw(2) << std::setfill('0') << (int)md5res[i];
    }
    std::string md5_hex = hex_stream.str();

    // Construit le nonce
    std::string nonce = ts + ":" + md5_hex;

    // Nettoie l’URL en supprimant "https://"
    std::string clean_url = url;
    const std::string https_prefix = "https://";
    if (clean_url.rfind(https_prefix, 0) == 0) {
        clean_url = clean_url.substr(https_prefix.size());
    }

    // Construit la string value
    std::string value = ts + "\n" + nonce + "\n" + method + "\n" + action + "\n" + clean_url + "\n" + port + "\n\n";

    // Calcule le HMAC-SHA256
    unsigned int len = 0;
    unsigned char* hmac_result = HMAC(EVP_sha256(), secret.c_str(), secret.size(),
        reinterpret_cast<const unsigned char*>(value.c_str()), value.size(),
        nullptr, &len);

    // Encodage base64
    std::string mac = base64_encode(hmac_result, len);

    // Assemble l’en-tête MAC
    std::ostringstream oss;
    oss << "MAC id=\"" << access_token << "\" nonce=\"" << nonce
        << "\" ts=\"" << ts << "\" mac=\"" << mac << "\"";
    return oss.str();
}

std::string Cryption::encrypt_sign(const std::string& data) {
    std::string padded = pad(data);

    std::string full_key_str = "iyMgxvi240Smc5oPikQugi0luUp8aQjcxMPO27n7CyPIIDZbfQbEgpWCYNHSTHB";

    unsigned char salt[8];
    RAND_bytes(salt, sizeof(salt));

    unsigned char key[32], iv[16];
    get_key_and_iv(full_key_str, salt, key, iv); 
    AES_KEY aes_key;
    AES_set_encrypt_key(key, 256, &aes_key);

    std::vector<unsigned char> enc_data(padded.size());
    AES_cbc_encrypt(reinterpret_cast<const unsigned char*>(padded.c_str()), enc_data.data(), padded.size(), &aes_key, iv, AES_ENCRYPT);

    std::vector<unsigned char> full_data(salt, salt + 8);
    full_data.insert(full_data.end(), enc_data.begin(), enc_data.end());

    return base64_encode(full_data.data(), full_data.size());
}


std::string Cryption::decrypt_sign(const std::string& sign) {
    std::vector<unsigned char> buffer = base64_decode_binary(sign);

    std::string full_key_str = "iyMgxvi240Smc5oPikQugi0luUp8aQjcxMPO27n7CyPIIDZbfQbEgpWCYNHSTHB";

    const unsigned char* salt = buffer.data();
    const unsigned char* data = buffer.data() + 8;
    size_t data_len = buffer.size() - 8;

    unsigned char key[32], iv[16];
    get_key_and_iv(full_key_str, salt, key, iv); 

    AES_KEY aes_key;
    AES_set_decrypt_key(key, 256, &aes_key);

    std::vector<unsigned char> dec_data(data_len);
    AES_cbc_encrypt(data, dec_data.data(), data_len, &aes_key, iv, AES_DECRYPT);

    std::string decrypted(reinterpret_cast<char*>(dec_data.data()), dec_data.size());
    decrypted = unpad(decrypted); 
    return decrypted;
}


std::string Cryption::pad(const std::string& s) {
    int padding = BLOCK_SIZE - (s.length() % BLOCK_SIZE);
    return s + std::string(padding, static_cast<char>(padding));
}

std::string Cryption::unpad(const std::string& s) {
    int pad_size = s[s.size() - 1];
    return s.substr(0, s.size() - pad_size);
}

void Cryption::get_key_and_iv(const std::string& password, const unsigned char* salt, unsigned char* key, unsigned char* iv) {
    const EVP_MD* dgst = EVP_md5();
    EVP_BytesToKey(EVP_aes_256_cbc(), dgst, salt, reinterpret_cast<const unsigned char*>(password.c_str()), password.length(), 1, key, iv);
}

std::string Cryption::to_hex(const unsigned char* data, int len) {
    std::ostringstream oss;
    for (int i = 0; i < len; ++i) oss << std::hex << std::setw(2) << std::setfill('0') << (int)data[i];
    return oss.str();
}

std::string Cryption::base64_encode(const unsigned char* buffer, size_t length) {
    BIO* bio, * b64;
    BUF_MEM* bufferPtr;
    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(bio, buffer, length);
    BIO_flush(bio);
    BIO_get_mem_ptr(bio, &bufferPtr);
    std::string result(bufferPtr->data, bufferPtr->length);
    BIO_free_all(bio);
    return result;
}

std::vector<unsigned char> Cryption::base64_decode_binary(const std::string& encoded) {
    BIO* bio, * b64;
    int length = encoded.length();
    std::vector<unsigned char> decoded(length);
    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new_mem_buf(encoded.data(), length);
    bio = BIO_push(b64, bio);
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    int decoded_length = BIO_read(bio, decoded.data(), length);
    decoded.resize(decoded_length);
    BIO_free_all(bio);
    return decoded;
}

std::string Cryption::base64_decode(const std::string& encoded) {
    std::vector<unsigned char> decoded = base64_decode_binary(encoded);
    return std::string(decoded.begin(), decoded.end());
}
