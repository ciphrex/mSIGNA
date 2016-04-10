////////////////////////////////////////////////////////////////////////////////
//
// CoinSocketClient
//
// Sync.cpp
//
// Copyright (c) 2014 Eric Lombrozo, all rights reserved
//

#include "Sync.h"

#include <WebSocketAPI/Client.h>

using namespace CoinSocket::Client;
using namespace json_spirit;

Sync::Sync() :
    m_socket("event", "data")
{
    m_socket.on("txinsertedserialized", [this](const Value& data) {
        cout << "txinsertedserialized: " << write_string<Value>(data, true) << endl << endl;
    }).on("txstatuschangedserialized", [](const Value& data) {
        cout << "txstatuschangedserialized: " << write_string<Value>(data, true) << endl << endl;
    });
}

Sync::~Sync()
{
}

void Sync::start(int txStatusFlags, const std::string& url)
{
}

void Sync::stop()
{
}

void Sync::sendTx(std::shared_ptr<Tx> tx)
{
}




    try
    {
        socket.start(argv[1], [&]() {
            JsonRpc::Request getvaultinfo("getvaultinfo");
            socket.send(getvaultinfo, [](const Value& result) {
                cout << "Command result: " << write_string<Value>(result, true) << endl << endl;
            }, [](const Value& error) {
                cout << "Command error: " << write_string<Value>(error, true) << endl << endl; 
            });

            Array params;
            params.push_back("all");
            JsonRpc::Request subscribe("subscribe", params); 
            socket.send(subscribe, [](const Value& result) {
                cout << "Command result: " << write_string<Value>(result, true) << endl << endl;
            }, [](const Value& error) {
                cout << "Command error: " << write_string<Value>(error, true) << endl << endl; 
            });
        }, []() {
            cout << "Connection closed." << endl << endl;
        }, [](const string& msg) {
            // cout << "Log: " << msg << endl << endl;
        }, [](const string& msg) {
            cout << "Error: " << msg << endl << endl;
        });
    }
    catch (const exception& e)
    {
        cerr << "Client start error: " << e.what() << endl;
        return -1;
    }

    return 0;
}

    }
}
