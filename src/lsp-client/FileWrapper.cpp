/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
 
#include "FileWrapper.h"
#include "LSPClient.h"
#include <debugger.h>
#include <stdio.h>
#include <thread>
#include <unistd.h>
#include <algorithm>
#include "LSPClientWrapper.h"
#include <Application.h>
#include "TextUtils.h"
#include "Log.h"
#include "Editor.h"
#include "protocol.h"

#define IF_ID(METHOD_NAME, METHOD) if (id.compare(METHOD_NAME) == 0) { METHOD(result); return; }
#define IND_DIAG 0
#define IND_LINK 1

FileWrapper::FileWrapper(std::string filenameURI, Editor* editor):
						 LSPTextDocument(filenameURI), 
						 fEditor(editor) {
  fToolTip = NULL;
  fLSPClientWrapper = NULL;
  assert(fEditor);
}

void
FileWrapper::ApplySettings()
{

  fEditor->SendMessage(SCI_INDICSETFORE, IND_DIAG, (255 | (0 << 8) | (0 << 16)));  

  fEditor->SendMessage(SCI_INDICSETFORE,  IND_LINK, 0xff0000);
  fEditor->SendMessage(SCI_INDICSETSTYLE, IND_LINK, INDIC_PLAIN);
  
  
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
	fFileStatus = "";
	fLSPClientWrapper->UnregisterTextDocument(this);
	fLSPClientWrapper = NULL;
}

void	
FileWrapper::SetLSPClient(LSPClientWrapper* cW) {

	assert(cW);
	assert(!initialized);
	assert(fEditor);
	
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

	if (fEditor) {
		_RemoveAllDiagnostics();
		_RemoveAllDocumentLinks();
	}
  
  fLSPClientWrapper->DidClose(this, fFilenameURI.c_str());
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
  FromSciPositionToLSPPosition(start_pos, &range.start);
  FromSciPositionToLSPPosition(end_pos, &range.end);

  event.range = range;
  event.text.assign(text, len);

  std::vector<TextDocumentContentChangeEvent> changes{event};

  fLSPClientWrapper->DidChange(this, fFilenameURI.c_str(), changes, false);
  fLSPClientWrapper->DocumentLink(this, fFilenameURI.c_str());
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
    FromSciPositionToRange(s_start, s_end, &range);
    fLSPClientWrapper->RangeFomatting(this, fFilenameURI.c_str(), range);
  } else {
    fLSPClientWrapper->Formatting(this, fFilenameURI.c_str());
   }
}

void	
FileWrapper::GoTo(FileWrapper::GoToType type)
{
  if (!initialized || !fEditor)
    return;
  
  Position position;
  GetCurrentLSPPosition(&position);
  
  switch(type) {
	case GOTO_DEFINITION:
		fLSPClientWrapper->GoToDefinition(this, fFilenameURI.c_str(), position);
	break;
	case GOTO_DECLARATION:
		fLSPClientWrapper->GoToDeclaration(this, fFilenameURI.c_str(), position);
	break;
	case GOTO_IMPLEMENTATION:
		fLSPClientWrapper->GoToImplementation(this, fFilenameURI.c_str(), position);
	break;
  };
  
   
}


void FileWrapper::StartHover(Sci_Position sci_position) {
  if (!initialized)
		return;
		
  if (fEditor->SendMessage(SCI_INDICATORVALUEAT, IND_DIAG, sci_position) == 1)
  {
	  for (auto& ir: fLastDiagnostics) {			
		if (sci_position > ir.from && sci_position <= ir.to){
			_ShowToolTip(ir.info.c_str());
			break;
		}
	  }
	  return;
  }
  
  Position position;
  FromSciPositionToLSPPosition(sci_position, &position);
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
  GetCurrentLSPPosition(&position);

  fLSPClientWrapper->SignatureHelp(this, fFilenameURI.c_str(), position);
}


void FileWrapper::SwitchSourceHeader() {
  if (!initialized)
    return;
  fLSPClientWrapper->SwitchSourceHeader(this, fFilenameURI.c_str());
}

