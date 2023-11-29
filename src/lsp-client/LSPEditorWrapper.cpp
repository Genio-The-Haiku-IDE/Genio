/*
 * Copyright 2023, Andrea Anzani 
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "LSPEditorWrapper.h"

#include <Application.h>
#include <Path.h>
#include <Window.h>
#include <Catalog.h>

#include <cstdio>
#include <debugger.h>
#include <unistd.h>

#include "Editor.h"
#include "Log.h"
#include "LSPProjectWrapper.h"
#include "protocol.h"
#include "TextUtils.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Editor"

#define IF_ID(METHOD_NAME, METHOD) if (id.compare(METHOD_NAME) == 0) { METHOD(result); return; }
#define IND_DIAG 1 //Style for Problems
#define IND_LINK 2 //Style for Links

LSPEditorWrapper::LSPEditorWrapper(BPath filenamePath, Editor* editor)
	:
	LSPTextDocument(filenamePath, editor->FileType().c_str()),
	fEditor(editor),
	fToolTip(nullptr),
	fLSPProjectWrapper(nullptr)
{
	assert(fEditor);
}


void
LSPEditorWrapper::ApplySettings()
{
	fEditor->SendMessage(SCI_INDICSETFORE,  IND_DIAG, (255 | (0 << 8) | (0 << 16)));
	fEditor->SendMessage(SCI_INDICSETSTYLE, IND_DIAG, INDIC_SQUIGGLE);
	fEditor->SendMessage(SCI_INDICSETALPHA, IND_DIAG, 100);

	fEditor->SendMessage(SCI_INDICSETFORE,  IND_LINK, 0xff0000);
	fEditor->SendMessage(SCI_INDICSETSTYLE, IND_LINK, INDIC_PLAIN);
	fEditor->SendMessage(SCI_INDICSETALPHA, IND_LINK, 100);

	fEditor->SendMessage(SCI_SETMOUSEDWELLTIME, 1000);

	// int margins = fEditor->SendMessage(SCI_GETMARGINS);
	// fEditor->SendMessage(SCI_SETMARGINS, margins + 1);
	// fEditor->SendMessage(SCI_SETMARGINTYPEN, margins, SC_MARGIN_SYMBOL);
	// fEditor->SendMessage(SCI_SETMARGINWIDTHN,margins, 16);
	// fEditor->SendMessage(SCI_SETMARGINMASKN, margins, 1 << 2);
	// fEditor->SendMessage(SCI_MARKERSETBACK, margins, kMarkerForeColor);
	// fEditor->SendMessage(SCI_MARKERADD, 1, 2);
}


void
LSPEditorWrapper::UnsetLSPServer()
{
	if (!fLSPProjectWrapper)
		return;

	didClose();
	fFileStatus = "";
	fLSPProjectWrapper->UnregisterTextDocument(this);
	fLSPProjectWrapper = nullptr;
}


bool
LSPEditorWrapper::HasLSPServer()
{
	return (fLSPProjectWrapper != nullptr);
}

bool
LSPEditorWrapper::HasLSPServerCapability(const LSPCapability cap)
{
	return (HasLSPServer() && fLSPProjectWrapper->HasCapability(cap));
}

void
LSPEditorWrapper::ApplyFix(BMessage* info)
{
	if (!HasLSPServer())
		return;

	if (!info->GetBool("quickFix", false))
		return;

	int32 diaIndex = info->GetInt32("index", -1);
	if (diaIndex >= 0 && fLastDiagnostics.size() > (size_t)diaIndex) {
		//how are we sure the fix list is syncronized? (TODO: how?)
		std::map<std::string, std::vector<TextEdit>> map =
			fLastDiagnostics.at(diaIndex).diagnostic.codeActions.value()[0].edit.value().changes.value();
		for (auto& ed : map){
			if (GetFilenameURI().ICompare(ed.first.c_str()) == 0) {
				for (TextEdit& te : ed.second) {
					ApplyTextEdit(te);
				}
			}
		}
	}
}


void
LSPEditorWrapper::SetLSPServer(LSPProjectWrapper* cW) {

	assert(cW);
	assert(!fLSPProjectWrapper);
	assert(fEditor);

	SetFileType(fEditor->FileType().c_str());

	if (cW->RegisterTextDocument(this)) {
		fLSPProjectWrapper = cW;
		didOpen();
	}
}


void
LSPEditorWrapper::didOpen()
{
	if (!fLSPProjectWrapper)
		return;
	const char* text = (const char*) fEditor->SendMessage(SCI_GETCHARACTERPOINTER);

	fLSPProjectWrapper->DidOpen(this, text, FileType().String());
	// fLSPProjectWrapper->Sync();
}


void
LSPEditorWrapper::didClose()
{
	if (!fLSPProjectWrapper)
		return;

	if (fEditor) {
		_RemoveAllDiagnostics();
		_RemoveAllDocumentLinks();
	}

	fLSPProjectWrapper->DidClose(this);
}


void
LSPEditorWrapper::didSave()
{
	if (!fLSPProjectWrapper)
		return;

	fLSPProjectWrapper->DidSave(this);
}


void
LSPEditorWrapper::didChange(
	const char* text, long len, Sci_Position start_pos, Sci_Position poslength)
{
	if (!fLSPProjectWrapper || !fEditor)
		return;

	Sci_Position end_pos = fEditor->SendMessage(SCI_POSITIONRELATIVE, start_pos, poslength);

	TextDocumentContentChangeEvent event;
	Range range;
	FromSciPositionToLSPPosition(start_pos, &range.start);
	FromSciPositionToLSPPosition(end_pos, &range.end);

	event.range = range;
	event.text.assign(text, len);

	std::vector<TextDocumentContentChangeEvent> changes{event};

	fLSPProjectWrapper->DidChange(this, changes, false);
}


void
LSPEditorWrapper::_DoFormat(json& params)
{
	fEditor->SendMessage(SCI_BEGINUNDOACTION, 0, 0);
	auto items = params;
	for (json::reverse_iterator it = items.rbegin(); it != items.rend(); ++it) {
		ApplyTextEdit((*it));
	}
	fEditor->SendMessage(SCI_ENDUNDOACTION, 0, 0);
}


void
LSPEditorWrapper::Format()
{
	if (!fLSPProjectWrapper || !fEditor)
		return;

	// format a range or format the whole doc?
	Sci_Position s_start = fEditor->SendMessage(SCI_GETSELECTIONSTART, 0, 0);
	Sci_Position s_end = fEditor->SendMessage(SCI_GETSELECTIONEND, 0, 0);

	if (s_start < s_end) {
		Range range;
		FromSciPositionToRange(s_start, s_end, &range);
		fLSPProjectWrapper->RangeFomatting(this, range);
	} else {
		fLSPProjectWrapper->Formatting(this);
	}
}


void
LSPEditorWrapper::GoTo(LSPEditorWrapper::GoToType type)
{
	if (!fLSPProjectWrapper || !fEditor || !IsStatusValid())
		return;

	Position position;
	GetCurrentLSPPosition(&position);

	switch (type) {
		case GOTO_DEFINITION:
			fLSPProjectWrapper->GoToDefinition(this, position);
			break;
		case GOTO_DECLARATION:
			fLSPProjectWrapper->GoToDeclaration(this, position);
			break;
		case GOTO_IMPLEMENTATION:
			fLSPProjectWrapper->GoToImplementation(this, position);
			break;
	}
}


void
LSPEditorWrapper::StartHover(Sci_Position sci_position)
{
	if (!fLSPProjectWrapper || sci_position < 0 || !IsStatusValid()) {
		return;
	}

	LSPDiagnostic dia;
	if (DiagnosticFromPosition(sci_position, dia) > -1) {
		_ShowToolTip(dia.range.info.c_str());
		return;
	}

	Position position;
	FromSciPositionToLSPPosition(sci_position, &position);
	fLSPProjectWrapper->Hover(this, position);
}

int32
LSPEditorWrapper::DiagnosticFromPosition(Sci_Position sci_position, LSPDiagnostic& dia)
{
	int32 index = -1;
	if (fEditor->SendMessage(SCI_INDICATORVALUEAT, IND_DIAG, sci_position) == 1) {
		for (auto& ir : fLastDiagnostics) {
			index++;
			if (sci_position > ir.range.from && sci_position <= ir.range.to) {
				dia = ir;
				return index;
			}
		}
	}
	return index;
}


void
LSPEditorWrapper::EndHover()
{
	if (!fLSPProjectWrapper)
		return;
	BAutolock l(fEditor->Looper());
	if (l.IsLocked()) {
		fEditor->HideToolTip();
	}
}


void
LSPEditorWrapper::SwitchSourceHeader()
{
	if (!fLSPProjectWrapper || !IsStatusValid())
		return;
	fLSPProjectWrapper->SwitchSourceHeader(this);
}


void
LSPEditorWrapper::SelectedCompletion(const char* text)
{
	if (!fLSPProjectWrapper || !fEditor)
		return;

	if (fCurrentCompletion.items.size() > 0) {
		for (auto& item : fCurrentCompletion.items) {
			if (item.label.compare(std::string(text)) == 0) {
				TextEdit textEdit = item.textEdit;

				const Sci_Position s_pos = FromLSPPositionToSciPosition(&textEdit.range.start);
				const Sci_Position e_pos = FromLSPPositionToSciPosition(&textEdit.range.end);
				const Sci_Position pos = fEditor->SendMessage(SCI_GETCURRENTPOS);
				Sci_Position cursorPos = e_pos;

				std::string textToAdd = textEdit.newText;

				// algo to remove the ${} stuff
				size_t dollarPos = textToAdd.find_first_of('$');

				if (dollarPos != std::string::npos) {
					size_t lastPos = dollarPos;
					// single value case: check the case is $0
					if (dollarPos < textToAdd.length() - 1 && textToAdd.at(dollarPos + 1) == '0') {
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

				fEditor->SendMessage(
					SCI_SETTARGETRANGE, s_pos, std::max(pos, std::max(e_pos, fCompletionPosition)));
				fEditor->SendMessage(SCI_REPLACETARGET, -1, (sptr_t) "");
				fEditor->SendMessage(SCI_INSERTTEXT, s_pos, (sptr_t) textToAdd.c_str());

				fEditor->SendMessage(SCI_SETCURRENTPOS, cursorPos, 0);
				fEditor->SendMessage(SCI_SETANCHOR, cursorPos, 0);

				fEditor->SendMessage(SCI_SCROLLCARET, 0, 0);

				if (dollarPos != std::string::npos && dollarPos > 0) {
					char posChar = textToAdd.at(dollarPos - 1);
					CharAdded(posChar);
				}

				break;
			}
		}
	}
	fEditor->SendMessage(SCI_AUTOCCANCEL, 0, 0);
	fCurrentCompletion = CompletionList();
}


void
LSPEditorWrapper::StartCompletion()
{
	if (!fLSPProjectWrapper || !fEditor || !IsStatusValid())
		return;

	// let's check if a completion is ongoing
	if (fCurrentCompletion.items.size() > 0) {
		// let's close the current Scintilla listbox
		fEditor->SendMessage(SCI_AUTOCCANCEL, 0, 0);
		// let's cancel any previous request running on clangd
		// --> TODO: cancel previous clangd request!

		// let's clean-up current request details:
		this->fCurrentCompletion = CompletionList();
	}

	Position position;
	GetCurrentLSPPosition(&position);
	CompletionContext context;

	fCompletionPosition = fEditor->SendMessage(SCI_GETCURRENTPOS, 0, 0);
	fLSPProjectWrapper->Completion(this, position, context);
}

// TODO move these, check if they are all used.. and move to a config section
// as these are c++/java parameters.. not sure they fit for all languages.

const std::string kCalltipParametersEnd(")");
const std::string kCalltipParametersStart("(");
const std::string kCalltipWordCharacters("_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
const std::string kCalltipParametersSeparators(",");

bool
LSPEditorWrapper::StartCallTip(bool searchStart)
{
	Sci_Position pos = fEditor->SendMessage(SCI_GETCURRENTPOS);
	fCalltipStartPosition = pos;
	fCalltipPosition = pos;

	Position position;
	FromSciPositionToLSPPosition(pos, &position);

	if (searchStart) {
		std::string line = GetCurrentLine();
		Sci_Position current = position.character;
		do {
			int braces = 0;
			while (
				current > 0 && (braces || !Contains(kCalltipParametersStart, line[current - 1]))) {
				if (Contains(kCalltipParametersStart, line[current - 1]))
					braces--;
				else if (Contains(kCalltipParametersEnd, line[current - 1]))
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
		} while (current > 0 && !Contains(kCalltipWordCharacters, line[current - 1]));

		if (current <= 0)
			return false;

		fStartCalltipWord = current - 1;
		while (
			fStartCalltipWord > 0 && Contains(kCalltipWordCharacters, line[fStartCalltipWord - 1])) {
			fStartCalltipWord--;
		}

		Position newPos;
		newPos.line = position.line;
		newPos.character = fStartCalltipWord;
		fCalltipPosition = FromLSPPositionToSciPosition(&newPos);
	}

	fLSPProjectWrapper->SignatureHelp(this, position);

	return true;
}


void
LSPEditorWrapper::UpdateCallTip(int deltaPos)
{
	LogTraceF("BEFORE currentCalltip %d", fCurrentCalltip);
	if (deltaPos == 1 && fCurrentCalltip > 0 && fCurrentCalltip + 1)
		fCurrentCalltip--;
	else if (deltaPos == 2 && fCurrentCalltip + 1 < fMaxCalltip)
		fCurrentCalltip++;

	LogTraceF("AFTER currentCalltip %d", fCurrentCalltip);

	fFunctionDefinition = "";

	if (fMaxCalltip > 1)
		fFunctionDefinition += "\001 " + std::to_string(fCurrentCalltip + 1) + " of "
			+ std::to_string(fMaxCalltip) + " \002";

	fFunctionDefinition += fLastCalltip.signatures[fCurrentCalltip].label;

	LogTraceF("functionDefinition %s", fFunctionDefinition.c_str());

	Sci_Position hackPos = fEditor->SendMessage(SCI_GETCURRENTPOS);
	fEditor->SendMessage(SCI_SETCURRENTPOS, fCalltipStartPosition);
	fEditor->SendMessage(SCI_CALLTIPSHOW, fCalltipPosition, (sptr_t) (fFunctionDefinition.c_str()));
	fEditor->SendMessage(SCI_SETCURRENTPOS, hackPos);

	ContinueCallTip();
}


void
LSPEditorWrapper::ContinueCallTip()
{
	std::string line = GetCurrentLine();
	Position position;
	GetCurrentLSPPosition(&position);

	Sci_Position current = position.character;

	int braces = 0;
	size_t commas = 0;
	for (Sci_Position i = fStartCalltipWord; i < current; i++) {
		if (Contains(kCalltipParametersStart, line[i]))
			braces++;
		else if (Contains(kCalltipParametersEnd, line[i]) && braces > 0)
			braces--;
		else if (braces == 1 && Contains(kCalltipParametersSeparators, line[i]))
			commas++;
	}

	// if the num of commas is not compatible with current calltip
	// try to find a better one..
    if (fLastCalltip.signatures.size() <= 0)
		return;

	auto params = fLastCalltip.signatures[fCurrentCalltip].parameters;
	// printf("1) DEBUG:%s %ld, %ld\n", params.dump().c_str(), params.size(), commas);
	if (commas > params.size()) {
		for (int i = 0; i < fMaxCalltip; i++) {
			if (commas < fLastCalltip.signatures[i].parameters.size()) {
				fCurrentCalltip = i;
				UpdateCallTip(0);
				return;
			}
		}
	}

	if (params.size() > commas) {
		// printf("2) DEBUG:%s\n", params.dump().c_str());
		int base = fFunctionDefinition.find("\002", 0);
		int start = base + params[commas].labelOffsets.first;
		int end = base + params[commas].labelOffsets.second;
		// printf("DEBUG:%s %d,%d\n", params.dump().c_str(), start, end);
		fEditor->SendMessage(SCI_CALLTIPSETHLT, start + 1, end + 1);
	} else if (commas == 0 && params.size() == 0) {
		// printf("3) DEBUG: reset\n");
		fEditor->SendMessage(SCI_CALLTIPSETHLT, 1, 1);
	}
}


void
LSPEditorWrapper::CharAdded(const char ch /*utf-8?*/)
{
	// printf("on char %c\n", ch);
	if (!fLSPProjectWrapper || !fEditor)
		return;

	/* algo took from SciTE editor .. */
	const Sci_Position selStart = fEditor->SendMessage(SCI_GETSELECTIONSTART);
	const Sci_Position selEnd = fEditor->SendMessage(SCI_GETSELECTIONEND);
	if ((selEnd == selStart) && (selStart > 0)) {
		if (fEditor->SendMessage(SCI_CALLTIPACTIVE)) {
			if (Contains(kCalltipParametersEnd, ch)) {
				fBraceCount--;
				if (fBraceCount < 1)
					fEditor->SendMessage(SCI_CALLTIPCANCEL);
				else
					StartCallTip(true);
			} else if (Contains(kCalltipParametersStart, ch)) {
				fBraceCount++;
				StartCallTip(true);
			} else {
				ContinueCallTip();
			}
		} else if (fEditor->SendMessage(SCI_AUTOCACTIVE)) {
			if (Contains(kCalltipParametersStart, ch)) {
				fBraceCount++;
				StartCallTip(true);
			} else if (Contains(kCalltipParametersEnd, ch)) {
				fBraceCount--;
			} else if (!Contains(wordCharacters, ch)) {
				fEditor->SendMessage(SCI_AUTOCCANCEL);
				if (Contains(fLSPProjectWrapper->triggerCharacters(), ch)) {
					StartCompletion();
				}
			}
		} else {
			if (fLSPProjectWrapper->HasCapability(kLCapSignatureHelp) &&
				Contains(kCalltipParametersStart, ch)) {
				fBraceCount = 1;
				StartCallTip(true);
			} else {
				if (fLSPProjectWrapper->HasCapability(kLCapCompletion) &&
				    Contains(fLSPProjectWrapper->triggerCharacters(), ch)) {

					StartCompletion();

				}
			}
		}
	}
}


