/*
<<<<<<< HEAD
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
 
#include "FileWrapper.h"
#include "client.h"
#include <debugger.h>
#include <stdio.h>
#include <thread>
#include <unistd.h>
#include "LSPClientWrapper.h"
#include <Application.h>





FileWrapper::FileWrapper(std::string filenameURI, Editor* editor):
						 LSPTextDocument(filenameURI), 
						 fEditor(editor) {
  fToolTip = NULL;
  fLSPClientWrapper = NULL;
}

void
FileWrapper::ApplySettings()
{
  fEditor->SendMessage(SCI_SETINDICATORCURRENT, 0);
  fEditor->SendMessage(SCI_INDICSETFORE, 0, (255 | (0 << 8) | (0 << 16)));
  
  // int margins = fEditor->SendMessage(SCI_GETMARGINS);
  // fEditor->SendMessage(SCI_SETMARGINS, margins + 1);
  // fEditor->SendMessage(SCI_SETMARGINTYPEN, margins, SC_MARGIN_SYMBOL);
  // fEditor->SendMessage(SCI_SETMARGINWIDTHN,margins, 16);
  // fEditor->SendMessage(SCI_SETMARGINMASKN, margins, 1 << 2);
  // fEditor->SendMessage(SCI_MARKERSETBACK, margins, kMarkerForeColor);
  // fEditor->SendMessage(SCI_MARKERADD, 1, 2);
  
}

void
FileWrapper::UnsetLSPClient()
{
	if (!fLSPClientWrapper)
		return;
	
	didClose();
	initialized = false;
	fLSPClientWrapper->UnregisterTextDocument(this);
	fLSPClientWrapper = NULL;
}

void	
FileWrapper::SetLSPClient(LSPClientWrapper* cW) {

	assert(cW);
	assert(!initialized);
	
	fLSPClientWrapper = cW;
	fLSPClientWrapper->RegisterTextDocument(this);

	initialized = true;
	didOpen();
}

void FileWrapper::didOpen() {
  if (!initialized)
    return;
  const char* text = (const char*)fEditor->SendMessage(SCI_GETCHARACTERPOINTER);
	  	
  fLSPClientWrapper->DidOpen(this, fFilenameURI.c_str(), text, "cpp");
  //fLSPClientWrapper->Sync();
  
}

void FileWrapper::didClose() {
  if (!initialized)
    return;

  //_RemoveAllDiagnostics();
  
  fLSPClientWrapper->DidClose(this, fFilenameURI.c_str());
  
  fEditor = NULL;
}

void FileWrapper::didSave() {
  if (!initialized)
    return;

  fLSPClientWrapper->DidSave(this, fFilenameURI.c_str());
}

void
FileWrapper::didChange(const char* text, long len, Sci_Position start_pos, Sci_Position poslength)
{
  if (!initialized || !fEditor)
    return;

  Sci_Position end_pos =
      fEditor->SendMessage(SCI_POSITIONRELATIVE, start_pos, poslength);

  TextDocumentContentChangeEvent event;
  Range range;
  FromSciPositionToLSPPosition(start_pos, range.start);
  FromSciPositionToLSPPosition(end_pos, range.end);

  event.range = range;
  event.text.assign(text, len);

  std::vector<TextDocumentContentChangeEvent> changes{event};

  fLSPClientWrapper->DidChange(this, fFilenameURI.c_str(), changes, false);
}


void FileWrapper::_DoFormat(json &params) {

  fEditor->SendMessage(SCI_BEGINUNDOACTION, 0, 0);
  auto items = params;
  for (json::reverse_iterator it = items.rbegin(); it != items.rend(); ++it) {
    ApplyTextEdit((*it));
  }
  fEditor->SendMessage(SCI_ENDUNDOACTION, 0, 0);
}

void FileWrapper::Format() {
  if (!initialized || !fEditor)
    return;

  // format a range or format the whole doc?
  Sci_Position s_start = fEditor->SendMessage(SCI_GETSELECTIONSTART, 0, 0);
  Sci_Position s_end = fEditor->SendMessage(SCI_GETSELECTIONEND, 0, 0);

  if (s_start < s_end) {
    Range range;
    FromSciPositionToRange(s_start, s_end, range);
    fLSPClientWrapper->RangeFomatting(this, fFilenameURI.c_str(), range);
  } else {
    fLSPClientWrapper->Formatting(this, fFilenameURI.c_str());
   }
=======
 * Copyright 2018, Your Name <your@email.address>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "FileWrapper.h"
#include "lsp-cpp/include/client.h"
#include <unistd.h>
#include <stdio.h>
#include <thread>
#include <debugger.h>

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

int
PositionFromDuple(Editor* editor, int line, int character)
{
	int pos = 0;
	pos = editor->SendMessage(SCI_POSITIONFROMLINE, line, 0);
	pos = editor->SendMessage(SCI_POSITIONRELATIVE, pos,  character);
	return pos;
}
		


int
ApplyTextEdit(Editor* editor, json& textEdit)
{
		int e_line = textEdit["range"]["end"]["line"].get<int>();
		int s_line = textEdit["range"]["start"]["line"].get<int>();
		int e_char = textEdit["range"]["end"]["character"].get<int>();
		int s_char = textEdit["range"]["start"]["character"].get<int>();
		int s_pos  = PositionFromDuple(editor, s_line, s_char);
		int e_pos  = PositionFromDuple(editor, e_line, e_char);
		
		editor->SendMessage(SCI_SETTARGETRANGE, s_pos, e_pos); 
		int replaced = editor->SendMessage(SCI_REPLACETARGET, -1, (sptr_t)(textEdit["newText"].get<std::string>().c_str()));
		
		return s_pos + replaced;
}
void OpenFile(std::string &uri, int32 line = -1, int32 character = -1) {
  // fixe me if (uri.find("file://") == 0)
  {
    uri.erase(uri.begin(), uri.begin() + 7);
    BEntry entry(uri.c_str());
    entry_ref ref;
    if (entry.Exists()) {
      BMessage refs(B_REFS_RECEIVED);
      if (entry.GetRef(&ref) == B_OK) {
        refs.AddRef("refs", &ref);
        if (line != -1)
	    {
		  refs.AddInt32("be:line", line);
		  if (character != -1)
			refs.AddInt32("lsp:character", character);
        }  
        be_app->PostMessage(&refs);
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

	client->Initialize();
	
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
FileWrapper::didOpen(const char* text, long len) {
	if (!initialized)
		return;
		
	client->DidOpen(fFilenameURI.c_str(), text);
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
	my.bindResponse("shutdown", [&](json& j){
		initialized = false;
		thread.detach();
        //client->Exit();
        delete client;
		client = NULL;
	});
	
	if (initialized) {
		client->Shutdown();
		//client->Exit();
	}
		
    while(client!=NULL){sleep(1);}
    

}


void
FileWrapper::didChange(const char* text, long len, int s_line, int s_char, int e_line, int e_char) {
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
	
	client->DidChange(fFilenameURI.c_str(), changes, false);
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
				ApplyTextEdit(fEditor, (*it));
		}
	});
	client->Formatting(fFilenameURI.c_str());	
>>>>>>> 4ca1733 (simplified files structure and fixed a bug in GoTo lsp position)
}

void	
FileWrapper::GoTo(FileWrapper::GoToType type)
{
  if (!initialized || !fEditor)
    return;
  
  Position position;
<<<<<<< HEAD
  GetCurrentLSPPosition(position);
  
  switch(type) {
	case GOTO_DEFINITION:
		fLSPClientWrapper->GoToDefinition(this, fFilenameURI.c_str(), position);
	break;
	case GOTO_DECLARATION:
		fLSPClientWrapper->GoToDeclaration(this, fFilenameURI.c_str(), position);
	break;
	case GOTO_IMPLEMENTATION:
		fLSPClientWrapper->GoToImplementation(this, fFilenameURI.c_str(), position);
=======
  GetCurrentLSPPosition(fEditor, position);
  
  std::string event("textDocument/");
  switch(type) {
	case GOTO_DEFINITION:
		event += "definition";
	break;
	case GOTO_DECLARATION:
		event += "declaration";
	break;
	case GOTO_IMPLEMENTATION:
		event += "implementation";
	break;
  };

  my.bindResponse(event.c_str(), [&](json &items) {
    if (!items.empty()) {
      // TODO if more than one match??
      auto first = items[0];
      std::string uri = first["uri"].get<std::string>();      
     
      Position pos = first["range"]["start"].get<Position>();

      OpenFile(uri, pos.line + 1, pos.character);
    }
  });
  
  switch(type) {
	case GOTO_DEFINITION:
		client->GoToDefinition(fFilenameURI.c_str(), position);
	break;
	case GOTO_DECLARATION:
		client->GoToDeclaration(fFilenameURI.c_str(), position);
	break;
	case GOTO_IMPLEMENTATION:
		client->GoToImplementation(fFilenameURI.c_str(), position);
>>>>>>> 4ca1733 (simplified files structure and fixed a bug in GoTo lsp position)
	break;
  };
  
   
}


void FileWrapper::StartHover(Sci_Position sci_position) {
  if (!initialized)
		return;
<<<<<<< HEAD
  if (fEditor->SendMessage(SCI_INDICATORVALUEAT, 0, sci_position) == 1)
  {
	  for (auto& d: fLastDiagnostics) {
		Sci_Position from = FromLSPPositionToSciPosition(d.range.start);
		Sci_Position to   = FromLSPPositionToSciPosition(d.range.end);
			
		if (sci_position > from && sci_position <= to){
			_ShowToolTip(d.message.c_str());
			break;
		}	
	  }
	  return;
  }
  
  Position position;
  FromSciPositionToLSPPosition(sci_position, position);
  fLSPClientWrapper->Hover(this, fFilenameURI.c_str(), position);
}

void FileWrapper::EndHover() {
  if (!initialized)
		return;
	BAutolock l(fEditor->Looper());
	if(l.IsLocked()) {
		fEditor->HideToolTip();
	}
}

void	
FileWrapper::SignatureHelp()
{
	if (!initialized)
    return;

  Position position;
  GetCurrentLSPPosition(position);

  fLSPClientWrapper->SignatureHelp(this, fFilenameURI.c_str(), position);
=======

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
FileWrapper::SelectedCompletion(const char* text){
	if (!initialized || !fEditor)
		return;	
	

<<<<<<< HEAD
	//int cur = fEditor->SendMessage(SCI_AUTOCGETCURRENT, 0, 0);
	//printf("SelectedCompletion([%s]) - [%s] - cur[%d]\n", text, fCurrentCompletion.dump().c_str(),cur);
	
	if (this->fCurrentCompletion != json({}))
	{
		auto items = fCurrentCompletion;
		std::string list;
		for (json::iterator it = items.begin(); it != items.end(); ++it) {
			
			if ((*it)["label"].get<std::string>().compare(std::string(text)) == 0)
			{
				//first let's clean the text entered until the combo is selected:
				Sci_Position pos = fEditor->SendMessage(SCI_GETCURRENTPOS,0,0);
				
				 if (pos > fCompletionPosition) {
					fEditor->SendMessage(SCI_SETTARGETRANGE, fCompletionPosition, pos); 
					fEditor->SendMessage(SCI_REPLACETARGET, -1, (sptr_t)"");
				}
				
				int endpos = ApplyTextEdit(fEditor, (*it)["textEdit"]);
				fEditor->SendMessage(SCI_SETEMPTYSELECTION, endpos, 0);
				fEditor->SendMessage(SCI_SCROLLCARET, 0 ,0);
				break;
			}
			
		}		
	}
	fEditor->SendMessage(SCI_AUTOCCANCEL, 0, 0);
	this->fCurrentCompletion = json({});
=======
  Position position;
  GetCurrentLSPPosition(fEditor, position);
  
  my.bindResponse("textDocument/signatureHelp", [&](json &result) {

		if (result["signatures"][0] != nlohmann::detail::value_t::null) {
			 const Sci_Position pos = fEditor->SendMessage(SCI_GETSELECTIONSTART, 0, 0);
			 auto str = result["signatures"][0]["label"].get<std::string>();
			fEditor->SendMessage(SCI_CALLTIPSHOW, pos, (sptr_t)(str.c_str()));
		}
  });
  

  client->SignatureHelp(fFilenameURI.c_str(), position);
>>>>>>> 4ca1733 (simplified files structure and fixed a bug in GoTo lsp position)
}


void FileWrapper::SwitchSourceHeader() {
  if (!initialized)
    return;
<<<<<<< HEAD
  fLSPClientWrapper->SwitchSourceHeader(this, fFilenameURI.c_str());
=======

  my.bindResponse("textDocument/switchSourceHeader", [&](json &result) {
    std::string uri = result.get<std::string>();
    OpenFile(uri);
  });

  client->SwitchSourceHeader(fFilenameURI.c_str());
>>>>>>> 4ca1733 (simplified files structure and fixed a bug in GoTo lsp position)
}

void FileWrapper::SelectedCompletion(const char *text) {
  if (!initialized || !fEditor)
    return;

  if (this->fCurrentCompletion != json({})) {
    auto items = fCurrentCompletion;
    std::string list;
    for (json::iterator it = items.begin(); it != items.end(); ++it) {

      if ((*it)["label_sci"].get<std::string>().compare(std::string(text)) == 0) {
        // first let's clean the text entered until the combo is selected:
        Sci_Position pos = fEditor->SendMessage(SCI_GETCURRENTPOS, 0, 0);

        if (pos > fCompletionPosition) {
          fEditor->SendMessage(SCI_SETTARGETRANGE, fCompletionPosition, pos);
          fEditor->SendMessage(SCI_REPLACETARGET, -1, (sptr_t) "");
        }

<<<<<<< HEAD
        int endpos = ApplyTextEdit((*it)["textEdit"]);
=======
        int endpos = ApplyTextEdit(fEditor, (*it)["textEdit"]);
>>>>>>> 4ca1733 (simplified files structure and fixed a bug in GoTo lsp position)
        fEditor->SendMessage(SCI_SETEMPTYSELECTION, endpos, 0);
        fEditor->SendMessage(SCI_SCROLLCARET, 0, 0);
        break;
      }
    }
  }
  fEditor->SendMessage(SCI_AUTOCCANCEL, 0, 0);
  this->fCurrentCompletion = json({});
<<<<<<< HEAD
=======
>>>>>>> efcf8e5 (experimental calltip, GoTo supporting character position)
>>>>>>> 4ca1733 (simplified files structure and fixed a bug in GoTo lsp position)
}

// trim from start (in place)
static inline void ltrim(std::string &s) {
<<<<<<< HEAD
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
            return !std::isspace(ch);
          }));
}

void FileWrapper::StartCompletion() {
  if (!initialized || !fEditor)
    return;

=======
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
}

void	
FileWrapper::StartCompletion(){
	if (!initialized || !fEditor)
		return;
		
	//let's check if a completion is ongoing
	
	if (this->fCurrentCompletion != json({}))
	{
		//let's close the current Scintilla listbox
		fEditor->SendMessage(SCI_AUTOCCANCEL, 0, 0);
		//let's cancel any previous request running on clangd
		// --> TODO: cancel previous clangd request!
		
		//let's clean-up current request details:
		this->fCurrentCompletion = json({});
	}
	
	Position position;
	GetCurrentLSPPosition(fEditor, position);
	CompletionContext context;
	
	this->fCompletionPosition = fEditor->SendMessage(SCI_GETCURRENTPOS,0,0);

<<<<<<< HEAD
	my.bindResponse("textDocument/completion", [&](json& params){
		auto items = params["items"];
		std::string list;
		for (json::iterator it = items.begin(); it != items.end(); ++it) {
			std::string label = (*it)["label"].get<std::string>();
			ltrim(label);
			fprintf(stderr, "completion: [%s]\n", label.c_str());
			if (list.length() > 0)
				list += "@";
			list += label;
			(*it)["label"] = label;
			
		}
		if (list.length() > 0) {
			this->fCurrentCompletion = items;
			fEditor->SendMessage(SCI_AUTOCSETSEPARATOR, (int)'@', 0);
			fEditor->SendMessage(SCI_AUTOCSHOW, 0, (sptr_t)list.c_str());
			//printf("Dump([%s])\n", fCurrentCompletion.dump().c_str());
		}
			
	});
	client->Completion(fFilenameURI.c_str(), position, context);
}

=======
>>>>>>> 4ca1733 (simplified files structure and fixed a bug in GoTo lsp position)
  // let's check if a completion is ongoing

  if (this->fCurrentCompletion != json({})) {
    // let's close the current Scintilla listbox
    fEditor->SendMessage(SCI_AUTOCCANCEL, 0, 0);
    // let's cancel any previous request running on clangd
    // --> TODO: cancel previous clangd request!

    // let's clean-up current request details:
    this->fCurrentCompletion = json({});
  }

  Position position;
<<<<<<< HEAD
  GetCurrentLSPPosition(position);
  CompletionContext context;

  this->fCompletionPosition = fEditor->SendMessage(SCI_GETCURRENTPOS, 0, 0);
  fLSPClientWrapper->Completion(this, fFilenameURI.c_str(), position, context);
=======
  GetCurrentLSPPosition(fEditor, position);
  CompletionContext context;

  this->fCompletionPosition = fEditor->SendMessage(SCI_GETCURRENTPOS, 0, 0);

  my.bindResponse("textDocument/completion", [&](json &params) {
    auto items = params["items"];
    std::string list;
    for (json::iterator it = items.begin(); it != items.end(); ++it) {
      std::string label = (*it)["label"].get<std::string>();
      ltrim(label);
      // fprintf(stderr, "completion: [%s]\n", label.c_str());
      if (list.length() > 0)
        list += "\n";
      list += label;
      (*it)["label_sci"] = label;
    }
    if (list.length() > 0) {
      this->fCurrentCompletion = items;
      fEditor->SendMessage(SCI_AUTOCSETSEPARATOR, (int)'\n', 0);
      fEditor->SendMessage(SCI_AUTOCSHOW, 0, (sptr_t)list.c_str());
      // printf("Dump([%s])\n", fCurrentCompletion.dump().c_str());
    }
  });
  client->Completion(fFilenameURI.c_str(), position, context);
>>>>>>> 4ca1733 (simplified files structure and fixed a bug in GoTo lsp position)
}

// TODO move these, check if they are all used.. and move to a config section
// as these are c++/java parameters.. not sure they fit for all languages.

std::string calltipParametersEnd(")");
std::string calltipParametersStart("(");
std::string autoCompleteStartCharacters(".>");
std::string calltipWordCharacters("_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
std::string wordCharacters ("_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
std::string calltipParametersSeparators(",");

bool Contains(std::string const &s, char ch) noexcept {
	return s.find(ch) != std::string::npos;
}

constexpr bool IsASpace(int ch) noexcept {
	return (ch == ' ') || ((ch >= 0x09) && (ch <= 0x0d));
}

<<<<<<< HEAD

=======
std::string 
GetCurrentLine(Editor* fEditor) {
	// Get needed buffer size
	const Sci_Position len = fEditor->SendMessage(SCI_GETCURLINE, 0, (sptr_t)nullptr);
	// Allocate buffer, including space for NUL
	std::string text(len, '\0');
	// And get the line
	fEditor->SendMessage(SCI_GETCURLINE, len, (sptr_t)(&text[0]));
	return text;
}
>>>>>>> 4ca1733 (simplified files structure and fixed a bug in GoTo lsp position)

bool
FileWrapper::StartCallTip()
{
	// printf("StartCallTip\n");
<<<<<<< HEAD
	std::string line = GetCurrentLine();
	
	Position position;
	GetCurrentLSPPosition(position);
=======
	std::string line = GetCurrentLine(fEditor);
	
	Position position;
	GetCurrentLSPPosition(fEditor, position);
>>>>>>> 4ca1733 (simplified files structure and fixed a bug in GoTo lsp position)
	
	Sci_Position current = position.character;
	Sci_Position pos = fEditor->SendMessage(SCI_GETCURRENTPOS);
	calltipStartPosition = pos;
	
	do {
		int braces = 0;
		while (current > 0 && (braces || !Contains(calltipParametersStart, line[current - 1]))) {
			if (Contains(calltipParametersStart, line[current - 1]))
				braces--;
			else if (Contains(calltipParametersEnd, line[current - 1]))
				braces++;
			current--;
			pos--;
		}
		if (current > 0) {
			current--;
			pos--;
		} else
			break;
		while (current > 0 && IsASpace(line[current - 1])) {
			current--;
			pos--;
		}
	} while (current > 0 && !Contains(calltipWordCharacters, line[current - 1]));
	if (current <= 0)
		return true;

	startCalltipWord = current - 1;
	while (startCalltipWord > 0 &&
			Contains(calltipWordCharacters, line[startCalltipWord - 1])) {
		startCalltipWord--;
	}

	Position newPos;
	newPos.line = position.line;
	newPos.character = startCalltipWord;
<<<<<<< HEAD
	calltipPosition = FromLSPPositionToSciPosition(newPos);
	
	line.at(current + 1) = '\0';
	
  fLSPClientWrapper->SignatureHelp(this, fFilenameURI.c_str(), position);
=======
	calltipPosition = FromLSPPositionToSciPosition(fEditor, newPos);
	
	line.at(current + 1) = '\0';
	
	// printf("StartCallTip %d - %ld - [%s]\n", startCalltipWord, calltipPosition, line.c_str());

  my.bindResponse("textDocument/signatureHelp", [&](json &result) {

	    auto array = result["signatures"];
	    lastCalltip = array;
	    maxCalltip = array.size();
	    currentCalltip = -1;
	    
	    if (array.empty()) {
			fEditor->SendMessage(SCI_CALLTIPCANCEL);
			return;
		}	

		currentCalltip = 0;
		UpdateCallTip(0);
  });
  
  client->SignatureHelp(fFilenameURI.c_str(), position);
>>>>>>> 4ca1733 (simplified files structure and fixed a bug in GoTo lsp position)
  return true;
}

void
FileWrapper::UpdateCallTip(int deltaPos)
{
	if (deltaPos == 1 && currentCalltip > 0  && currentCalltip + 1)
		currentCalltip--;
	else if (deltaPos == 2 && currentCalltip + 1 < maxCalltip)
		currentCalltip++;
	

	
	functionDefinition = "";
	
	if (maxCalltip > 0)
		functionDefinition += "\001 " + std::to_string(currentCalltip+1) + " of " + 
								std::to_string(maxCalltip) + " \002";
		
	functionDefinition += lastCalltip[currentCalltip]["label"].get<std::string>();

	Sci_Position hackPos = fEditor->SendMessage(SCI_GETCURRENTPOS);
	fEditor->SendMessage(SCI_SETCURRENTPOS, calltipStartPosition);
	fEditor->SendMessage(SCI_CALLTIPSHOW, calltipPosition, (sptr_t)(functionDefinition.c_str()));
	fEditor->SendMessage(SCI_SETCURRENTPOS, hackPos);

	ContinueCallTip();
}

void
FileWrapper::ContinueCallTip()
{
<<<<<<< HEAD
	std::string line = GetCurrentLine();
	Position position;
	GetCurrentLSPPosition(position);
=======
	std::string line = GetCurrentLine(fEditor);
	Position position;
	GetCurrentLSPPosition(fEditor, position);
>>>>>>> 4ca1733 (simplified files structure and fixed a bug in GoTo lsp position)
	
	Sci_Position current = position.character;

	int braces = 0;
	size_t commas = 0;
	for (Sci_Position i = startCalltipWord; i < current; i++) {
		if (Contains(calltipParametersStart, line[i]))
			braces++;
		else if (Contains(calltipParametersEnd, line[i]) && braces > 0)
			braces--;
		else if (braces == 1 && Contains(calltipParametersSeparators, line[i]))
			commas++;
	}
	
	// if the num of commas is not compatible with current calltip
	// try to find a better one..
	auto params = lastCalltip[currentCalltip]["parameters"];
	// printf("DEBUG:%s %ld, %ld\n", params.dump().c_str(), params.size(), commas);
	if (commas >= params.size())
	{
		for (int i=0;i<maxCalltip;i++)
		{
			if (commas < lastCalltip[i]["parameters"].size())
			{
				currentCalltip = i;
				UpdateCallTip(0);
				return;
			}
		}
	}
	
	if (params.size() > commas)
	{
		// printf("DEBUG:%s\n", params.dump().c_str());
		int base  = functionDefinition.find("\002", 0);
		int start = base + params[commas]["label"][0].get<int>();
		int end   = base + params[commas]["label"][1].get<int>();
		// printf("DEBUG:%s %d,%d\n", params.dump().c_str(), start, end);
		fEditor->SendMessage(SCI_CALLTIPSETHLT, start, end);
	}	
}

void
FileWrapper::CharAdded(const char ch /*utf-8?*/)
{
	//printf("on char %c\n", ch);
	if (!initialized || !fEditor)
		return;
	
	/* algo took from SciTE editor .. */
	const Sci_Position selStart = fEditor->SendMessage(SCI_GETSELECTIONSTART);
	const Sci_Position selEnd   = fEditor->SendMessage(SCI_GETSELECTIONEND);
	
	if ((selEnd == selStart) && (selStart > 0)) {
		if (fEditor->SendMessage(SCI_CALLTIPACTIVE)) {
			if (Contains(calltipParametersEnd, ch)) {
				braceCount--;
				if (braceCount < 1)
					fEditor->SendMessage(SCI_CALLTIPCANCEL);
				else
					StartCallTip();
			} else if (Contains(calltipParametersStart, ch)) {
				braceCount++;
				StartCallTip();
			} else {
				ContinueCallTip();
			}
		} else if (fEditor->SendMessage(SCI_AUTOCACTIVE)) {
			if (Contains(calltipParametersStart, ch)) {
				braceCount++;
				StartCallTip();
			} else if (Contains(calltipParametersEnd, ch)) {
				braceCount--;
			} else if (!Contains(wordCharacters, ch)) {
				fEditor->SendMessage(SCI_AUTOCCANCEL);
				if (Contains(autoCompleteStartCharacters, ch)) {
					StartCompletion();
				}
			} 
		} else  {
			if (Contains(calltipParametersStart, ch)) {
				braceCount = 1;
				StartCallTip();
			} else {
				if (Contains(autoCompleteStartCharacters, ch)) {
					StartCompletion();
				} 
			}
		}
	}
}
<<<<<<< HEAD

