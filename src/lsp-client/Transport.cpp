// Code and inspiration taken from https://github.com/microsoft/language-server-protocol
// Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>

#include "Transport.h"
#include "json.hpp"
#include <Messenger.h>
#define    jsonrpc  "2.0"
///////////////////////

enum {
	kReadResult		= 'read',
	kWriteRequest	= 'writ'
};


AsyncJsonTransport::AsyncJsonTransport(uint32 what, BMessenger& msgr)
					: BLooper("AsyncJsonTransport")
					, fWhat(what)
					, fMessenger(msgr)
{

}


void AsyncJsonTransport::notify(string_ref method, value &params) {
  nlohmann::json value = {{"jsonrpc", jsonrpc}, {"method", method}, {"params", params}};
  writeJson(value);
}
void AsyncJsonTransport::request(string_ref method, value &params, RequestID &id) {
  nlohmann::json rpc = {
      {"jsonrpc", jsonrpc}, {"id", id}, {"method", method}, {"params", params}};
  writeJson(rpc);
}

bool
AsyncJsonTransport::readStep()
{
	std::string data;
	bool result = readMessage(data);
	if(result){
		BMessage req(kReadResult);
		req.AddString("data", data.c_str());
		return (BLooper::PostMessage(&req) == B_OK);
	}
    return result;
}

void
AsyncJsonTransport::MessageReceived(BMessage* msg)
{
	switch(msg->what) {

		case kReadResult: {
			const char* data;
			if (msg->FindString("data", &data) == B_OK) {
				msg->what = fWhat;
				fMessenger.SendMessage(msg);
			}
			break;
		}

		case kWriteRequest: {
			const char* data;
			if (msg->FindString("data", &data) == B_OK) {
				std::string str(data);
				writeMessage(str);
			}
			break;
		}

		break;
		default:
			BLooper::MessageReceived(msg);
		break;

	};
}

bool
AsyncJsonTransport::writeJson(value& msg) {
	BMessage req(kWriteRequest);
	req.AddString("data", msg.dump().c_str());
	return (BLooper::PostMessage(&req) == B_OK);
}