void
LSPEditorWrapper::IndicatorClick(Sci_Position sci_position)
{
	Sci_Position s_start = fEditor->SendMessage(SCI_GETSELECTIONSTART, 0, 0);
	Sci_Position s_end = fEditor->SendMessage(SCI_GETSELECTIONEND, 0, 0);
	if (s_start != s_end)
		return;

	if (fEditor->SendMessage(SCI_INDICATORVALUEAT, IND_LINK, sci_position) == 1) {
		for (auto& ir : fLastDocumentLinks) {
			if (sci_position > ir.from && sci_position <= ir.to) {
				LogTrace("Opening file: [%s]", ir.info.c_str());
				OpenFileURI(ir.info);
				break;
			}
		}
	}
}


void
LSPEditorWrapper::_ShowToolTip(const char* text)
{
	if (!fToolTip)
		fToolTip = new BTextToolTip(text);

	fToolTip->SetText(text);
	if (fEditor->Looper()->Lock()) {
		fEditor->ShowToolTip(fToolTip);
		fEditor->Looper()->Unlock();
	}
}


void
LSPEditorWrapper::_DoHover(nlohmann::json& result)
{
	if (result == nlohmann::detail::value_t::null) {
		EndHover();
		return;
	}

	std::string tip = result["contents"]["value"].get<std::string>();

	_ShowToolTip(tip.c_str());
}