void
FileWrapper::_ShowToolTip(const char* text)
{
	if (!fToolTip)
		fToolTip = new BTextToolTip(text);
		
	fToolTip->SetText(text);
	if(fEditor->Looper()->Lock()) {
		fEditor->ShowToolTip(fToolTip);
		fEditor->Looper()->Unlock();
	}
}
void 
FileWrapper::_DoHover(nlohmann::json& result)
{
	if (result == nlohmann::detail::value_t::null){
				EndHover();
				return;
	}
					
	std::string tip = result["contents"]["value"].get<std::string>();
	
	_ShowToolTip(tip.c_str());
}

void 
FileWrapper::_DoGoTo(nlohmann::json& items){
    if (!items.empty()) {
      // TODO if more than one match??
      auto first = items[0];
      std::string uri = first["uri"].get<std::string>();      
     
      Position pos = first["range"]["start"].get<Position>();

      OpenFile(uri, pos.line + 1, pos.character);
    }
};
  
void
FileWrapper::_DoSignatureHelp(json &result) {
	if (result["signatures"][0] != nlohmann::detail::value_t::null) {
		 const Sci_Position pos = fEditor->SendMessage(SCI_GETSELECTIONSTART, 0, 0);
		 auto str = result["signatures"][0]["label"].get<std::string>();
		fEditor->SendMessage(SCI_CALLTIPSHOW, pos, (sptr_t)(str.c_str()));
	}
};

