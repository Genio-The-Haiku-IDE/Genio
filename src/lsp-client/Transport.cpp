// Code and inspiration taken from https://github.com/microsoft/language-server-protocol
// Copyright 2023, Andrea Anzani 

#include "Transport.h"
#include "Log.h"
#include "json.hpp"

#define    jsonrpc  "2.0"
///////////////////////

enum {
	kReadResult		= 'read',
	kWriteRequest	= 'writ'
};


AsyncJsonTransport::AsyncJsonTransport(MessageHandler& handler):BLooper("AsyncJsonTransport"),fHandler(handler)
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


void	
AsyncJsonTransport::DispatchResult(const char* json)
{
	try {
		auto value = nlohmann::json::parse(json);

		if (value.count("id")) {
		  if (value.contains("method")) {
			fHandler.onRequest(value["method"].get<std::string>(),
							  value["params"], value["id"]);
		  } else if (value.contains("result")) {
			fHandler.onResponse(value["id"].get<std::string>(), value["result"]);
		  } else if (value.contains("error")) {
			fHandler.onError(value["id"].get<std::string>(), value["error"]);
		  }
		} else if (value.contains("method")) {
		  if (value.contains("params")) {
			fHandler.onNotify(value["method"].get<std::string>(),
							 value["params"]);
		  }
		}
	
	} catch (std::exception &e) {
		return;
	}
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
				DispatchResult(data);
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


	
