//
// Created by Alex on 2020/1/28. (https://github.com/microsoft/language-server-protocol)
// Additional changes by Andrea Anzani 
//

#ifndef LSP_TRANSPORT_H
#define LSP_TRANSPORT_H

#include "MessageHandler.h"
#include <functional>
#include <utility>

//TODO: why using string_ref?

class Transport {
public:
    virtual void notify(string_ref method,  value &params) = 0;
    virtual void request(string_ref method, value &params, RequestID &id) = 0;
    virtual int  loop(MessageHandler &) = 0;
};

class JsonTransport : public Transport {
public:
    const char *jsonrpc = "2.0";
    int  loop(MessageHandler &handler) override;
    void notify(string_ref method, value &params) override;
    void request(string_ref method, value &params, RequestID &id) override;

    virtual bool readJson(value &) = 0;
    virtual bool writeJson(value &) = 0;
};

#endif //LSP_TRANSPORT_H