void FileWrapper::SelectedCompletion(const char *text) {
  if (!initialized || !fEditor)
    return;

  if (this->fCurrentCompletion != json({})) {

	for (auto& item: fCurrentCompletion) {

      if (item["label_sci"].get<std::string>().compare(std::string(text)) == 0) {


		TextEdit textEdit = item["textEdit"].get<TextEdit>();
		
		const Sci_Position s_pos = FromLSPPositionToSciPosition(&textEdit.range.start);
		const Sci_Position e_pos = FromLSPPositionToSciPosition(&textEdit.range.end);
		const Sci_Position   pos = fEditor->SendMessage(SCI_GETCURRENTPOS);
		Sci_Position cursorPos = e_pos;
        
        std::string textToAdd = textEdit.newText;
        
        // algo to remove the ${} stuff
        size_t dollarPos = textToAdd.find_first_of('$');
        
        if (dollarPos != std::string::npos) {
	        size_t lastPos = dollarPos;
			//single value case: check the case is $0
			if (dollarPos < textToAdd.length() - 1 && textToAdd.at(dollarPos+1) == '0')
			{
				lastPos += 2;
				
			} else {
				size_t endMarket = textToAdd.find_last_of('}');
				if (endMarket != std::string::npos)
					lastPos = endMarket + 1;
			}
			textToAdd.erase(dollarPos, lastPos - dollarPos);		
			
			cursorPos = s_pos + dollarPos;
	    } else {
			cursorPos = s_pos + textToAdd.length();
		}
	    
        fEditor->SendMessage(SCI_SETTARGETRANGE, s_pos, std::max(pos, std::max(e_pos, fCompletionPosition)));
        fEditor->SendMessage(SCI_REPLACETARGET, -1, (sptr_t) "");
        fEditor->SendMessage(SCI_INSERTTEXT, s_pos, (sptr_t)textToAdd.c_str());

        fEditor->SendMessage(SCI_SETCURRENTPOS, cursorPos, 0);
        fEditor->SendMessage(SCI_SETANCHOR,     cursorPos, 0);
        
        fEditor->SendMessage(SCI_SCROLLCARET, 0, 0);
        
        if (dollarPos!= std::string::npos && dollarPos > 0) {
	        char posChar = textToAdd.at(dollarPos-1);
			CharAdded(posChar);
        }
        
        break;
      }
    }
  }
  fEditor->SendMessage(SCI_AUTOCCANCEL, 0, 0);
  fCurrentCompletion = json({});
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
  GetCurrentLSPPosition(&position);
  CompletionContext context;

  this->fCompletionPosition = fEditor->SendMessage(SCI_GETCURRENTPOS, 0, 0);
  fLSPClientWrapper->Completion(this, fFilenameURI.c_str(), position, context);
}

// TODO move these, check if they are all used.. and move to a config section
// as these are c++/java parameters.. not sure they fit for all languages.

std::string calltipParametersEnd(")");
std::string calltipParametersStart("(");
std::string autoCompleteStartCharacters(".>");
std::string calltipWordCharacters("_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
std::string calltipParametersSeparators(",");



bool
FileWrapper::StartCallTip()
{
	// printf("StartCallTip\n");
	std::string line = GetCurrentLine();
	
	Position position;
	GetCurrentLSPPosition(&position);
	
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
	calltipPosition = FromLSPPositionToSciPosition(&newPos);
	
	line.at(current + 1) = '\0';
	
  fLSPClientWrapper->SignatureHelp(this, fFilenameURI.c_str(), position);
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
	std::string line = GetCurrentLine();
	Position position;
	GetCurrentLSPPosition(&position);
	
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

void	
FileWrapper::IndicatorClick(Sci_Position sci_position)
{
  Sci_Position s_start = fEditor->SendMessage(SCI_GETSELECTIONSTART, 0, 0);
  Sci_Position s_end = fEditor->SendMessage(SCI_GETSELECTIONEND, 0, 0);
  if (s_start != s_end)
	  return; 
	  

  if (fEditor->SendMessage(SCI_INDICATORVALUEAT, IND_LINK, sci_position) == 1)
  {
	  for (auto& ir: fLastDocumentLinks) {			
		if (sci_position > ir.from && sci_position <= ir.to){
			LogTrace("Opening file: [%s]", ir.info.c_str());
			OpenFileURI(ir.info);
			break;
		}
	  }
	  return;
  }
}

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

      OpenFileURI(uri, pos.line + 1, pos.character);
    }
};
  
