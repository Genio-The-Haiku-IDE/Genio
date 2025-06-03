/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * Copyright 2024, Nexus6 <nexus6.haiku@icloud.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "LSPEditorWrapper.h"

#include <Application.h>
#include <Path.h>
#include <Catalog.h>
#include <Window.h>

#include <algorithm>
#include <cstdio>
#include <debugger.h>
#include <unistd.h>

#include "Editor.h"
#include "EditorStatusView.h"
#include "Log.h"
#include "LSPProjectWrapper.h"
#include "JumpNavigator.h"
#include "protocol.h"
#include "TextUtils.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Editor"


#define IF_ID(METHOD_NAME, METHOD) if (id.compare(METHOD_NAME) == 0) { METHOD(result); return; }
#define IND_DIAG INDICATOR_CONTAINER + 1 //Style for Problems
#define IND_LINK INDICATOR_CONTAINER + 2 //Style for Links
#define IND_OVER INDICATOR_CONTAINER + 3 //Style for mouse hover

LSPEditorWrapper::LSPEditorWrapper(BPath filenamePath, Editor* editor)
	:
	LSPTextDocument(filenamePath, editor->FileType().c_str()),
	fEditor(editor),
	fToolTip(nullptr),
	fLSPProjectWrapper(nullptr),
	fCallTip(editor),
	fInitialized(false),
	fLastWordStartPosition(-1),
	fLastWordEndPosition(-1)
{
	assert(fEditor);
}


void
LSPEditorWrapper::ApplySettings()
{
	fEditor->SendMessage(SCI_INDICSETFORE,  IND_DIAG, (255 | (0 << 8) | (0 << 16)));
	fEditor->SendMessage(SCI_INDICSETSTYLE, IND_DIAG, INDIC_SQUIGGLE);

#ifdef DOCUMENT_LINK
	fEditor->SendMessage(SCI_INDICSETFORE,  IND_LINK, 0xff0000);
	fEditor->SendMessage(SCI_INDICSETSTYLE, IND_LINK, INDIC_PLAIN);
#endif

	fEditor->SendMessage(SCI_INDICSETFORE,  IND_OVER, 0xff0000);
	fEditor->SendMessage(SCI_INDICSETSTYLE, IND_OVER, INDIC_PLAIN);

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
	if (fLSPProjectWrapper == nullptr)
		return;

	didClose();
	fFileStatus = "";
	fLSPProjectWrapper->UnregisterTextDocument(this);
	fLSPProjectWrapper = nullptr;
}


bool
LSPEditorWrapper::HasLSPServer()
{
	return fLSPProjectWrapper != nullptr;
}


bool
LSPEditorWrapper::HasLSPServerCapability(const LSPCapability cap)
{
	return HasLSPServer() && fLSPProjectWrapper->HasCapability(cap);
}


void
LSPEditorWrapper::ApplyFix(BMessage* info)
{
	if (!HasLSPServer())
		return;

	if (!info->GetBool("quickFix", false))
		return;

	int32 diaIndex = info->GetInt32("index", -1);
	int32 actIndex = info->GetInt32("action", -1);
	if (diaIndex >= 0 && fLastDiagnostics.size() > (size_t)diaIndex) {
		std::map<std::string, std::vector<TextEdit>> map =
			fLastDiagnostics.at(diaIndex).diagnostic.codeActions.value()[actIndex].edit.value().changes.value();
		for (auto& ed : map){
			if (GetFilenameURI().ICompare(ed.first.c_str()) == 0) {
				json edits = ed.second;
				_DoFormat(edits);
			}
		}
	}
}


void
LSPEditorWrapper::ApplyEdit(std::string info)
{
	if (!HasLSPServer())
		return;

	json items = nlohmann::json::parse(info);
	_DoFormat(items);
}