void	
FileWrapper::_DoSwitchSourceHeader(json &result) {
	std::string url = result.get<std::string>();
    OpenFile(url);
};

void	
FileWrapper::_DoCompletion(json &params) {
    auto items = params["items"];
    std::string list;
    for (json::iterator it = items.begin(); it != items.end(); ++it) {
      std::string label = (*it)["label"].get<std::string>();
      ltrim(label);
      if (list.length() > 0)
        list += "\n";
      list += label;
      (*it)["label_sci"] = label;
    }
    if (list.length() > 0) {
      this->fCurrentCompletion = items;
      fEditor->SendMessage(SCI_AUTOCSETSEPARATOR, (int)'\n', 0);
      fEditor->SendMessage(SCI_AUTOCSHOW, 0, (sptr_t)list.c_str());
    }
}

void
FileWrapper::_RemoveAllDiagnostics()
{
	// remove all the indicators..
	fEditor->SendMessage(SCI_INDICATORCLEARRANGE, 0, fEditor->SendMessage(SCI_GETTEXTLENGTH));
	fLastDiagnostics.clear();	
}

void	
FileWrapper::_DoDiagnostics(nlohmann::json& params)
{
	//auto dias = params["diagnostics"];
	auto vect = params["diagnostics"].get<std::vector<Diagnostic>>();
	
	// remove all the indicators..
	fEditor->SendMessage(SCI_INDICATORCLEARRANGE, 0, fEditor->SendMessage(SCI_GETTEXTLENGTH));
	fLastDiagnostics.clear();

	for (auto &v: vect)
	{
		Range &r = v.range;
		Sci_Position from = FromLSPPositionToSciPosition(r.start);
		Sci_Position to   = FromLSPPositionToSciPosition(r.end);

		LogTrace("Diagnostics [%ld->%ld] [%s]", from, to, v.message.c_str());
		fEditor->SendMessage(SCI_INDICATORFILLRANGE, from, to-from);
	}
	fLastDiagnostics = vect;
}
		
