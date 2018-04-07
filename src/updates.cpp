///////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// updates.cpp
//
// Copyright (c) 2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "updates.h"

#include <iostream>
#include <istream>
#include <ostream>
#include <sstream>
#include <iostream>
#include <boost/asio.hpp>
//#include <boost/asio/ssl.hpp>

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
    UrlParser urlParser(url);

    using boost::asio::ip::tcp;

    //boost::asio::ssl::context ctx(boost::asio::ssl::context::sslv23);
    
    boost::asio::io_service io_service;

    tcp::resolver resolver(io_service);
    tcp::resolver::query query(urlParser.getHost(), urlParser.getService());
    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
    tcp::resolver::iterator end;

    //boost::asio::ssl::stream<boost::asio::ip::tcp::socket> socket(io_service, ctx);
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
    request_stream << "GET " << urlParser.getPath() << " HTTP/1.1\r\n";
    request_stream << "Host: " << urlParser.getHost() << "\r\n";
    request_stream << "Accept: */*\r\n";
    request_stream << "Connection: close\r\n\r\n";

    boost::asio::write(socket, request);

    boost::asio::streambuf response;
/*
    boost::asio::read_until(socket, response, "\r\n");
    std::cout << &response << std::endl;

    std::istream response_stream(&response);
    std::string http_version;
    response_stream >> http_version;
    unsigned int status_code;
    response_stream >> status_code;
    std::string status_message;
    std::getline(response_stream, status_message);
    if (!response_stream || http_version.substr(0, 5) != "HTTP/") throw std::runtime_error("Invalid response.");
    if (status_code != 200)
    {
        std::stringstream err;
        err << "Response return with status code " << status_code << ".";
        throw std::runtime_error(err.str());
    }
*/
    //boost::asio::read_until(socket, response, "\r\n\r\n");

    // Ignore headers

    std::stringstream content;
    while (boost::asio::read(socket, response, boost::asio::transfer_at_least(1), error))
    {
        content << &response;
    }
    if (error != boost::asio::error::eof) throw boost::system::system_error(error);

    std::cout << content.str();
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
