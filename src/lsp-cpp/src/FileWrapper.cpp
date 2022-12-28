/*
 * Copyright 2022, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "FileWrapper.h"
#include "client.h"
#include <unistd.h>
#include <stdio.h>
#include <thread>
#include <debugger.h>
#include "Editor.h"
#include <Application.h>

ProcessLanguageClient* client = NULL;

bool initialized = false;

MapMessageHandler my;

std::thread thread;

//utility
void
GetCurrentLSPPosition(Editor* editor, Position& position)
{
	Sci_Position pos   = editor->SendMessage(SCI_GETCURRENTPOS,0,0);
	position.line      = editor->SendMessage(SCI_LINEFROMPOSITION, pos, 0);
	int end_pos        = editor->SendMessage(SCI_POSITIONFROMLINE, position.line, 0);
	position.character = editor->SendMessage(SCI_COUNTCHARACTERS, end_pos, pos);
}

void
OpenFile(std::string& uri, int32 line = -1)
{
	//fixe me if (uri.find("file://") == 0)
	{
		uri.erase(uri.begin(), uri.begin()+7);

		BEntry entry(uri.c_str());
		entry_ref ref;
		if (entry.Exists()) {
			BMessage	refs(B_REFS_RECEIVED);
			if (entry.GetRef(&ref) == B_OK)
			{
				refs.AddRef("refs", &ref);
				if (line != -1)
					refs.AddInt32("be:line", line + 1);
				be_app->PostMessage(&refs);
			}
			
		}

	}
}
		
//end - utility

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
			
		fEditor->SendMessage(SCI_BEGINUNDOACTION, 0, 0);
		auto items = params;
		for (json::reverse_iterator it = items.rbegin(); it != items.rend(); ++it) {


				int e_line = (*it)["range"]["end"]["line"].get<int>();
				int s_line = (*it)["range"]["start"]["line"].get<int>();
				int e_char = (*it)["range"]["end"]["character"].get<int>();
				int s_char = (*it)["range"]["start"]["character"].get<int>();
				
				//printf("NewText: [%s] [%d,%d]->[%d,%d]\n", (*it)["newText"].get<std::string>().c_str(), s_line, s_char, e_line, e_char);
				
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
					
					//printf("Replace:\t s_pos[%d] -> e_pos[%d]\n", s_pos, e_pos);
					
					fEditor->SendMessage(SCI_SETTARGETRANGE, s_pos, e_pos); 
					fEditor->SendMessage(SCI_REPLACETARGET, -1, (sptr_t)((*it)["newText"].get<std::string>().c_str())); 
					
					//SCI_SETTARGETRANGE(position start, position end)
					//SCI_REPLACETARGET(position length, const char *text) → position

				}
		}
		fEditor->SendMessage(SCI_ENDUNDOACTION, 0, 0);
	});
	client->Formatting(fFilenameURI.c_str());	
}

void
FileWrapper::GoToDefinition() {
	if (!initialized || !fEditor)
		return;

	Position position;
	GetCurrentLSPPosition(fEditor, position);
	
	my.bindResponse("textDocument/definition", [&](json& items){
		if (!items.empty())
		{
			//TODO if more than one match??
			auto first=items[0];
			std::string uri = first["uri"].get<std::string>();
			
			int32 s_line = first["range"]["start"]["line"].get<int>();
			
			OpenFile(uri, s_line);	
		}
	});
	
	client->GoToDefinition(fFilenameURI.c_str(), position);
}

void
FileWrapper::GoToDeclaration() {
	if (!initialized || !fEditor)
		return;

	Position position;
	GetCurrentLSPPosition(fEditor, position);
	
	my.bindResponse("textDocument/declaration", [&](json& items){
		if (!items.empty())
		{
			//TODO if more than one match??
			auto first=items[0];
			std::string uri = first["uri"].get<std::string>();
			
			int32 s_line = first["range"]["start"]["line"].get<int>();
			
			OpenFile(uri, s_line);	
		}
	});
	
	client->GoToDeclaration(fFilenameURI.c_str(), position);
}

void
FileWrapper::SwitchSourceHeader() {
	if (!initialized)
		return;
		
	my.bindResponse("textDocument/switchSourceHeader", [&](json& result){		
		std::string uri = result.get<std::string>();
		OpenFile(uri);		
	});
	
	client->SwitchSourceHeader(fFilenameURI.c_str());
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
