/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include "LSPClientWrapper.h"


LSPClientWrapper::LSPClientWrapper()
{
	initialized.store(false);
}

bool	
LSPClientWrapper::Initialize(const char *uri)
{
  client = new ProcessLanguageClient();
  client->Init("clangd");

  std::atomic<bool> on_error;
  on_error.store(false);
  readerThread = std::thread([&] {
	if (client->loop(*this) == -1)
	{
		printf("loop ended!\n");
		on_error.store(true);
		initialized.store(false);
	}
  });
  
  rootURI = uri;
  client->Initialize(rootURI);

  while (!initialized.load() && !on_error.load()) {
    fprintf(stderr, "Waiting for clangd initialization.. \n");
    usleep(500);
  }
  return on_error.load();
}

bool	
LSPClientWrapper::Dispose()
{
	if (!initialized)
    	return true;
    		
	client->Shutdown();
	
	while (initialized.load()) {
		fprintf(stderr, "Waiting for shutdown...\n");
		usleep(500);
	}
	
  	readerThread.detach();
  	client->Exit();
  	delete client;
  	client = NULL;
	return true;
}
		
		

void 
LSPClientWrapper::onNotify(string_ref method, value &params)
{
}
void
LSPClientWrapper::onResponse(value &ID, value &result)
{
	auto id = ID.get<std::string>();
	if (id.compare("initialize") == 0)
	{
		initialized.store(true); 
		client->Initialized();
		return;
	}
	if (id.compare("shutdown") == 0)
	{
       fprintf(stderr, "Shutdown received\n");
       initialized.store(false);
       return; 
	}
}
void 
LSPClientWrapper::onError(value &ID, value &error)
{
}
void 
LSPClientWrapper::onRequest(string_ref method, value &params, value &ID)
{
}
