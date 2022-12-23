/*
 * Copyright 2018, Your Name <your@email.address>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "FileWrapper.h"
#include "client.h"
#include <unistd.h>
#include <stdio.h>
#include <thread>
#include <debugger.h>
#include "Editor.h"

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
	
	my.bindNotify("textDocument/publishDiagnostics", [](json& params){
		
		// iterate the array
		auto j = params["diagnostics"];
		for (json::iterator it = j.begin(); it != j.end(); ++it) {
		  //std::cout << *it << '\n';
		  const auto msg = (*it)["message"].get<std::string>();
		  fprintf(stderr, "Diagnostic: [%s]\n", msg.c_str());
		}
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
FileWrapper::didOpen(const char* text, Editor* editor) {
	if (!initialized)
		return;
	
	fEditor = editor;
	client->DidOpen(fFilenameURI.c_str(), text, "cpp");
    client->Sync();
}

void	
FileWrapper::didClose() {
	if (!initialized)
		return;
		
	client->DidClose(fFilenameURI.c_str());
	fEditor = NULL;
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
	//printf("didChange[%ld][%.*s]\n",len, (int)len,text);
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
	
	client->DidChange(fFilenameURI.c_str(), changes, true);
}
#include <Window.h>
void	
FileWrapper::Completion(int _line, int _char){
	if (!initialized)
		return;
	Position position;
	position.line = _line;
	position.character = _char;
	CompletionContext context;
	//std::string fake("bene male malissimo"); fEditor->Window()->MoveTo(11,11);
	//fEditor->SendMessage(SCI_AUTOCSHOW, 0, (sptr_t)fake.c_str());
	
	
	my.bindResponse("textDocument/completion", [&](json& params){
		auto items = params["items"];
		std::string list;
		for (json::iterator it = items.begin(); it != items.end(); ++it) {
			fprintf(stderr, "completion: [%s]\n", (*it)["insertText"].get<std::string>().c_str());
			if (list.length() > 0)
				list += " ";
			list += (*it)["insertText"].get<std::string>();
			
		}
		if (fEditor)
			fEditor->SendMessage(SCI_AUTOCSHOW, 0, (sptr_t)list.c_str());
	});
	client->Completion(fFilenameURI.c_str(), position, context);
}