void
LSPEditorWrapper::_DoGoTo(nlohmann::json& items)
{
	if (!items.empty()) {
		// TODO if more than one match??
		auto first = items[0];
		std::string uri = first["uri"].get<std::string>();

		Position pos = first["range"]["start"].get<Position>();

		OpenFileURI(uri, pos.line + 1, pos.character);
	}
}


void
LSPEditorWrapper::_DoSignatureHelp(json& result)
{
	fLastCalltip = result.get<SignatureHelp>(); // result["signatures"];
	fCurrentCalltip = 0;
	fMaxCalltip = 0;

	if (fCurrentCalltip < (int)fLastCalltip.signatures.size()) {
		fMaxCalltip = fLastCalltip.signatures.size();
		UpdateCallTip(0);
	}
}


void
LSPEditorWrapper::_DoSwitchSourceHeader(json& result)
{
	std::string url = result.get<std::string>();
	OpenFileURI(url);
}


void
LSPEditorWrapper::_DoCompletion(json& params)
{
	CompletionList allItems = params.get<CompletionList>();
	auto items = allItems.items;
	std::string list;
	for (auto& item : items) {
		std::string label = item.label;
		LeftTrim(label);
		if (list.length() > 0)
			list += "\n";
		list += label;
		item.label = label;
	}

	if (list.length() > 0) {
		fCurrentCompletion = allItems;
		fEditor->SendMessage(SCI_AUTOCSETSEPARATOR, (int) '\n', 0);
		fEditor->SendMessage(SCI_AUTOCSETIGNORECASE, true);
		fEditor->SendMessage(SCI_AUTOCGETCANCELATSTART, false);
		fEditor->SendMessage(SCI_AUTOCSETORDER, SC_ORDER_CUSTOM, 0);

		// whats' the text already selected so far?
		const Position start = fCurrentCompletion.items[0].textEdit.range.start;
		const Sci_Position s_pos = FromLSPPositionToSciPosition(&start);
		Sci_Position len = fCompletionPosition - s_pos;
		if (len < 0)
			len = 0;
		fEditor->SendMessage(SCI_AUTOCSHOW, len, (sptr_t) list.c_str());
	}
}


