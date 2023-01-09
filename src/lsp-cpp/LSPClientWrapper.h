/*
 * Copyright 2023, Andrea Anzani 
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef _H_LSPClientWrapper
#define _H_LSPClientWrapper

#include <SupportDefs.h>
#include <Locker.h>
#include "client.h"
#include <atomic>
#include <thread>


class LSPClientWrapper : public MessageHandler {
	
public:
			LSPClientWrapper();
		
	bool	Initialize(const char* uri);
	bool	Dispose();

    void onNotify(string_ref method, value &params);
    void onResponse(value &ID, value &result);
    void onError(value &ID, value &error);
    void onRequest(string_ref method, value &params, value &ID);
    

private:

	ProcessLanguageClient *client = NULL;

	std::atomic<bool> initialized;

	//MapMessageHandler my;

	std::thread readerThread;
	
	 string_ref rootURI;
};


#endif // _H_LSPClientWrapper
