///////////////////////////////////////////////////////////////////////////////
//
// CoinDB
//
// CoinDBConfig.h
//
// Copyright (c) 2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#pragma once

#include <CoinQ/CoinQ_coinparams.h>

#include <string>
#include <sstream>
#include <fstream>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include <sysutils/filesystem.h>

const std::string DEFAULT_DATA_DIR = "CoinDB";
const std::string DEFAULT_CONFIG_FILE = "coindb.conf";
const std::string DEFAULT_NETWORK_NAME = "bitcoin";

class CoinDBConfig
{
public:
    CoinDBConfig();
    virtual ~CoinDBConfig() { }

    std::string getHelpOptions() const;
    virtual bool parseParams(int argc, char* argv[]);

    const std::string&          getDataDir() const { return m_dataDir; }
    const std::string&          getConfigFile() const { return m_configFile; }
    const std::string&          getDatabaseUser() const { return m_databaseUser; }
    const std::string&          getDatabasePassword() const { return m_databasePassword; }
    const std::string&          getNetworkName() const { return m_networkName; }
    const CoinQ::CoinParams&    getCoinParams() const { return m_networkSelector.getCoinParams(); }

protected:
    boost::program_options::options_description m_options;
    boost::program_options::variables_map m_vm;

    std::string m_dataDir;
    std::string m_configFile;
    std::string m_databaseUser;
    std::string m_databasePassword;
    std::string m_networkName;

    CoinQ::NetworkSelector m_networkSelector;
};

inline CoinDBConfig::CoinDBConfig() : m_options("Options")
{
    namespace po = boost::program_options;

    m_options.add_options()
        ("help", "display help message")
        ("datadir", po::value<std::string>(&m_dataDir), "data directory")
        ("config", po::value<std::string>(&m_configFile), "name of the configuration file")
        ("dbuser", po::value<std::string>(&m_databaseUser), "database user")
        ("dbpasswd", po::value<std::string>(&m_databasePassword), "database password")
        ("network", po::value<std::string>(&m_networkName), "network name (default: bitcoin)")
    ;
}

inline std::string CoinDBConfig::getHelpOptions() const
{
    std::stringstream ss;
    ss << m_options;
    return ss.str();
}

inline bool CoinDBConfig::parseParams(int argc, char* argv[])
{
    namespace po = boost::program_options;

    po::store(po::parse_command_line(argc, argv, m_options), m_vm);
    po::notify(m_vm);

    if (m_vm.count("help")) return false;

    using namespace sysutils::filesystem;
    if (!m_vm.count("datadir"))     { m_dataDir = getDefaultDataDir(DEFAULT_DATA_DIR); }
    if (!m_vm.count("config"))      { m_configFile = m_dataDir + "/" + DEFAULT_CONFIG_FILE; }
    else                            { m_configFile = m_dataDir + "/" + m_configFile; }

    namespace fs = boost::filesystem;
    fs::path dataDirPath(m_dataDir);
    if ((!fs::exists(dataDirPath) && !fs::create_directory(dataDirPath)) || !fs::is_directory(dataDirPath))
        throw std::runtime_error("Invalid datadir.");

    fs::path p(m_configFile);
    if (fs::exists(p))
    {
        std::ifstream ifs(m_configFile);
        po::store(po::parse_config_file(ifs, m_options), m_vm);
        ifs.close();
        po::notify(m_vm); 
    }

    if (!m_vm.count("network")) { m_networkName = DEFAULT_NETWORK_NAME; }
    std::transform(m_networkName.begin(), m_networkName.end(), m_networkName.begin(), ::tolower);
    m_networkSelector.select(m_networkName);

    return true;
}