void
LSPEditorWrapper::_RemoveAllDiagnostics()
{
	// remove all the indicators..
	fEditor->SendMessage(SCI_SETINDICATORCURRENT, IND_DIAG);
	fEditor->SendMessage(SCI_INDICATORCLEARRANGE, 0, fEditor->SendMessage(SCI_GETTEXTLENGTH));
	fLastDiagnostics.clear();
}

#include "GMessage.h"
void
LSPEditorWrapper::_DoDiagnostics(nlohmann::json& params)
{
	auto vect = params["diagnostics"].get<std::vector<Diagnostic>>();

	_RemoveAllDiagnostics();

	BMessage toJson('diag');
	int32 index = 0;
	for (auto& v : vect) {
		LSPDiagnostic lspDiag;

		Range& r = v.range;
		InfoRange& ir = lspDiag.range;
		ir.from = FromLSPPositionToSciPosition(&r.start);
		ir.to = FromLSPPositionToSciPosition(&r.end);
		ir.info = v.message;

		lspDiag.diagnostic = v;

		LogTrace("Diagnostics [%ld->%ld] [%s]", ir.from, ir.to, ir.info.c_str());
		fEditor->SendMessage(SCI_INDICATORFILLRANGE, ir.from, ir.to - ir.from);

		GMessage dia;
		dia["category"] = v.category.value().c_str();
		dia["message"] = v.message.c_str();
		dia["source"] = v.source.c_str();
		dia["index"] = index++;
		dia["be:line"] = v.range.start.line + 1;
		dia["lsp:character"] = v.range.start.character;
		dia["quickFix"] = false;
		dia["title"] = B_TRANSLATE("No fix available");
		if (v.codeActions.value().size() > 0) {
			if (v.codeActions.value()[0].edit.has()){
				dia["quickFix"] = true;
				BString str(B_TRANSLATE("Fix: %fix_desc%"));
				str.ReplaceAll("%fix_desc%", v.codeActions.value()[0].title.c_str());
				dia["title"] = str.String();
			}
		}
		lspDiag.fixTitle = (const char*)dia["title"];
		toJson.AddMessage("diagnostic", &dia);
		fLastDiagnostics.push_back(lspDiag);
	}

	if (fEditor->LockLooper()) {
		fEditor->SetProblems(&toJson);
		fEditor->UnlockLooper();
	}

	if (fLSPProjectWrapper)
		fLSPProjectWrapper->DocumentLink(this);
}


