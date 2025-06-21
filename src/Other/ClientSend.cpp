#include "ClientSend.hpp"

ClientSend::ClientSend(const std::string& base_url)
    : base_url(base_url) {
}

void ClientSend::setDefaultHeaders(const cpr::Header& headers) {
    current_headers = headers;
}

void ClientSend::setHeader(const std::string& key, const std::string& value) {
    current_headers[key] = value;
}

cpr::Response ClientSend::get(const std::string& endpoint) {
    return cpr::Get(
        cpr::Url{ base_url + endpoint },
        current_headers
    );
}

cpr::Response ClientSend::post(const std::string& endpoint, const cpr::Body& body) {
    return cpr::Post(
        cpr::Url{ base_url + endpoint },
        current_headers,
        body
    );
}

cpr::Response ClientSend::put(const std::string& endpoint, const cpr::Body& body) {
    return cpr::Put(
        cpr::Url{ base_url + endpoint },
        current_headers,
        body
    );
}

cpr::Response ClientSend::del(const std::string& endpoint) {
    return cpr::Delete(
        cpr::Url{ base_url + endpoint },
        current_headers
    );
}