void
LSPEditorWrapper::SetLSPServer(LSPProjectWrapper* cW) {

	assert(cW);
	assert(!fLSPProjectWrapper);
	assert(fEditor);

	SetFileType(fEditor->FileType().c_str());

	fLSPProjectWrapper = cW;
	if (!cW->RegisterTextDocument(this)) {
		fLSPProjectWrapper = nullptr;
	}
}


bool
LSPEditorWrapper::IsInitialized()
{
	return fInitialized && fLSPProjectWrapper != nullptr;
}


void
LSPEditorWrapper::didOpen()
{
	if (!IsInitialized())
		return;

	const char* text = (const char*) fEditor->SendMessage(SCI_GETCHARACTERPOINTER);

	fLSPProjectWrapper->DidOpen(this, text, FileType().String());
	// fLSPProjectWrapper->Sync();
}


void
LSPEditorWrapper::didClose()
{
	if (!IsInitialized())
		return;

	flushChanges();

	if (fEditor) {
		_RemoveAllDiagnostics();
		_RemoveAllDocumentLinks();
	}

	fLSPProjectWrapper->DidClose(this);
}


void
LSPEditorWrapper::didSave()
{
	if (!IsInitialized())
		return;

	flushChanges();
	fLSPProjectWrapper->DidSave(this);
}


void
LSPEditorWrapper::didChange(
	const char* text, long len, Sci_Position start_pos, Sci_Position poslength)
{
	if (!IsInitialized() || fEditor == nullptr)
		return;

	Sci_Position end_pos = fEditor->SendMessage(SCI_POSITIONRELATIVE, start_pos, poslength);

	TextDocumentContentChangeEvent event;
	Range range;
	FromSciPositionToLSPPosition(start_pos, &range.start);
	FromSciPositionToLSPPosition(end_pos, &range.end);

	event.range = range;
	event.text.assign(text, len);

	fChanges.push_back(event);
}


void
LSPEditorWrapper::flushChanges()
{
	if (fChanges.size() > 0) {
		fLSPProjectWrapper->DidChange(this, fChanges, false);
		fChanges.clear();
	}
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

	// Invalidate the current diagnostics and links to avoid any race-condition
	// between the list of fixes and the updated document.
	// a new list will arrive from LSP in few instants!
	_RemoveAllDiagnostics();
	_RemoveAllDocumentLinks();
}


void
LSPEditorWrapper::_DoRename(json& params)
{
	BMessage bjson;
	if (!params["changes"].is_null()) {
		for (auto& [uri, edits] : params["changes"].items()) {
			for (auto& e: edits) {
				auto edit = e.get<TextEdit>();
				edit.range.start.line += 1;
				edit.range.end.line += 1;
			}
			OpenFileURI(uri, -1, -1, edits.dump().c_str());
		}
	} else if (params["documentChanges"].is_array()) {
		for (auto& block : params["documentChanges"]) {
			std::string uri = block["textDocument"]["uri"].get<std::string>();
			OpenFileURI(uri, -1, -1, block["edits"].dump().c_str());
		}
	}
}


