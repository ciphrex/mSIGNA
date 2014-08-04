////////////////////////////////////////////////////////////////////////////////
//
// CoinSocketClientTest.cpp
//
// Copyright (c) 2014 Eric Lombrozo, all rights reserved
//

#include <Client.h>

#include <iostream>

using namespace std;
using namespace json_spirit;

int main(int argc, char** argv)
{
    if (argc != 2) {
        cout << "Usage: " << argv[0] << " <server url>" << endl;
        return 1;
    }

    WebSocket::Client socket("event", "data");

    socket.on("statuschanged", [](const Value& data) {
        cout << "statuschanged: " << write_string<Value>(data, true) << endl << endl;
    }).on("txinserted", [](const Value& data) {
        cout << "txinserted: " << write_string<Value>(data, true) << endl << endl;
    }).on("txstatuschanged", [](const Value& data) {
        cout << "txstatuschanged: " << write_string<Value>(data, true) << endl << endl;
    });

    socket.start(argv[1], [&]() {
        JsonRpc::Request getvaultinfo("getvaultinfo");
        socket.send(getvaultinfo, [](const Value& result) {
            cout << "Command result: " << write_string<Value>(result, true) << endl << endl;
        }, [](const Value& error) {
            cout << "Command error: " << write_string<Value>(error, true) << endl << endl; 
        });

        Array params;
        params.push_back(1);
        JsonRpc::Request gettx("gettx", params); 
        socket.send(gettx, [](const Value& result) {
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
