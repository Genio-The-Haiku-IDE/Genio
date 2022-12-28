/*
 * Copyright 2018, Your Name 
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


void
FileWrapper::Format()
{
	if (!initialized)
		return;
	my.bindResponse("textDocument/formatting", [&](json& params){
		
		if (!fEditor)
			return;
			
		auto items = params;
		for (json::reverse_iterator it = items.rbegin(); it != items.rend(); ++it) {


				int e_line = (*it)["range"]["end"]["line"].get<int>();
				int s_line = (*it)["range"]["start"]["line"].get<int>();
				int e_char = (*it)["range"]["end"]["character"].get<int>();
				int s_char = (*it)["range"]["start"]["character"].get<int>();
				
				printf("NewText: [%s] [%d,%d]->[%d,%d]\n", (*it)["newText"].get<std::string>().c_str(), s_line, s_char, e_line, e_char);
				
				if (e_line == s_line && e_char == s_char)
				{
					int pos = fEditor->SendMessage(SCI_POSITIONFROMLINE, s_line, 0);
					pos += s_char;
					//printf("NewText:\t pos[%d]\n", pos);
					
					fEditor->SendMessage(SCI_INSERTTEXT, pos, (sptr_t)((*it)["newText"].get<std::string>().c_str()));
				}
				else
				{
					//This is a replacement.
					int s_pos = fEditor->SendMessage(SCI_POSITIONFROMLINE, s_line, 0);
					s_pos += s_char;
					
					int e_pos = fEditor->SendMessage(SCI_POSITIONFROMLINE, e_line, 0);
					e_pos += e_char;
					
					printf("Replace:\t s_pos[%d] -> e_pos[%d]\n", s_pos, e_pos);
					
					fEditor->SendMessage(SCI_SETTARGETRANGE, s_pos, e_pos); 
					fEditor->SendMessage(SCI_REPLACETARGET, -1, (sptr_t)((*it)["newText"].get<std::string>().c_str())); 
					
					//SCI_SETTARGETRANGE(position start, position end)
					//SCI_REPLACETARGET(position length, const char *text) → position

				}
					

				
		}
	});
	client->Formatting(fFilenameURI.c_str());	
}


void	
FileWrapper::Completion(int _line, int _char){
	if (!initialized)
		return;
	Position position;
	position.line = _line;
	position.character = _char;
	CompletionContext context;

	my.bindResponse("textDocument/completion", [&](json& params){
		auto items = params["items"];
		std::string list;
		for (json::iterator it = items.begin(); it != items.end(); ++it) {
			fprintf(stderr, "completion: [%s]\n", (*it)["insertText"].get<std::string>().c_str());
			if (list.length() > 0)
				list += " ";
			list += (*it)["insertText"].get<std::string>();
			
		}
		if (list.length() > 0 && fEditor)
			fEditor->SendMessage(SCI_AUTOCSHOW, 0, (sptr_t)list.c_str());
	});
	client->Completion(fFilenameURI.c_str(), position, context);
}
