/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
 
#include "FileWrapper.h"
#include "client.h"
#include <debugger.h>
#include <stdio.h>
#include <thread>
#include <unistd.h>

#include <Application.h>

ProcessLanguageClient *client = NULL;

bool initialized = false;

MapMessageHandler my;

std::thread thread;
//
// utility
void FromSciPositionToLSPPosition(Editor *editor, const Sci_Position &pos,
                                  Position &lsp_position) {
	                                  
  lsp_position.line      = editor->SendMessage(SCI_LINEFROMPOSITION, pos, 0);
  Sci_Position end_pos   = editor->SendMessage(SCI_POSITIONFROMLINE, lsp_position.line, 0);
  lsp_position.character = editor->SendMessage(SCI_COUNTCHARACTERS, end_pos, pos);
}

Sci_Position 
FromLSPPositionToSciPosition(Editor *editor, const Position& lsp_position) {
	
  Sci_Position  sci_position;
  sci_position = editor->SendMessage(SCI_POSITIONFROMLINE, lsp_position.line, 0);
  sci_position = editor->SendMessage(SCI_POSITIONRELATIVE, sci_position, lsp_position.character);
  return sci_position;

}

void GetCurrentLSPPosition(Editor *editor, Position &lsp_position) {
  const Sci_Position pos = editor->SendMessage(SCI_GETSELECTIONSTART, 0, 0);
  FromSciPositionToLSPPosition(editor, pos, lsp_position);
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
}

void FromSciPositionToRange(Editor *editor, Sci_Position s_start,
                            Sci_Position s_end, Range &range) {
  FromSciPositionToLSPPosition(editor, s_start, range.start);
  FromSciPositionToLSPPosition(editor, s_end, range.end);
}

Sci_Position ApplyTextEdit(Editor *editor, json &textEdit) {
	
  Sci_Position s_pos = FromLSPPositionToSciPosition(editor, textEdit["range"]["start"].get<Position>());
  Sci_Position e_pos = FromLSPPositionToSciPosition(editor, textEdit["range"]["end"].get<Position>());

  editor->SendMessage(SCI_SETTARGETRANGE, s_pos, e_pos);
  
  Sci_Position replaced = editor->SendMessage(
      SCI_REPLACETARGET, -1,
      (sptr_t)(textEdit["newText"].get<std::string>().c_str()));

  return s_pos + replaced;
}

// end - utility

void FileWrapper::Initialize(const char *rootURI /*root folder*/) {

  client = new ProcessLanguageClient();
  
  client->Init("clangd");
  bool valid = true;
  
  thread = std::thread([&] { 
								if (client->loop(my) == -1) //closed pipe
								{
									fprintf(stderr, "Client loop ended!\n");
									valid = false;
									initialized = false; //quick and dirty!
									//while(true) sleep(1000000);
								} 
						    });

  my.bindResponse("initialize", [&](json &j) 
  { 
	initialized = true; 
	client->Initialized();
	
  });

  my.bindNotify("textDocument/publishDiagnostics", [](json &params) {
    // iterate the array
    auto j = params["diagnostics"];
    for (json::iterator it = j.begin(); it != j.end(); ++it) {
      // std::cout << *it << '\n';
      const auto msg = (*it)["message"].get<std::string>();
      fprintf(stderr, "Diagnostic: [%s]\n", msg.c_str());
    }
  });

  string_ref uri = rootURI;
  client->Initialize(uri);

  while (!initialized && valid) {
    fprintf(stderr, "Waiting for clangd initialization.. %d\n", initialized);
    usleep(500000);
  }
}

FileWrapper::FileWrapper(std::string filenameURI) {
  fFilenameURI = filenameURI;
  fToolTip = NULL;
}

void FileWrapper::didOpen(const char *text, Editor *editor) {
  if (!initialized)
    return;

  fEditor = editor;
  client->DidOpen(fFilenameURI.c_str(), text, "cpp");
  client->Sync();
}

void FileWrapper::didClose() {
  if (!initialized)
    return;

  client->DidClose(fFilenameURI.c_str());
  fEditor = NULL;
}

void FileWrapper::didSave() {
  if (!initialized)
    return;

  client->DidSave(fFilenameURI.c_str());
}