void
LSPEditorWrapper::_RemoveAllDocumentLinks()
{
	// remove all the indicators..
	fEditor->SendMessage(SCI_SETINDICATORCURRENT, IND_LINK);
	fEditor->SendMessage(SCI_INDICATORCLEARRANGE, 0, fEditor->SendMessage(SCI_GETTEXTLENGTH));
	fLastDocumentLinks.clear();
}


void
LSPEditorWrapper::_DoDocumentLink(nlohmann::json& result)
{
	auto links = result.get<std::vector<DocumentLink>>();

	_RemoveAllDocumentLinks();

	for (auto& l : links) {
		Range& r = l.range;
		InfoRange ir;
		ir.from = FromLSPPositionToSciPosition(&r.start);
		ir.to = FromLSPPositionToSciPosition(&r.end);
		ir.info = l.target;

		LogTrace("DocumentLink [%ld->%ld] [%s]", ir.from, ir.to, l.target.c_str());
		fEditor->SendMessage(SCI_INDICATORFILLRANGE, ir.from, ir.to - ir.from);
		fLastDocumentLinks.push_back(ir);
	}
}


void
LSPEditorWrapper::_DoFileStatus(nlohmann::json& params)
{
	auto state = params["state"].get<std::string>();
	LogInfo("FileStatus [%s] -> [%s]", GetFileStatus().String(), state.c_str());
	SetFileStatus(state.c_str());
}


