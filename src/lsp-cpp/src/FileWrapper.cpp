/*
 * Copyright 2018, Your Name <your@email.address>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "FileWrapper.h"
#include "client.h"
#include <unistd.h>
#include <stdio.h>
#include <thread>

ProcessLanguageClient* client = NULL;

bool initialized = false;

MapMessageHandler my;

std::thread thread;

void FileWrapper::Initialize(const char* rootURI) {
	
	client = new ProcessLanguageClient("clangd");

    thread = std::thread([&] {
        client->loop(my);
    });
    
	my.bindResponse("initialize", [&](json& j){		
        initialized = true;
	});

	string_ref uri = rootURI;
	client->Initialize(uri);
	
	while(!initialized)
	{
		fprintf(stderr, "Waiting for clangd initialization.. %d\n", initialized);
		sleep(1);
	}
}

FileWrapper::FileWrapper(std::string filenameURI)
{
	fFilenameURI = filenameURI;
}

void	
FileWrapper::didOpen(const char* text) {
	if (!initialized)
		return;
		
	client->DidOpen(fFilenameURI.c_str(), text, "cpp");
    client->Sync();
}

void	
FileWrapper::didClose() {
	if (!initialized)
		return;
		
	client->DidClose(fFilenameURI.c_str());
}

void
FileWrapper::Dispose()
{
	if (!initialized)
		return;
		
    client->Exit();
    thread.detach();
    delete client;
    client = NULL;
    initialized = false;
}

void
FileWrapper::didChange(const char* text, long len, int s_line, int s_char, int e_line, int e_char) {
	printf("didChange[%ld][%.*s]\n",len, (int)len,text);
	if (!initialized)
		return;
	
	TextDocumentContentChangeEvent event;
	Range range;
		
	range.start.line = s_line;
	range.start.character = s_char;

	range.end.line = e_line;
	range.end.character = e_char;
	
	event.range = range;
	event.text.assign(text, len);
	
	std::vector<TextDocumentContentChangeEvent> changes{event};
//		changes[0] = event;
	
	client->DidChange(fFilenameURI.c_str(), changes, true);
}
