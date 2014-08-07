///////////////////////////////////////////////////////////////////////////////
//
// coindb
//
// config.h
//
// Copyright (c) 2014 Eric Lombrozo
//
// All Rights Reserved.
//

#pragma once

#include <string>
#include <sstream>
#include <fstream>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include <sysutils/filesystem.h>

const std::string DEFAULT_DATA_DIR = "CoinDB";
const std::string DEFAULT_CONFIG_FILE = "coindb.conf";

class CoinDBConfig
{
public:
    CoinDBConfig() : m_bHelp(false) { }

    void init(int argc, char* argv[]);

    const std::string& getConfigFile() const { return m_configFile; }
    const std::string& getDatabaseUser() const { return m_databaseUser; }
    const std::string& getDatabasePassword() const { return m_databasePassword; }
    const std::string& getDataDir() const { return m_dataDir; }

    bool help() const { return m_bHelp; }
    const std::string& getHelpOptions() const { return m_helpOptions; }

private:
    std::string m_configFile;
    std::string m_databaseUser;
    std::string m_databasePassword;
    std::string m_dataDir;

    bool m_bHelp;
    std::string m_helpOptions;
};

inline void CoinDBConfig::init(int argc, char* argv[])
{
    namespace po = boost::program_options;
    namespace fs = boost::filesystem;
    po::options_description options("Options");
    options.add_options()
        ("help", "display help message")
        ("config", po::value<std::string>(&m_configFile), "name of the configuration file")
        ("dbuser", po::value<std::string>(&m_databaseUser), "database user")
        ("dbpasswd", po::value<std::string>(&m_databasePassword), "database password")
        ("datadir", po::value<std::string>(&m_dataDir), "data directory")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, options), vm);
    po::notify(vm);

    if (vm.count("help"))
    {
        m_bHelp = true;
        std::stringstream ss;
        ss << options;
        m_helpOptions = ss.str();
        return;
    }

    using namespace sysutils::filesystem;
    if (!vm.count("datadir"))       { m_dataDir = getDefaultDataDir(DEFAULT_DATA_DIR); }
    if (!vm.count("config"))        { m_configFile = m_dataDir + "/" + DEFAULT_CONFIG_FILE; }
    else                            { m_configFile = m_dataDir + "/" + m_configFile; }

    fs::path p(m_configFile);
    if (fs::exists(p))
    {
        std::ifstream config(m_configFile);
        po::store(po::parse_config_file(config, options), vm);
        config.close();
        po::notify(vm); 
    }
}