bool
LSPEditorWrapper::IsStatusValid()
{
	BString status = GetFileStatus();
	bool value = status.IsEmpty() || (status.Compare("idle") == 0);
	if (!value)
		LogDebugF("Invalid status (%d) for [%s] (%s)", value, GetFilenameURI().String(),
			GetFileStatus().String());
	return value;
}


void
LSPEditorWrapper::onNotify(std::string id, value& result)
{
	IF_ID("textDocument/publishDiagnostics", _DoDiagnostics);
	IF_ID("textDocument/clangd.fileStatus", _DoFileStatus);

	LogError("LSPEditorWrapper::onNotify not handled! [%s]", id.c_str());
}


void
LSPEditorWrapper::onResponse(RequestID id, value& result)
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

	LogError("LSPEditorWrapper::onResponse not handled! [%s]", id.c_str());
}


void
LSPEditorWrapper::onError(RequestID id, value& error)
{
	LogError("onError [%s] [%s]", GetFileStatus().String(), error.dump().c_str());
}


void
LSPEditorWrapper::onRequest(std::string method, value& params, value& ID)
{
	// LogError("onRequest not implemented! [%s] [%s] [%s]", method.c_str(), params.dump().c_str(),
	// ID.dump().c_str());
}


// utility
void
LSPEditorWrapper::FromSciPositionToLSPPosition(const Sci_Position& pos, Position* lsp_position)
{
	lsp_position->line = fEditor->SendMessage(SCI_LINEFROMPOSITION, pos, 0);
	Sci_Position end_pos = fEditor->SendMessage(SCI_POSITIONFROMLINE, lsp_position->line, 0);
	lsp_position->character = fEditor->SendMessage(SCI_COUNTCHARACTERS, end_pos, pos);
}