#include "Log.h"

#define IF_ID(METHOD_NAME, METHOD) if (id.compare(METHOD_NAME) == 0) { METHOD(result); return; }

void 
FileWrapper::onNotify(std::string id, value &result)
{
	IF_ID("textDocument/publishDiagnostics", _DoDiagnostics);
	
	LogError("FileWrapper::onNotify not handled! [%s]", id.c_str());
}
void
FileWrapper::onResponse(RequestID id, value &result)
{
	IF_ID("textDocument/hover", _DoHover);
	IF_ID("textDocument/rangeFormatting", _DoFormat);
	IF_ID("textDocument/formatting", _DoFormat);
	IF_ID("textDocument/definition", _DoGoTo);
	IF_ID("textDocument/declaration", _DoGoTo);
	IF_ID("textDocument/implementation", _DoGoTo);
	IF_ID("textDocument/signatureHelp", _DoSignatureHelp);
	IF_ID("textDocument/switchSourceHeader", _DoSwitchSourceHeader);
	IF_ID("textDocument/completion", _DoCompletion);
	
	LogError("FileWrapper::onResponse not handled! [%s]", id.c_str());

}
void 
FileWrapper::onError(RequestID id, value &error)
{ 
	LogError("onError not implemented! [%s] [%s]", error.dump().c_str(), id.c_str());
}
void 
FileWrapper::onRequest(std::string method, value &params, value &ID)
{
	//LogError("onRequest not implemented! [%s] [%s] [%s]", method.c_str(), params.dump().c_str(), ID.dump().c_str());
}