void FileWrapper::Dispose() {
	if (!initialized)
    	return;
    	
    my.bindResponse("shutdown", [&](json &j) {
          	fprintf(stderr, "Shutdown received\n");
    		initialized = false;
	});
	
	client->Shutdown();
	
	while (initialized) {
		fprintf(stderr, "Waiting for shutdown...\n");
		usleep(500000);
	}
	
  	thread.detach();
  	client->Exit();
  	delete client;
  	client = NULL;
 	return;    		
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
  FromSciPositionToLSPPosition(fEditor, start_pos, range.start);
  FromSciPositionToLSPPosition(fEditor, end_pos, range.end);

  event.range = range;
  event.text.assign(text, len);

  std::vector<TextDocumentContentChangeEvent> changes{event};

  client->DidChange(fFilenameURI.c_str(), changes, false);
  

}


void FileWrapper::_DoFormat(json &params) {

  fEditor->SendMessage(SCI_BEGINUNDOACTION, 0, 0);
  auto items = params;
  for (json::reverse_iterator it = items.rbegin(); it != items.rend(); ++it) {
    ApplyTextEdit(fEditor, (*it));
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
    FromSciPositionToRange(fEditor, s_start, s_end, range);
    my.bindResponse("textDocument/rangeFormatting", std::bind(&FileWrapper::_DoFormat, this, std::placeholders::_1));
    client->RangeFomatting(fFilenameURI.c_str(), range);
  } else {
    my.bindResponse("textDocument/formatting", std::bind(&FileWrapper::_DoFormat, this, std::placeholders::_1));
    client->Formatting(fFilenameURI.c_str());
   }
}

void	
FileWrapper::GoTo(FileWrapper::GoToType type)
{
  if (!initialized || !fEditor)
    return;
  
  Position position;
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
	break;
  };
  
   
}


void FileWrapper::StartHover(Sci_Position sci_position) {
  if (!initialized)
		return;
    
    Position position;
    FromSciPositionToLSPPosition(fEditor, sci_position, position);
    
    my.bindResponse("textDocument/hover", [&](json &result) {
			
			if (result == nlohmann::detail::value_t::null){
				EndHover();
				return;
			}
							
			std::string tip = result["contents"]["value"].get<std::string>();
			
			if (!fToolTip)
				fToolTip = new BTextToolTip(tip.c_str());
				
			fToolTip->SetText(tip.c_str());
			if(fEditor->Looper()->Lock()) {
				fEditor->ShowToolTip(fToolTip);
				fEditor->Looper()->Unlock();
			}
	});
  
	client->Hover(fFilenameURI.c_str(), position);
}

void FileWrapper::EndHover() {
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
  GetCurrentLSPPosition(fEditor, position);
  
  my.bindResponse("textDocument/signatureHelp", [&](json &result) {

		if (result["signatures"][0] != nlohmann::detail::value_t::null) {
			 const Sci_Position pos = fEditor->SendMessage(SCI_GETSELECTIONSTART, 0, 0);
			 auto str = result["signatures"][0]["label"].get<std::string>();
			fEditor->SendMessage(SCI_CALLTIPSHOW, pos, (sptr_t)(str.c_str()));
		}
  });
  

  client->SignatureHelp(fFilenameURI.c_str(), position);
}


void FileWrapper::SwitchSourceHeader() {
  if (!initialized)
    return;

  my.bindResponse("textDocument/switchSourceHeader", [&](json &result) {
    std::string uri = result.get<std::string>();
    OpenFile(uri);
  });

  client->SwitchSourceHeader(fFilenameURI.c_str());
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

        int endpos = ApplyTextEdit(fEditor, (*it)["textEdit"]);
        fEditor->SendMessage(SCI_SETEMPTYSELECTION, endpos, 0);
        fEditor->SendMessage(SCI_SCROLLCARET, 0, 0);
        break;
      }
    }
  }
  fEditor->SendMessage(SCI_AUTOCCANCEL, 0, 0);
  this->fCurrentCompletion = json({});
}

// trim from start (in place)
static inline void ltrim(std::string &s) {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
            return !std::isspace(ch);
          }));
}

void FileWrapper::StartCompletion() {
  if (!initialized || !fEditor)
    return;

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

bool
FileWrapper::StartCallTip()
{
	// printf("StartCallTip\n");
	std::string line = GetCurrentLine(fEditor);
	
	Position position;
	GetCurrentLSPPosition(fEditor, position);
	
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
	std::string line = GetCurrentLine(fEditor);
	Position position;
	GetCurrentLSPPosition(fEditor, position);
	
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
