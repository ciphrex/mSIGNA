///////////////////////////////////////////////////////////////////
//
// CoinVault
//
// updates.cpp
//
// Copyright (c) 2014 Eric Lombrozo
//
// All Rights Reserved.

#include "updates.h"

#include <iostream>
#include <istream>
#include <ostream>
//#include <boost/asio.hpp>

void UrlParser::parse(const std::string& url)
{
    service = "";
    host = "";
    path = "/";

    enum state_t { SERVICE, SLASH1, SLASH2, HOST, PATH };
    state_t state = SERVICE;
    for (auto& c: url)
    {
        switch (state)
        {
        case SERVICE:
            if (c == ':')
            {
                state = SLASH1;
                continue;
            }
            else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
            {
                service += c;
                continue;
            }
            else throw std::runtime_error("Invalid URL.");

        case SLASH1:
            if (c == '/')
            {
                state = SLASH2;
                continue;
            }
            else throw std::runtime_error("Invalid URL.");

        case SLASH2:
            if (c == '/')
            {
                state = HOST;
                continue;
            }
            else throw std::runtime_error("Invalid URL.");

        case HOST:
            if (c == '/')
            {
                state = PATH;
                continue;
            }
            else
            {
                host += c;
                continue;
            }

        case PATH:
            path += c;
        }

        if (state < HOST) throw std::runtime_error("Invalid URL.");
    } 
}

void LatestVersionInfo::download(const std::string& url)
{
/*
    using namespace boost::asio::ip::tcp;

    boost::asio::io_service io_service;

    tcp::resolver resolver(io_service);
    tcp::resolver::query query(url, "https");
    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
    tcp::resolver::iterator end;

    tcp::socket socket(io_service);
    boost::system::error_code error = boost::asio::error::host_not_found;
    while (error && endpoint_iterator != end)
    {
        socket.close();
        socket.connect(*endpoint_iterator++, error);
    }
    if (error) throw boost::system::system_error(error);

    boost::asio::streambuf request;
    std::ostream request_stream(&request);
    request_stream << "GET " 
*/
}

/*
    const std::string&  getVersionString() const { return versionString; }
    uint32_t            getVersionInt() const { return versionInt; }
    uint32_t            getSchema() const { return schema; }
    const std::string&  getWin64Url() const { return win64Url; }
    const std::string&  getOsxUrl() const { return osxUrl; }

private:
    std::string         versionString;
    uint32_t            versionInt;
    uint32_t            schema;
    std::string         win64Url;
    std::string         osxUrl;
};
*/