void
FileWrapper::_DoSignatureHelp(json &result) {
	lastCalltip    = result["signatures"];
	currentCalltip = 0;
	if (lastCalltip[currentCalltip] != nlohmann::detail::value_t::null) {
		 const Sci_Position pos = fEditor->SendMessage(SCI_GETSELECTIONSTART, 0, 0);
		 auto str = lastCalltip[currentCalltip]["label"].get<std::string>();
		fEditor->SendMessage(SCI_CALLTIPSHOW, pos, (sptr_t)(str.c_str()));
	}
};

void	
FileWrapper::_DoSwitchSourceHeader(json &result) {
	std::string url = result.get<std::string>();
    OpenFileURI(url);
};

void	
FileWrapper::_DoCompletion(json &params) {
    auto items = params["items"];
    std::string list;
    for (auto& item: items) {
      std::string label = item["label"].get<std::string>();
      ltrim(label);
      if (list.length() > 0)
        list += "\n";
      list += label;
      item["label_sci"] = label;
    }
    
    if (list.length() > 0) {
      this->fCurrentCompletion = items;
      fEditor->SendMessage(SCI_AUTOCSETSEPARATOR, (int)'\n', 0);
      fEditor->SendMessage(SCI_AUTOCSETIGNORECASE, true);
      fEditor->SendMessage(SCI_AUTOCGETCANCELATSTART, false);
      fEditor->SendMessage(SCI_AUTOCSETORDER, SC_ORDER_CUSTOM, 0);      
      
      //whats' the text already selected so far?
      const Position start = items[0]["textEdit"]["range"]["start"].get<Position>();
      const Sci_Position s_pos = FromLSPPositionToSciPosition(&start);
	  Sci_Position len = fCompletionPosition - s_pos;
	  if (len <  0)
		  len = 0;
	  fEditor->SendMessage(SCI_AUTOCSHOW, len, (sptr_t)list.c_str());
    }
    
}

void
FileWrapper::_RemoveAllDiagnostics()
{
	// remove all the indicators..
	fEditor->SendMessage(SCI_SETINDICATORCURRENT, IND_DIAG);
	fEditor->SendMessage(SCI_INDICATORCLEARRANGE, 0, fEditor->SendMessage(SCI_GETTEXTLENGTH));
	fLastDiagnostics.clear();	
}

void	
FileWrapper::_DoDiagnostics(nlohmann::json& params)
{
	//auto dias = params["diagnostics"];
	auto vect = params["diagnostics"].get<std::vector<Diagnostic>>();
	
	_RemoveAllDiagnostics();

	for (auto &v: vect)
	{
		Range &r = v.range;
		InfoRange ir;
		ir.from = FromLSPPositionToSciPosition(&r.start);
		ir.to   = FromLSPPositionToSciPosition(&r.end);
		ir.info = v.message;
		
		LogTrace("Diagnostics [%ld->%ld] [%s]", ir.from, ir.to, ir.info.c_str());
		fEditor->SendMessage(SCI_INDICATORFILLRANGE, ir.from, ir.to-ir.from);
		fLastDiagnostics.push_back(ir);
	}
}

void
FileWrapper::_RemoveAllDocumentLinks()
{
	// remove all the indicators..
	fEditor->SendMessage(SCI_SETINDICATORCURRENT, IND_LINK);
	fEditor->SendMessage(SCI_INDICATORCLEARRANGE, 0, fEditor->SendMessage(SCI_GETTEXTLENGTH));
	fLastDocumentLinks.clear();
}

