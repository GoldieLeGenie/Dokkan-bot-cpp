#ifndef CLIENTSEND_HPP
#define CLIENTSEND_HPP

#include <cpr/cpr.h>
#include <string>

class ClientSend {
public:
    ClientSend(const std::string& base_url);

    void setDefaultHeaders(const cpr::Header& headers);
    void setHeader(const std::string& key, const std::string& value);

    cpr::Response get(const std::string& endpoint);
    cpr::Response post(const std::string& endpoint, const cpr::Body& body);
    cpr::Response put(const std::string& endpoint, const cpr::Body& body);
    cpr::Response del(const std::string& endpoint);

private:
    std::string base_url;
    cpr::Header current_headers;
};

#endif // CLIENTSEND_HPP
