/*
 * Copyright 2023, Andrea Anzani 
 * All rights reserved. Distributed under the terms of the MIT license.
 */
 
#ifndef MessageHandler_H
#define MessageHandler_H


#include "uri.h"
#include "json.hpp"
#include <string>

using value = json;
using RequestID = std::string;

class MessageHandler {
public:
    MessageHandler() = default;
    virtual void onNotify(std::string method, value &params) {}
    virtual void onResponse(RequestID ID, value &result) {}
    virtual void onError(RequestID ID, value &error) {}
    virtual void onRequest(std::string method, value &params, value &ID) {}

};


#endif // MessageHandler_H