Sci_Position
LSPEditorWrapper::FromLSPPositionToSciPosition(const Position* lsp_position)
{
	Sci_Position sci_position;
	sci_position = fEditor->SendMessage(SCI_POSITIONFROMLINE, lsp_position->line, 0);
	sci_position
		= fEditor->SendMessage(SCI_POSITIONRELATIVE, sci_position, lsp_position->character);
	return sci_position;
}


void
LSPEditorWrapper::GetCurrentLSPPosition(Position* lsp_position)
{
	const Sci_Position pos = fEditor->SendMessage(SCI_GETSELECTIONSTART, 0, 0);
	FromSciPositionToLSPPosition(pos, lsp_position);
}


void
LSPEditorWrapper::FromSciPositionToRange(Sci_Position s_start, Sci_Position s_end, Range* range)
{
	FromSciPositionToLSPPosition(s_start, &range->start);
	FromSciPositionToLSPPosition(s_end, &range->end);
}


Sci_Position
LSPEditorWrapper::ApplyTextEdit(json& textEditJson)
{
	TextEdit textEdit = textEditJson.get<TextEdit>();
	return ApplyTextEdit(textEdit);
}

Sci_Position
LSPEditorWrapper::ApplyTextEdit(TextEdit &textEdit)
{
	Range range = textEdit.range;
	Sci_Position s_pos = FromLSPPositionToSciPosition(&range.start);
	Sci_Position e_pos = FromLSPPositionToSciPosition(&range.end);

	fEditor->SendMessage(SCI_SETTARGETRANGE, s_pos, e_pos);

	Sci_Position replaced
		= fEditor->SendMessage(SCI_REPLACETARGET, -1, (sptr_t) (textEdit.newText.c_str()));

	return s_pos + replaced;
}

void
LSPEditorWrapper::OpenFileURI(std::string uri, int32 line, int32 character)
{
	BUrl url(uri.c_str());
	if (url.IsValid() && url.HasPath()) {
		BEntry entry(url.Path().String());
		entry_ref ref;
		if (entry.Exists()) {
			BMessage refs(B_REFS_RECEIVED);
			if (entry.GetRef(&ref) == B_OK) {
				refs.AddRef("refs", &ref);
				if (line != -1) {
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
LSPEditorWrapper::GetCurrentLine()
{
	const Sci_Position len = fEditor->SendMessage(SCI_GETCURLINE, 0, (sptr_t) nullptr);
	std::string text(len, '\0');
	fEditor->SendMessage(SCI_GETCURLINE, len, (sptr_t) (&text[0]));
	return text;
}

// end - utility
