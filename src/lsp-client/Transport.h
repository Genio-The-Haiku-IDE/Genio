//
// Code and inspiration taken from https://github.com/microsoft/language-server-protocol
// Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
//

#ifndef LSP_TRANSPORT_H
#define LSP_TRANSPORT_H

#include "MessageHandler.h"
#include <Looper.h>
#include <Messenger.h>

class Transport {
public:
    virtual void notify(string_ref method,  value &params) = 0;
    virtual void request(string_ref method, value &params, RequestID &id) = 0;

    virtual bool  readStep() = 0;

    virtual bool readMessage(std::string &) = 0;
    virtual bool writeMessage(std::string &) = 0;
};

class AsyncJsonTransport: public Transport, public BLooper {

public:
		 AsyncJsonTransport(uint32 handler, BMessenger& msgr);

    void notify(string_ref method, value &params) override;
    void request(string_ref method, value &params, RequestID &id) override;


	bool  readStep() override;

	void MessageReceived(BMessage* msg) override;

private:


	bool writeJson(value& value);

	uint32			fWhat;
	BMessenger		fMessenger;

};

#endif //LSP_TRANSPORT_H
