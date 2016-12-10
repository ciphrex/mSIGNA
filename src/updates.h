///////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// updates.h
//
// Copyright (c) 2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#pragma once

#include <string>
#include <stdexcept>
#include <boost/asio/error.hpp>

class UrlParser
{
public:
    UrlParser() { }
    UrlParser(const std::string& url) { parse(url); }

    void parse(const std::string& url);

    const std::string& getService() const { return service; }
    const std::string& getHost() const { return host; }
    const std::string& getPath() const { return path; }

private:
    std::string service;
    std::string host;
    std::string path;
};

class LatestVersionInfo
{
public:
    void download(const std::string& url);

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