// utility
void 
FileWrapper::FromSciPositionToLSPPosition(const Sci_Position &pos,
                                  Position &lsp_position) {
	                                  
  lsp_position.line      = fEditor->SendMessage(SCI_LINEFROMPOSITION, pos, 0);
  Sci_Position end_pos   = fEditor->SendMessage(SCI_POSITIONFROMLINE, lsp_position.line, 0);
  lsp_position.character = fEditor->SendMessage(SCI_COUNTCHARACTERS, end_pos, pos);
}

Sci_Position 
FileWrapper::FromLSPPositionToSciPosition(const Position& lsp_position) {
	
  Sci_Position  sci_position;
  sci_position = fEditor->SendMessage(SCI_POSITIONFROMLINE, lsp_position.line, 0);
  sci_position = fEditor->SendMessage(SCI_POSITIONRELATIVE, sci_position, lsp_position.character);
  return sci_position;

}

void 
FileWrapper::GetCurrentLSPPosition(Position &lsp_position) {
  const Sci_Position pos = fEditor->SendMessage(SCI_GETSELECTIONSTART, 0, 0);
  FromSciPositionToLSPPosition(pos, lsp_position);
}

void 
FileWrapper::FromSciPositionToRange(Sci_Position s_start,
                            Sci_Position s_end, Range &range) {
  FromSciPositionToLSPPosition(s_start, range.start);
  FromSciPositionToLSPPosition(s_end, range.end);
}