void 
FileWrapper::_DoDocumentLink(nlohmann::json& result)
{
	auto links = result.get<std::vector<DocumentLink>>();
	
	_RemoveAllDocumentLinks();

	for (auto &l: links) {
		Range &r = l.range;
		InfoRange ir;
		ir.from = FromLSPPositionToSciPosition(&r.start);
		ir.to   = FromLSPPositionToSciPosition(&r.end);
		ir.info = l.target;
		
		LogTrace("DocumentLink [%ld->%ld] [%s]", ir.from, ir.to, l.target.c_str());
		fEditor->SendMessage(SCI_INDICATORFILLRANGE, ir.from, ir.to-ir.from);
		fLastDocumentLinks.push_back(ir);
	}
}

		
void	
FileWrapper::_DoFileStatus(nlohmann::json& params)
{
	auto state = params["state"].get<std::string>();
	LogTrace("FileStatus [%s] [%s]",  GetFilenameURI().c_str(), state.c_str());
	fFileStatus = state.c_str();
}

void 
FileWrapper::onNotify(std::string id, value &result)
{
	IF_ID("textDocument/publishDiagnostics", _DoDiagnostics);
	IF_ID("textDocument/clangd.fileStatus",  _DoFileStatus);
	
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
	IF_ID("textDocument/documentLink", _DoDocumentLink);
	
	LogError("FileWrapper::onResponse not handled! [%s]", id.c_str());

}
void 
FileWrapper::onError(RequestID id, value &error)
{
	LogError("onError [%s] [%s]",  GetFilenameURI().c_str(), error.dump().c_str());
}
void 
FileWrapper::onRequest(std::string method, value &params, value &ID)
{
	//LogError("onRequest not implemented! [%s] [%s] [%s]", method.c_str(), params.dump().c_str(), ID.dump().c_str());
}

// utility
void 
FileWrapper::FromSciPositionToLSPPosition(const Sci_Position &pos,
                                  Position *lsp_position) {
	                                  
  lsp_position->line      = fEditor->SendMessage(SCI_LINEFROMPOSITION, pos, 0);
  Sci_Position end_pos   = fEditor->SendMessage(SCI_POSITIONFROMLINE, lsp_position->line, 0);
  lsp_position->character = fEditor->SendMessage(SCI_COUNTCHARACTERS, end_pos, pos);
}

Sci_Position 
FileWrapper::FromLSPPositionToSciPosition(const Position* lsp_position) {
	
  Sci_Position  sci_position;
  sci_position = fEditor->SendMessage(SCI_POSITIONFROMLINE, lsp_position->line, 0);
  sci_position = fEditor->SendMessage(SCI_POSITIONRELATIVE, sci_position, lsp_position->character);
  return sci_position;

}

void 
FileWrapper::GetCurrentLSPPosition(Position *lsp_position) {
  const Sci_Position pos = fEditor->SendMessage(SCI_GETSELECTIONSTART, 0, 0);
  FromSciPositionToLSPPosition(pos, lsp_position);
}

void 
FileWrapper::FromSciPositionToRange(Sci_Position s_start,
                            Sci_Position s_end, Range *range) {
  FromSciPositionToLSPPosition(s_start, &range->start);
  FromSciPositionToLSPPosition(s_end, &range->end);
}

Sci_Position 
FileWrapper::ApplyTextEdit(json &textEditJson) {

  TextEdit textEdit = textEditJson.get<TextEdit>();
  Range range = textEdit.range;
  Sci_Position s_pos = FromLSPPositionToSciPosition(&range.start);
  Sci_Position e_pos = FromLSPPositionToSciPosition(&range.end);

  fEditor->SendMessage(SCI_SETTARGETRANGE, s_pos, e_pos);
  
  Sci_Position replaced = fEditor->SendMessage(
      SCI_REPLACETARGET, -1,
      (sptr_t)(textEdit.newText.c_str()));

  return s_pos + replaced;
}

void 
FileWrapper::OpenFileURI(std::string uri, int32 line, int32 character) {
  if (uri.find("file://") == 0)
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
