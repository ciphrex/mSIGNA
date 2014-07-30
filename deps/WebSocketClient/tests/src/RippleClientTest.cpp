////////////////////////////////////////////////////////////////////////////////
//
// RippleClientTest.cpp
//
// Copyright (c) 2014 Eric Lombrozo, all rights reserved
//

#include <WebSocketClient.h>

#include <iostream>

using namespace std;
using namespace json_spirit;

int main(int argc, char** argv)
{
    if (argc != 2) {
        cout << "Usage: " << argv[0] << " <server url>" << endl;
        return 1;
    }

    WebSocket::Client socket("type");
    socket.returnFullResponse(true);

    socket.on("transaction", [] (const Value& obj) {
        cout << "transaction: " << write_string<Value>(obj, true) << endl << endl;
    }).on("ledgerClosed", [] (const Value& obj) {
        cout << "ledgerClosed: " << write_string<Value>(obj, true) << endl << endl;
    }).on("serverStatus", [] (const Value& obj) {
        cout << "serverStatus: " << write_string<Value>(obj, true) << endl << endl;
    });

    socket.start(argv[1], [&]() {
        vector<string> streams;
        streams.push_back("transactions");
        streams.push_back("ledger");
        streams.push_back("server");
        const Array streamsVal(streams.begin(), streams.end());
        Object cmd;
        cmd.push_back(Pair("command", "subscribe"));
        cmd.push_back(Pair("streams", streamsVal));
        socket.send(cmd, [](const Value& result) {
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

    return 0;
}