Sci_Position 
FileWrapper::ApplyTextEdit(json &textEdit) {
	
  Sci_Position s_pos = FromLSPPositionToSciPosition(textEdit["range"]["start"].get<Position>());
  Sci_Position e_pos = FromLSPPositionToSciPosition(textEdit["range"]["end"].get<Position>());

  fEditor->SendMessage(SCI_SETTARGETRANGE, s_pos, e_pos);
  
  Sci_Position replaced = fEditor->SendMessage(
      SCI_REPLACETARGET, -1,
      (sptr_t)(textEdit["newText"].get<std::string>().c_str()));

  return s_pos + replaced;
}

void 
FileWrapper::OpenFile(std::string &uri, int32 line, int32 character) {
  // fixe me if (uri.find("file://") == 0)
  {
    uri.erase(uri.begin(), uri.begin() + 7);
    BEntry entry(uri.c_str());
    entry_ref ref;
    if (entry.Exists()) {
      BMessage refs(B_REFS_RECEIVED);
      if (entry.GetRef(&ref) == B_OK) {
        refs.AddRef("refs", &ref);
        if (line != -1)
	    {
		  refs.AddInt32("be:line", line);
		  if (character != -1)
			refs.AddInt32("lsp:character", character);
        }  
        be_app->PostMessage(&refs);
      }
    }
  }
}

std::string 
FileWrapper::GetCurrentLine() {

	const Sci_Position len = fEditor->SendMessage(SCI_GETCURLINE, 0, (sptr_t)nullptr);
	std::string text(len, '\0');
	fEditor->SendMessage(SCI_GETCURLINE, len, (sptr_t)(&text[0]));
	return text;
}

// end - utility
=======
>>>>>>> efcf8e5 (experimental calltip, GoTo supporting character position)
>>>>>>> 4ca1733 (simplified files structure and fixed a bug in GoTo lsp position)