void
LSPEditorWrapper::Format()
{
	if (!IsInitialized() || fEditor == nullptr)
		return;

	flushChanges();

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
	if (!IsInitialized()|| !fEditor || !IsStatusValid())
		return;

	flushChanges();

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
LSPEditorWrapper::Rename(std::string newName)
{
	if (!IsInitialized()|| !fEditor || !IsStatusValid())
		return;

	flushChanges();

	Position position;
	GetCurrentLSPPosition(&position);
	fLSPProjectWrapper->Rename(this, position, newName);
}


void
LSPEditorWrapper::StartHover(Sci_Position sci_position)
{
	if (!IsInitialized() || sci_position < 0 || !IsStatusValid()) {
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


void
LSPEditorWrapper::MouseMoved(BMessage* message)
{
	if (!IsInitialized()) {
		return;
	}

	if (fLastWordEndPosition > -1 && fLastWordStartPosition > -1) {
		fEditor->SendMessage(SCI_SETINDICATORCURRENT, IND_OVER);
		fEditor->SendMessage(SCI_INDICATORCLEARRANGE, fLastWordStartPosition, fLastWordEndPosition - fLastWordStartPosition);
		fLastWordEndPosition = fLastWordStartPosition = -1;
	}

	if (message->GetInt32("buttons", 0) != 0 || !(modifiers() & B_COMMAND_KEY)) {
		return;
	}

	if (message->GetInt32("be:transit", 0) == 1) {
		BPoint point = message->GetPoint("be:view_where", BPoint(0,0));
		Sci_Position sci_position = fEditor->SendMessage(SCI_POSITIONFROMPOINTCLOSE, point.x, point.y);
		fLastWordStartPosition = fEditor->SendMessage(SCI_WORDSTARTPOSITION, sci_position, true);
		fLastWordEndPosition   = fEditor->SendMessage(SCI_WORDENDPOSITION,   sci_position, true);
		fEditor->SendMessage(SCI_SETINDICATORCURRENT, IND_OVER);
		fEditor->SendMessage(SCI_INDICATORFILLRANGE, fLastWordStartPosition, fLastWordEndPosition - fLastWordStartPosition);
	}
}




int32
LSPEditorWrapper::DiagnosticFromPosition(Sci_Position sci_position, LSPDiagnostic& dia)
{
	int32 index = -1;
	if (fEditor->SendMessage(SCI_INDICATORVALUEAT, IND_DIAG, sci_position) == 1) {
		for (auto& ir : fLastDiagnostics) {
			index++;
			if (sci_position >= ir.range.from && sci_position < ir.range.to) {
				dia = ir;
				return index;
			}
		}
	}
	return -1;
}


int32
LSPEditorWrapper::DiagnosticFromRange(Range& range, LSPDiagnostic& dia)
{
	int32 index = -1;
	for (auto& ir : fLastDiagnostics) {
		index++;
		if (ir.diagnostic.range == range) {
			dia = ir;
			return index;
		}
	}
	return index;
}


void
LSPEditorWrapper::EndHover()
{
	if (!IsInitialized())
		return;

	BAutolock l(fEditor->Looper());
	if (l.IsLocked()) {
		fEditor->HideToolTip();
	}
}


void
LSPEditorWrapper::SwitchSourceHeader()
{
	if (!IsInitialized() || !IsStatusValid())
		return;
	fLSPProjectWrapper->SwitchSourceHeader(this);
}


void
LSPEditorWrapper::SelectedCompletion(const char* text)
{
	if (!IsInitialized() || fEditor == nullptr)
		return;

	flushChanges();

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

	if (!IsInitialized() || !fEditor || !IsStatusValid())
		return;

	flushChanges();

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


void
LSPEditorWrapper::NextCallTip()
{
	fCallTip.NextCallTip();
}


void
LSPEditorWrapper::PrevCallTip()
{
	fCallTip.PrevCallTip();
}


void
LSPEditorWrapper::HideCallTip()
{
	fCallTip.HideCallTip();
}


void
LSPEditorWrapper::RequestDocumentSymbols()
{
	if (fLSPProjectWrapper == nullptr || fEditor == nullptr)
		return;

	fLSPProjectWrapper->DocumentSymbol(this);
}


void
LSPEditorWrapper::CharAdded(const char ch /*utf-8?*/)
{
	// printf("on char %c\n", ch);
	if (!IsInitialized() || fEditor == nullptr)
		return;

	if (ch != 0) {
		if (fEditor->SendMessage(SCI_AUTOCACTIVE) &&
			!Contains(kWordCharacters, ch)) {
			fEditor->SendMessage(SCI_AUTOCCANCEL);
		}

		if (fLSPProjectWrapper->HasCapability(kLCapCompletion) &&
				Contains(fLSPProjectWrapper->triggerCharacters(), ch)) {

			if (fCallTip.IsVisible())
				fCallTip.HideCallTip();

			flushChanges();
			StartCompletion();
		}
	}

	if (fLSPProjectWrapper->HasCapability(kLCapSignatureHelp)) {
		CallTipAction action = fCallTip.UpdateCallTip(ch, ch == 0);
		if (action == CALLTIP_NEWDATA) {

			flushChanges();
			Position lsp_position;
			FromSciPositionToLSPPosition(fCallTip.Position(), &lsp_position);
			fLSPProjectWrapper->SignatureHelp(this, lsp_position);

		} else if (action == CALLTIP_UPDATE) {
			fCallTip.ShowCallTip();
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

	if (fEditor->SendMessage(SCI_INDICATORVALUEAT, IND_OVER, sci_position) == 1) {
		Position position;
		FromSciPositionToLSPPosition(sci_position, &position);
		fLSPProjectWrapper->GoToDefinition(this, position);
	}
#ifdef IND_LINK
	if (fEditor->SendMessage(SCI_INDICATORVALUEAT, IND_LINK, sci_position) == 1) {
		for (auto& ir : fLastDocumentLinks) {
			if (sci_position > ir.from && sci_position <= ir.to) {
				LogTrace("Opening file: [%s]", ir.info.c_str());
				OpenFileURI(ir.info);
				break;
			}
		}
	}
#endif
}


void
LSPEditorWrapper::_ShowToolTip(const char* text)
{
	if (fToolTip == nullptr)
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
	if (fEditor == nullptr || !fEditor->Window()->IsActive())
		return;

	if (result == nlohmann::detail::value_t::null &&
		!result["contents"].contains("value")) {
		EndHover();
		return;
	}

	std::string tip = result["contents"]["value"].get<std::string>();

	if (tip.empty()) {
		EndHover();
		return;
	}
	_ShowToolTip(tip.c_str());
}


void
LSPEditorWrapper::_DoGoTo(nlohmann::json& items)
{
	if (!items.empty()) {
		Location location;

		// TODO if more than one match??
		// clangd sends an array of Locations while OmniSharp seems to conform to the standard
		// and sends just one.
		if (items.is_array())
			location = items[0].get<Location>();
		else
			location = items.get<Location>();

		std::string uri = location.uri;
		Position pos = location.range.start;
		OpenFileURI(uri, pos.line + 1, pos.character);
	}
}


void
LSPEditorWrapper::_DoSignatureHelp(json& result)
{
	auto signs = result.get<SignatureHelp>().signatures;
	fCallTip.UpdateSignatures(signs);
	fCallTip.ShowCallTip();
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
	std::string line;
	Position position;
	position.character = -1;

	CompletionList allItems = params.get<CompletionList>();
	auto& items = allItems.items;
	std::string list;
	for (auto& item : items) {
		std::string label = item.label;
		LeftTrim(label);
		if (list.length() > 0)
			list += "\n";
		list += label;
		item.label = label;
		// if the server is not providing us the textEdit (like pylsp)
		// let's try to create it.
		if (item.textEdit.newText.empty()) {
			item.textEdit.newText = item.insertText;

			Position pos;
			FromSciPositionToLSPPosition(fCompletionPosition, &pos);
			item.textEdit.range.end = pos;

			// fancy algo to find insertText before current position.
			if (position.character == -1) {
				line = GetCurrentLine();
				GetCurrentLSPPosition(&position);
			}

			Sci_Position current = position.character - 1;
			int32 points = 0;
			for (size_t i = 0 ; i < item.insertText.length() ; i++) {
				if (current - i >= 0) {
					if (strncasecmp(item.insertText.c_str(), line.c_str() + current-i, i+1) == 0){
						points = i+1;
					}
				}
			}
			FromSciPositionToLSPPosition(fCompletionPosition - points, &pos);
			item.textEdit.range.start = pos;

		}
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


void
LSPEditorWrapper::_DoDiagnostics(nlohmann::json& params)
{
	auto vect = params["diagnostics"].get<std::vector<Diagnostic>>();

	_RemoveAllDiagnostics();

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

		// dia["be:line"] = v.range.start.line + 1;
		// dia["lsp:character"] = v.range.start.character;

		if (v.codeActions.value().size() == 0) {
			// if the language serve does not support in-line code actions we request them
			// asyncronously
			// TODO: Check the capability
			RequestCodeActions(v);
		}

		fLastDiagnostics.push_back(lspDiag);
	}

	if (fEditor->LockLooper()) {
		fEditor->SetProblems();
		fEditor->UnlockLooper();
	}

	if (fLSPProjectWrapper) {
#ifdef DOCUMENT_LINK
		fLSPProjectWrapper->DocumentLink(this);
#endif
		fLSPProjectWrapper->DocumentSymbol(this);
	}
}


void
LSPEditorWrapper::RequestCodeActions(Diagnostic& diagnostic)
{
	CodeActionContext context;
	context.diagnostics.push_back(std::move(diagnostic));
	fLSPProjectWrapper->CodeAction(this, diagnostic.range, context);
}


void
LSPEditorWrapper::CodeActionResolve(value &params)
{
	fLSPProjectWrapper->CodeActionResolve(this, params);
}


void
LSPEditorWrapper::_DoCodeActions(nlohmann::json& params)
{
	for (auto& v : params) {
		CodeAction action;

		action.kind = v["kind"].get<std::string>();
		action.title = v["title"].get<std::string>();
		auto data = v["data"];
		action.data = data;

		Range range;
		range.start.character = data["Range"]["Start"]["Character"].get<int>();
		range.start.line = data["Range"]["Start"]["Line"].get<int>();
		range.end.character = data["Range"]["End"]["Character"].get<int>();
		range.end.line = data["Range"]["End"]["Line"].get<int>();

		for (auto& d: fLastDiagnostics) {
			if (d.diagnostic.range == range) {
				d.diagnostic.codeActions.value().push_back(action);
				action.diagnostics.value().push_back(d.diagnostic);

				if (v["edit"].empty())
					CodeActionResolve(v);
			}
		}
	}
}


void
LSPEditorWrapper::_DoCodeActionResolve(nlohmann::json& params)
{
	CodeAction action;

	action.kind = params["kind"].get<std::string>();
	action.title = params["title"].get<std::string>();
	action.edit = params["edit"].get<WorkspaceEdit>();
	auto data = params["data"];
	action.data = data;

	Range range;
	range.start.character = data["Range"]["Start"]["Character"].get<int>();
	range.start.line = data["Range"]["Start"]["Line"].get<int>();
	range.end.character = data["Range"]["End"]["Character"].get<int>();
	range.end.line = data["Range"]["End"]["Line"].get<int>();

	for (auto& d: fLastDiagnostics) {
		if (d.diagnostic.range == range) {
			for (auto& ca: d.diagnostic.codeActions.value()) {
				auto caIdentifier = ca.data.value()["Identifier"].get<std::string>();
				auto actionIdentifier = action.data.value()["Identifier"].get<std::string>();
				if (caIdentifier == actionIdentifier) {
					ca.edit = action.edit;
				}
			}
		}
	}
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
LSPEditorWrapper::_DoInitialize(nlohmann::json& params)
{
	fInitialized = true;
	didOpen();
	BMessage symbols;
	if (HasLSPServerCapability(kLCapDocumentSymbols))
		fEditor->SetDocumentSymbols(&symbols, Editor::STATUS_REQUESTED);
	else
		fEditor->SetDocumentSymbols(&symbols, Editor::STATUS_NO_CAPABILITY);
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
	if (fEditor != nullptr) {
		BMessage msg(editor::StatusView::UPDATE_STATUS);
		BMessenger(fEditor).SendMessage(&msg);
	}
}

void
LSPEditorWrapper::_DoDocumentSymbol(nlohmann::json& params)
{
	BMessage msg(EDITOR_UPDATE_SYMBOLS);
	if (params.is_array() && params.size() > 0) {
		if (params[0]["location"].is_null()) {
			auto vect = params.get<std::vector<DocumentSymbol>>();
			_DoRecursiveDocumentSymbol(vect, msg);
		} else {
			auto vect = params.get<std::vector<SymbolInformation>>();
			_DoLinearSymbolInformation(vect, msg);
		}
	}
	if (fEditor != nullptr)
		fEditor->SetDocumentSymbols(&msg, Editor::STATUS_HAS_SYMBOLS);
}

void
LSPEditorWrapper::_DoRecursiveDocumentSymbol(std::vector<DocumentSymbol>& vect, BMessage& msg)
{
	for (DocumentSymbol sym: vect) {
		BMessage symbol;
		symbol.AddString("name", sym.name.c_str());
		symbol.AddInt32("kind", (int32)sym.kind);
		symbol.AddString("detail", sym.detail.c_str());
		BMessage child;
		if (sym.children.size() > 0) {
			_DoRecursiveDocumentSymbol(sym.children, child);
		}
		symbol.AddMessage("children", &child);
		Range& symbolRange = sym.selectionRange;
		symbol.AddInt32("start:line", symbolRange.start.line + 1);
		symbol.AddInt32("start:character", symbolRange.start.character);
		Range& range = sym.range;
		symbol.AddInt32("range:start:line", range.start.line + 1);
		symbol.AddInt32("range:end:line", 	range.end.line + 1);
		msg.AddMessage("symbol", &symbol);
	}
}

void
LSPEditorWrapper::_DoLinearSymbolInformation(std::vector<SymbolInformation>& vect, BMessage& msg)
{
	for (SymbolInformation sym: vect) {
		BMessage symbol;
		symbol.AddString("name", sym.name.c_str());
		symbol.AddInt32("kind", (int32)sym.kind);
		Range& symbolRange = sym.location.range;
		symbol.AddInt32("start:line", symbolRange.start.line + 1);
		symbol.AddInt32("start:character", symbolRange.start.character);
		msg.AddMessage("symbol", &symbol);
	}
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
	IF_ID("textDocument/rename", _DoRename);
	IF_ID("textDocument/definition", _DoGoTo);
	IF_ID("textDocument/declaration", _DoGoTo);
	IF_ID("textDocument/implementation", _DoGoTo);
	IF_ID("textDocument/signatureHelp", _DoSignatureHelp);
	IF_ID("textDocument/switchSourceHeader", _DoSwitchSourceHeader);
	IF_ID("textDocument/completion", _DoCompletion);
	IF_ID("textDocument/documentLink", _DoDocumentLink);
	IF_ID("textDocument/documentSymbol", _DoDocumentSymbol);
	IF_ID("initialize", _DoInitialize);
	IF_ID("textDocument/codeAction", _DoCodeActions);
	IF_ID("codeAction/resolve", _DoCodeActionResolve);

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
LSPEditorWrapper::OpenFileURI(std::string uri, int32 line, int32 character, BString edits)
{
	BUrl url = uri.c_str();
	if (url.IsValid() && url.HasPath()) {
		BEntry entry(url.Path().String());
		entry_ref ref;
		if (entry.Exists()) {
			BMessage refs(B_REFS_RECEIVED);
			if (entry.GetRef(&ref) == B_OK) {
				refs.AddRef("refs", &ref);
				if (line != -1) {
					refs.AddInt32("start:line", line);
					if (character != -1)
						refs.AddInt32("start:character", character);
				}
				if (!edits.IsEmpty())
					refs.AddString("edit", edits);

				JumpNavigator::getInstance()->JumpToFile(&refs, fEditor->FileRef());
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
