/*
 * Orininal code from Idea project
 * Parts borrowed from SciTe and Koder editors
 * Copyright 2017 A. Mosca <amoscaster@gmail.com>
 * Copyright (c) Neil Hodgson
 * Copyright 2014-2019 Kacper Kasper <kacperkasper@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "Editor.h"

#include <regex>

#include <Alert.h>
#include <Application.h>
#include <Catalog.h>
#include <Control.h>
#include <ControlLook.h>
#include <editorconfig/editorconfig.h>
#include <ILexer.h>
#include <Lexilla.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <SciLexer.h>
#include <Url.h>
#include <Volume.h>

#include "ConfigManager.h"
#include "EditorContextMenu.h"
#include "EditorMessages.h"
#include "EditorStatusView.h"
#include "GenioApp.h"
#include "GenioWindowMessages.h"
#include "GoToLineWindow.h"
#include "HvifImporter.h"
#include "alert/GTextAlert.h"
#include "Languages.h"
#include "Log.h"
#include "LSPEditorWrapper.h"
#include "ProjectFolder.h"
#include "ScintillaUtils.h"
#include "Styler.h"
#include "Utils.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Editor"

namespace Sci = Scintilla;
using namespace Sci::Properties;

const int kIdleTimeout = 500000; //1/2sec

// Differentiate unset parameters from 0 ones
// in scintilla messages
#define UNSET 0
#define UNUSED 0


Editor::Editor(entry_ref* ref, const BMessenger& target)
	:
	BScintillaView(ref->name, 0, true, true)
	, fFileRef(*ref)
	, fModified(false)
	, fBracingAvailable(false)
	, fFoldingAvailable(false)
	, fSyntaxAvailable(false)
	, fParsingAvailable(false)
	, fCommenter("")
	, fCurrentLine(-1)
	, fCurrentColumn(-1)
	, fProjectFolder(NULL)
	, fSymbolsStatus(STATUS_NOT_INITIALIZED)
	, fIdleHandler(nullptr)
{
	fStatusView = new editor::StatusView(this);
	fFileName = BString(ref->name);
	SetTarget(target);

	fLSPEditorWrapper = new LSPEditorWrapper(BPath(&fFileRef), this);

	LoadEditorConfig();

	// MARGINS
	SendMessage(SCI_SETMARGINS, 4, UNSET);

	//Line number margins.
	SendMessage(SCI_SETMARGINTYPEN, sci_NUMBER_MARGIN, SC_MARGIN_NUMBER);

	//Bookmark margin
	SendMessage(SCI_SETMARGINTYPEN, sci_BOOKMARK_MARGIN, SC_MARGIN_SYMBOL);
	SendMessage(SCI_SETMARGINSENSITIVEN, sci_BOOKMARK_MARGIN, 1);
	SendMessage(SCI_MARKERDEFINE, sci_BOOKMARK, SC_MARK_BOOKMARK);
 	SendMessage(SCI_SETMARGINMASKN, sci_BOOKMARK_MARGIN, (1 << sci_BOOKMARK));

	//Fold margin
	SendMessage(SCI_SETMARGINTYPEN, sci_FOLD_MARGIN, SC_MARGIN_SYMBOL);
	SendMessage(SCI_SETMARGINMASKN, sci_FOLD_MARGIN, SC_MASK_FOLDERS);
	SendMessage(SCI_SETMARGINSENSITIVEN, sci_FOLD_MARGIN, 1);

	SendMessage(SCI_SETPROPERTY, (sptr_t) "fold", (sptr_t) "1");
	SendMessage(SCI_SETPROPERTY, (sptr_t) "fold.comment", (sptr_t) "1");

	SendMessage(SCI_SETMARGINTYPEN, sci_FOLD_MARGIN, SC_MARGIN_SYMBOL);
	SendMessage(SCI_SETMARGINMASKN, sci_FOLD_MARGIN, SC_MASK_FOLDERS);
	SendMessage(SCI_SETMARGINWIDTHN, sci_FOLD_MARGIN, 16);
	SendMessage(SCI_SETMARGINSENSITIVEN, sci_FOLD_MARGIN, 1);

	SendMessage(SCI_MARKERDEFINE, SC_MARKNUM_FOLDER, SC_MARK_BOXPLUS);
	SendMessage(SCI_MARKERDEFINE, SC_MARKNUM_FOLDEROPEN, SC_MARK_BOXMINUS);
	SendMessage(SCI_MARKERDEFINE, SC_MARKNUM_FOLDEREND, SC_MARK_BOXPLUSCONNECTED);
	SendMessage(SCI_MARKERDEFINE, SC_MARKNUM_FOLDERMIDTAIL, SC_MARK_TCORNER);
	SendMessage(SCI_MARKERDEFINE, SC_MARKNUM_FOLDEROPENMID, SC_MARK_BOXMINUSCONNECTED);
	SendMessage(SCI_MARKERDEFINE, SC_MARKNUM_FOLDERSUB, SC_MARK_VLINE);
	SendMessage(SCI_MARKERDEFINE, SC_MARKNUM_FOLDERTAIL, SC_MARK_LCORNER);
	SendMessage(SCI_SETFOLDFLAGS, SC_FOLDFLAG_LINEAFTER_CONTRACTED, 0);

	SendMessage(SCI_SETAUTOMATICFOLD, SC_AUTOMATICFOLD_SHOW, 1);
	SendMessage(SCI_SETAUTOMATICFOLD, SC_AUTOMATICFOLD_CHANGE, 4);
	SendMessage(SCI_SETAUTOMATICFOLD, SC_AUTOMATICFOLD_CLICK, 2);

	// Comment margin
	SendMessage(SCI_SETMARGINSENSITIVEN, sci_COMMENT_MARGIN, 1);
	//Wrap visual flag
	SendMessage(SCI_SETWRAPVISUALFLAGS, SC_WRAPVISUALFLAG_MARGIN);
}

bool
Editor::HasLSPServer() const
{
	return (fLSPEditorWrapper && fLSPEditorWrapper->HasLSPServer());
}

bool
Editor::HasLSPCapability(const LSPCapability cap)
{
	return (HasLSPServer() && fLSPEditorWrapper->HasLSPServerCapability(cap));
}


Editor::~Editor()
{
	// Stop monitoring
	StopMonitoring();

	// Set caret position
	if (gCFG["save_caret"]) {
		BNode node(&fFileRef);
		if (node.InitCheck() == B_OK) {
			int32 pos = GetCurrentPosition();
			node.WriteAttr("be:caret_position", B_INT32_TYPE, 0, &pos, sizeof(pos));
		}
	}

	fLSPEditorWrapper->UnsetLSPServer();
	delete fLSPEditorWrapper;
	fLSPEditorWrapper = NULL;
}


void
Editor::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kIdle:
			fLSPEditorWrapper->flushChanges();
			break;
		case MSG_REPLACE_ALL:
		case MSG_REPLACE_NEXT:
		case MSG_REPLACE_ONE:
		case MSG_REPLACE_PREVIOUS:
		{
			BString text = message->GetString("text", "");
			BString replace = message->GetString("replace", "");

			GrabFocus();
			bool matchCase = message->GetBool("match_case", false);
			bool wholeWord = message->GetBool("whole_word", false);

			int flags = SetSearchFlags(matchCase, wholeWord, false, false, false);
			bool wrap = message->GetBool("wrap", false);
			int32 kind = message->GetInt32("kind", REPLACE_NONE);

			switch (kind) {
				case REPLACE_ALL:
				{
					ReplaceAll(text, replace, flags);
					break;
				}
				case REPLACE_NEXT:
				{
					ReplaceAndFindNext(text, replace, flags, wrap);
					break;
				}
				case REPLACE_ONE:
				{
					ReplaceOne(text, replace);
					break;
				}
				case REPLACE_PREVIOUS:
				{
					ReplaceAndFindPrevious(text, replace, flags, wrap);
					break;
				}
				default:
					break;
			}
			break;
		}
		case MSG_FIND_MARK_ALL:
		{
			BString text = message->GetString("text", "");
			if (text.IsEmpty())
				return;
			GrabFocus();
			bool matchCase = message->GetBool("match_case", false);
			bool wholeWord = message->GetBool("whole_word", false);

			int flags = SetSearchFlags(matchCase, wholeWord, false, false, false);

			FindMarkAll(text, flags);

			break;
		}
		case MSG_FIND_NEXT:
		case MSG_FIND_PREVIOUS:
		{
			BString text = message->GetString("text", "");
			if (text.IsEmpty())
				return;

			GrabFocus();
			bool matchCase = message->GetBool("match_case", false);
			bool wholeWord = message->GetBool("whole_word", false);

			int flags = SetSearchFlags(matchCase, wholeWord, false, false, false);
			bool wrap = message->GetBool("wrap", false);

			if (message->GetBool("backward", false) == false)
				FindNext(text, flags, wrap);
			else
				FindPrevious(text, flags, wrap);

			break;
		}
		case MSG_BOOKMARK_CLEAR_ALL:
			BookmarkClearAll(sci_BOOKMARK);
			break;
		case MSG_BOOKMARK_GOTO_NEXT:
			if (!BookmarkGoToNext())
				LogInfo("Next Bookmark not found");
			break;
		case MSG_BOOKMARK_GOTO_PREVIOUS:
			if (!BookmarkGoToPrevious())
				LogInfo("Previous Bookmark not found");
			break;
		case MSG_BOOKMARK_TOGGLE:
			BookmarkToggle(GetCurrentPosition());
			break;
		case MSG_EOL_CONVERT_TO_UNIX:
			EndOfLineConvert(SC_EOL_LF);
			break;
		case MSG_EOL_CONVERT_TO_DOS:
			EndOfLineConvert(SC_EOL_CRLF);
			break;
		case MSG_EOL_CONVERT_TO_MAC:
			EndOfLineConvert(SC_EOL_CR);
			break;
		case MSG_FILE_FOLD_TOGGLE:
			ToggleFolding();
			break;
		case GTLW_GO:
		{
			int32 line;
			if(message->FindInt32("line", &line) == B_OK) {
				GoToLine(line);
			}
			break;
		}
		case MSG_DUPLICATE_LINE:
			DuplicateCurrentLine();
			break;
		case MSG_DELETE_LINES:
			DeleteSelectedLines();
			break;
		case MSG_COMMENT_SELECTED_LINES:
			CommentSelectedLines();
			break;
		case MSG_FILE_TRIM_TRAILING_SPACE:
			TrimTrailingWhitespace();
			break;
		case MSG_AUTOCOMPLETION:
			Completion();
			break;
		case MSG_FORMAT:
			Format();
			break;
		case MSG_GOTODEFINITION:
			GoToDefinition();
			break;
		case MSG_GOTODECLARATION:
			GoToDeclaration();
			break;
		case MSG_GOTOIMPLEMENTATION:
			GoToImplementation();
			break;
		case MSG_RENAME:
			Rename();
			break;
		case MSG_SWITCHSOURCE:
			SwitchSourceHeader();
			break;
		case MSG_TEXT_OVERWRITE:
			OverwriteToggle();
			break;
		case MSG_SET_LANGUAGE:
			SetFileType(std::string(message->GetString("lang", "")));
			ApplySettings();
			//NOTE (TODO?) we are not changing any LSP configuration!
			break;
		case kApplyFix:
			if (fLSPEditorWrapper)
				fLSPEditorWrapper->ApplyFix(message);
			break;
		case kCallTipClick:
		{
			int32 position = message->GetInt32("position", 0);
			if (position == 1)
				fLSPEditorWrapper->PrevCallTip();
			else
				fLSPEditorWrapper->NextCallTip();
			break;
		}
		case MSG_COLLAPSE_SYMBOL_NODE:
		{
			BString symbol;
			message->FindString("name", &symbol);
			int32 kind;
			message->FindInt32("kind", &kind);
			bool collapsed;
			message->FindBool("collapsed", &collapsed);
			if (collapsed) {
				fCollapsedSymbols.insert(std::make_pair(symbol.String(), kind));
			} else
				fCollapsedSymbols.erase(std::make_pair(symbol.String(), kind));
			break;
		}
		case MSG_LOAD_HVIF:
		{
			entry_ref ref;
			for (auto count = 0; message->FindRef("refs", count, &ref) == B_OK; count++) {
				// HvifImporter(ref);
				SendMessage(SCI_INSERTTEXT, UNSET, (sptr_t)HvifImporter(ref).String());
			}
			break;
		}
		default:
			BScintillaView::MessageReceived(message);
			break;
	}
}


void
Editor::ApplySettings()
{
	SendMessage(SCI_STYLECLEARALL, UNSET, UNSET);

	_ApplyExtensionSettings();

	ShowWhiteSpaces(gCFG["show_white_space"]);
	ShowLineEndings(gCFG["show_line_endings"]);

	// editorconfig
	SendMessage(SCI_SETUSETABS, fEditorConfig.IndentStyle==IndentStyle::Tab, 0);
	SendMessage(SCI_SETTABWIDTH, fEditorConfig.IndentSize, 0);
	SendMessage(SCI_SETINDENT, 0, 0);
	if (fEditorConfig.TrimTrailingWhitespace)
		TrimTrailingWhitespace();
	EndOfLineConvert(fEditorConfig.EndOfLine);

	SendMessage(SCI_SETCARETLINEVISIBLE, bool(gCFG["mark_caretline"]), 0);
	SendMessage(SCI_SETCARETLINEVISIBLEALWAYS, true, 0);
	// TODO add settings: SendMessage(SCI_SETCARETLINEFRAME, fPreferences->fLineHighlightingMode ? 2
	// : 0);

	// Edge line
	SendMessage(SCI_SETEDGEMODE, (bool)gCFG["show_ruler"], UNSET);
	SendMessage(SCI_SETEDGECOLUMN, int32(gCFG["ruler_column"]), UNSET);

	// TODO: Implement this settings (right now managed by _HighlightBraces)
	/*if(fPreferences->fIndentGuidesShow == true) {
		fEditor->SendMessage(SCI_SETINDENTATIONGUIDES, fPreferences->fIndentGuidesMode, 0);
	} else {
		fEditor->SendMessage(SCI_SETINDENTATIONGUIDES, 0, 0);
	}*/

	_HighlightBraces();

	if((bool)gCFG["wrap_lines"] == true) {
		SendMessage(SCI_SETWRAPMODE, SC_WRAP_WORD, 0);
	} else {
		SendMessage(SCI_SETWRAPMODE, SC_WRAP_NONE, 0);
	}

	// Folding
	_SetFoldMargin(gCFG["enable_folding"]);

	// Line commenter margin
	if (gCFG["show_commentmargin"] && !fCommenter.empty()) {
		SendMessage(SCI_SETMARGINWIDTHN, sci_COMMENT_MARGIN, 12); //TODO make it relative to font size
	} else {
		SendMessage(SCI_SETMARGINWIDTHN, sci_COMMENT_MARGIN, 0);
	}

	SetZoom(gCFG["editor_zoom"]);

	fLSPEditorWrapper->ApplySettings();

	// custom ContextMenu!
	SendMessage(SCI_USEPOPUP, SC_POPUP_NEVER, 0);

	UpdateStatusBar();
}

void
Editor::ApplyEdit(std::string info)
{
	fLSPEditorWrapper->ApplyEdit(info);
}

void
Editor::BookmarkClearAll(int marker)
{
	SendMessage(SCI_MARKERDELETEALL, marker, UNSET);
}


bool
Editor::BookmarkGoToNext()
{
	Sci_Position pos = SendMessage(SCI_GETCURRENTPOS);
	int64 line = SendMessage(SCI_LINEFROMPOSITION, pos);
	int64 bookmark = SendMessage(SCI_MARKERNEXT, line + 1, (1 << sci_BOOKMARK));
	if (bookmark == -1)
		bookmark = SendMessage(SCI_MARKERNEXT, 0, (1 << sci_BOOKMARK));
	if (bookmark != -1)
		GoToLine(bookmark+1);
	return true;
}


bool
Editor::BookmarkGoToPrevious()
{
	Sci_Position pos = SendMessage(SCI_GETCURRENTPOS);
	int64 line = SendMessage(SCI_LINEFROMPOSITION, pos);
	int64 bookmark = SendMessage(SCI_MARKERPREVIOUS, line - 1, (1 << sci_BOOKMARK));
	if (bookmark == -1)
		bookmark = SendMessage(SCI_MARKERPREVIOUS, SendMessage(SCI_GETLINECOUNT), (1 << sci_BOOKMARK));
	if (bookmark != -1)
		GoToLine(bookmark+1);
	return true;
}


void
Editor::BookmarkToggle(int position)
{
	int line = SendMessage(SCI_LINEFROMPOSITION, position, UNSET);
	int markerSet = SendMessage(SCI_MARKERGET, line, UNSET);
	if ((markerSet & (1 << sci_BOOKMARK)) != 0)
		SendMessage(SCI_MARKERDELETE, line, sci_BOOKMARK);
	else
		SendMessage(SCI_MARKERADD, line, sci_BOOKMARK);
}


void
Editor::TrimTrailingWhitespace()
{
	if ((gCFG["ignore_editorconfig"] && gCFG["trim_trailing_whitespace"])
		|| fEditorConfig.TrimTrailingWhitespace) {
		Sci::Guard<SearchTarget, SearchFlags> guard(this);

		Sci_Position length = SendMessage(SCI_GETLENGTH, 0, 0);
		Set<SearchTarget>({0, length});
		Set<SearchFlags>(SCFIND_REGEXP | SCFIND_CXX11REGEX);

		Sci::UndoAction action(this);
		const std::string whitespace = "\\s+$";
		int result;
		do {
			result = SendMessage(SCI_SEARCHINTARGET, whitespace.size(), (sptr_t)whitespace.c_str());
			if (result != -1) {
				SendMessage(SCI_REPLACETARGET, -1, (sptr_t)"");
				Set<SearchTarget>({Get<SearchTargetEnd>(), length});
			}
		} while(result != -1);
	}
}


bool
Editor::CanClear()
{
	return ((SendMessage(SCI_GETSELECTIONEMPTY, UNSET, UNSET) == 0) &&
				!IsReadOnly());
}


bool
Editor::CanCopy()
{
	return (SendMessage(SCI_GETSELECTIONEMPTY, UNSET, UNSET) == 0);
}


bool
Editor::CanCut()
{
	return ((SendMessage(SCI_GETSELECTIONEMPTY, UNSET, UNSET) == 0) &&
				!IsReadOnly());
}


bool
Editor::CanPaste()
{
	return SendMessage(SCI_CANPASTE, UNSET, UNSET);
}


bool
Editor::CanRedo()
{
	return SendMessage(SCI_CANREDO, UNSET, UNSET);
}


bool
Editor::CanUndo()
{
	return SendMessage(SCI_CANUNDO, UNSET, UNSET);
}


void
Editor::Clear()
{
	SendMessage(SCI_CLEAR, UNSET, UNSET);
}


void
Editor::Copy()
{
	SendMessage(SCI_COPY, UNSET, UNSET);
}


int32
Editor::CountLines()
{
	return SendMessage(SCI_GETLINECOUNT, UNSET, UNSET);
}


void
Editor::Cut()
{
	SendMessage(SCI_CUT, UNSET, UNSET);
}


BString const
Editor::_EndOfLineString()
{
	int32 eolMode = EndOfLine();

	switch (eolMode) {
		case SC_EOL_CRLF:
			return "CRLF";
		case SC_EOL_CR:
			return "CR";
		case SC_EOL_LF:
			return "LF";
		default:
			return "";
	}
}


void
Editor::EndOfLineConvert(int32 eolMode)
{
	// Should not happen
	if (IsReadOnly() == true)
		return;

	SendMessage(SCI_CONVERTEOLS, eolMode, UNSET);
	SendMessage(SCI_SETEOLMODE, eolMode, UNSET);

	UpdateStatusBar();
}


void
Editor::EnsureVisiblePolicy()
{
	SendMessage(SCI_ENSUREVISIBLEENFORCEPOLICY,
		SendMessage(SCI_LINEFROMPOSITION, GetCurrentPosition(), UNSET), UNSET);
}


const BString
Editor::FilePath() const
{
	BPath path(&fFileRef);
	return path.Path();
}

int32
Editor::Find(const BString&  text, int flags, bool backwards, bool onWrap)
{
	const Sci_Position anchor = SendMessage(SCI_GETANCHOR);
	const Sci_Position current = SendMessage(SCI_GETCURRENTPOS);
	const int length = SendMessage(SCI_GETLENGTH);

	int position =  -1;
	if (!onWrap) {
		position = backwards ? FindInTarget(text, flags, std::min(anchor, current), 0)
							 : FindInTarget(text, flags, std::max(anchor, current), length);
	} else {
		position = backwards ? FindInTarget(text, flags, length, std::max(anchor, current))
							 : FindInTarget(text, flags, 0, std::min(anchor, current));
	}

	if (position != -1) {
		int end = SendMessage(SCI_GETTARGETEND);
		SendMessage(SCI_SETANCHOR, position);
		SendMessage(SCI_SETCURRENTPOS, end);
	}
	return position;
}

int
Editor::FindInTarget(const BString& search, int flags, int startPosition, int endPosition)
{
	SendMessage(SCI_SETTARGETRANGE, startPosition, endPosition);
	SendMessage(SCI_SETSEARCHFLAGS, flags, UNSET);
	return SendMessage(SCI_SEARCHINTARGET, search.Length(), (sptr_t) search.String());
}


int32
Editor::FindMarkAll(const BString& text, int flags)
{
	int position, count = 0;
	int line, firstMark;

	// Clear all
	BookmarkClearAll(sci_BOOKMARK);

	// TODO use wrap
	SendMessage(SCI_TARGETWHOLEDOCUMENT, UNSET, UNSET);
	SendMessage(SCI_SETSEARCHFLAGS, flags, UNSET);
	position = SendMessage(SCI_SEARCHINTARGET, text.Length(), (sptr_t) text.String());
	firstMark = position;

	if (position == -1)
		// No occurrence found
		return 0;
	else
		SendMessage(SCI_GOTOPOS, position, UNSET);

	while (position != -1) {
		SendMessage(SCI_SEARCHANCHOR, 0, 0);
		position = SendMessage(SCI_SEARCHNEXT, flags, (sptr_t) text.String());

		if (position == -1)
			break;
		else {
			// Get line
			line = SendMessage(SCI_LINEFROMPOSITION, position, UNSET);
			SendMessage(SCI_MARKERADD, line, sci_BOOKMARK);
			SendMessage(SCI_CHARRIGHT, UNSET, UNSET);
			count++;

			// Found occurrence, message window
			BMessage message(EDITOR_FIND_SET_MARK);
			message.AddRef("ref", &fFileRef);
			int line = SendMessage(SCI_LINEFROMPOSITION, position, UNSET) + 1;
			message.AddInt32("line", line);
			fTarget.SendMessage(&message);
		}
	}

	SendMessage(SCI_GOTOPOS, firstMark, UNSET);
	BMessage message(EDITOR_FIND_COUNT);
	message.AddString("text_to_find", text);
	message.AddInt32("count", count);
	fTarget.SendMessage(&message);

	return count;
}

int
Editor::FindNext(const BString& search, int flags, bool wrap)
{
	int position = Find(search, flags, false, false);

	if (position == -1 && wrap == false) {
		BMessage message(EDITOR_FIND_NEXT_MISS);
		fTarget.SendMessage(&message);
	} else if (position == -1 && wrap == true) {
		position = Find(search, flags, false, true);
	}
	return position;
}


int
Editor::FindPrevious(const BString& search, int flags, bool wrap)
{
	int position = Find(search, flags, true, false);
	if (position == -1 && wrap == false) {
		BMessage message(EDITOR_FIND_PREV_MISS);
		fTarget.SendMessage(&message);
	} else if (position == -1 && wrap == true) {
		position = Find(search, flags, true, true);
	}
	return position;
}


int32
Editor::GetCurrentPosition()
{
	return SendMessage(SCI_GETCURRENTPOS, UNSET, UNSET);
}


/*
 * Mind that first line is 0!
 */
void
Editor::GoToLine(int32 line)
{
	// Do not go to line 0
	if (line == 0)
		return;

	line -= 1;
	SendMessage(SCI_GOTOLINE, line, UNSET);
	EnsureVisiblePolicy();
}

void
Editor::GoToLSPPosition(int32 line, int character)
{
	Sci_Position  sci_position;
	sci_position = SendMessage(SCI_POSITIONFROMLINE, line, 0);
	sci_position = SendMessage(SCI_POSITIONRELATIVE, sci_position, character);
	SendMessage(SCI_SETSEL, sci_position, sci_position);

	EnsureVisiblePolicy();
}


void
Editor::GrabFocus()
{
	SendMessage(SCI_GRABFOCUS, UNSET, UNSET);
}


bool
Editor::IsOverwrite()
{
	return SendMessage(SCI_GETOVERTYPE, UNSET, UNSET);
}


BString const
Editor::IsOverwriteString()
{
	if (SendMessage(SCI_GETOVERTYPE, UNSET, UNSET) == true)
		return "OVR";
	return "INS";
}


bool
Editor::IsReadOnly()
{
	return SendMessage(SCI_GETREADONLY, UNSET, UNSET);
}


bool
Editor::IsSearchSelected(const BString& search, int flags)
{
	int start = SendMessage(SCI_GETSELECTIONSTART, UNSET, UNSET);
	int end = SendMessage(SCI_GETSELECTIONEND, UNSET, UNSET);
	if (FindInTarget(search, flags, start, end) == start)
		return true;

	return false;
}


bool
Editor::IsTextSelected()
{
	return SendMessage(SCI_GETCURRENTPOS, UNSET, UNSET) !=
			SendMessage(SCI_GETANCHOR, UNSET, UNSET);
}


void
Editor::LoadEditorConfig()
{
	fEditorConfig.EndOfLine = EndOfLine();
	fEditorConfig.IndentStyle = (bool)gCFG["tab_to_space"] ? IndentStyle::Space : IndentStyle::Tab;
	fEditorConfig.IndentSize = (int)gCFG["tab_width"];
	fEditorConfig.TrimTrailingWhitespace = (bool)gCFG["trim_trailing_whitespace"];
	fHasEditorConfig = false;

	if (!(bool)gCFG["ignore_editorconfig"]) {
		// start parsing
		// Ignore full path error, whose error code is EDITORCONFIG_PARSE_NOT_FULL_PATH
		editorconfig_handle handle = editorconfig_handle_init();
		int errNum;
		if ((errNum = editorconfig_parse(FilePath().String(), handle)) != 0 &&
				errNum != EDITORCONFIG_PARSE_NOT_FULL_PATH) {
			editorconfig_handle_destroy(handle);
			LogError("Can't load .editorconfig with error %d", editorconfig_get_error_msg(errNum));
			fHasEditorConfig = false;
		} else {
			fHasEditorConfig = true;

			int32 nameValueCount = editorconfig_handle_get_name_value_count(handle);

			/* get settings */
			// Defaults. TODO: This avoids the compiler error
			// but maybe the code should be refactored
			int32 tabWidth = 4;
			for (int32 i = 0; i < nameValueCount; ++i) {
				const char* name;
				const char* value;
				editorconfig_handle_get_name_value(handle, i, &name, &value);

				if (!strcmp(name, "indent_style")) {
					fEditorConfig.IndentStyle = !strcmp(value, "space") ? IndentStyle::Space : IndentStyle::Tab;
				} else if (!strcmp(name, "tab_width")) {
					if (strcmp(value, "undefine"))
						tabWidth = atoi(value);
				} else if (!strcmp(name, "indent_size")) {
					if (strcmp(value, "undefine")) {
						int valueInt = atoi(value);
						if (!strcmp(value, "tab"))
							fEditorConfig.IndentSize = tabWidth;
						else if (valueInt > 0)
							fEditorConfig.IndentSize = valueInt;
					}
				} else if (!strcmp(name, "end_of_line")) {
					if (strcmp(value, "undefine")) {
						if (!strcmp(value, "lf"))
							fEditorConfig.EndOfLine = SC_EOL_LF;
						else if (!strcmp(value, "cr"))
							fEditorConfig.EndOfLine = SC_EOL_CR;
						else if (!strcmp(value, "crlf"))
							fEditorConfig.EndOfLine = SC_EOL_CRLF;
					}
				} else if (!strcmp(name, "trim_trailing_whitespace")) {
					if (strcmp(value, "undefine"))
						fEditorConfig.TrimTrailingWhitespace = !strcmp(value, "true") ? true : false;
				} else if (!strcmp(name, "insert_final_newline"))
					fEditorConfig.InsertFinalNewline = !strcmp(value, "true") ? true : false;
			}

			editorconfig_handle_destroy(handle);
		}
	}
}


/*
 * Code (editable) taken from stylededit
 */
status_t
Editor::LoadFromFile()
{
	status_t status;
	BFile file;
	if ((status = file.SetTo(&fFileRef, B_READ_ONLY)) != B_OK)
		return status;
	if ((status = file.InitCheck()) != B_OK)
		return status;
	if ((status = file.Lock()) != B_OK)
		return status;
	struct stat st;
	if ((status = file.GetStat(&st)) != B_OK)
		return status;

	bool editable = (getuid() == st.st_uid && S_IWUSR & st.st_mode)
					|| (getgid() == st.st_gid && S_IWGRP & st.st_mode)
					|| (S_IWOTH & st.st_mode);
	BVolume volume(fFileRef.device);
	editable = editable && !volume.IsReadOnly();

	off_t size;
	file.GetSize(&size);

	char* buffer = new char[size + 1];
	off_t len = file.Read(buffer, size);
	buffer[size] = '\0';

	SendMessage(SCI_SETTEXT, 0, (sptr_t) buffer);

	// Check the first newline only
	int32 lineLength = SendMessage(SCI_LINELENGTH, 0, UNSET);
	char *lineBuffer = new char[lineLength];
	SendMessage(SCI_GETLINE, 0, (sptr_t)lineBuffer);
	_EndOfLineAssign(lineBuffer, lineLength);
	delete[] lineBuffer;
	delete[] buffer;

	if (len != size)
		return B_ERROR;

	if ((status = file.Unlock()) != B_OK)
		return status;

	SendMessage(SCI_EMPTYUNDOBUFFER, UNSET, UNSET);
	SendMessage(SCI_SETSAVEPOINT, UNSET, UNSET);

	if (editable == false)
		SetReadOnly();

	// Monitor node
	StartMonitoring();

	fFileType = "";
	if (!Languages::GetLanguageForExtension(GetFileExtension(fFileName.String()), fFileType)) {
		BPath path(fFileName.String());
		if (path.InitCheck() == B_OK) {
			Languages::GetLanguageForExtension(path.Leaf(), fFileType);
		}
	}

	UpdateStatusBar();
	return B_OK;
}


BString const
Editor::ModeString()
{
	if (IsReadOnly())
		return "RO";
	return "RW";
}


void
Editor::NotificationReceived(SCNotification* notification)
{
	Sci_NotifyHeader* pNmhdr = &notification->nmhdr;

	switch (pNmhdr->code) {
		// Auto-indent
		case SCN_CHARADDED: {
			char ch = static_cast<char>(notification->ch);
			if (ch == '\n' || ch == '\r')
				_MaintainIndentation(ch);
			if (notification->characterSource == SC_CHARACTERSOURCE_DIRECT_INPUT)
				fLSPEditorWrapper->CharAdded(notification->ch);
			break;

		}
		case SCN_MARGINCLICK: {
			if (notification->margin == sci_BOOKMARK_MARGIN)
				// Bookmark toggle
				BookmarkToggle(notification->position);
			else if (notification->margin == sci_COMMENT_MARGIN)
				// Line commenter/decommenter
				_CommentLine(notification->position);
			break;
		}
		case SCN_AUTOCCOMPLETED:
		case SCN_AUTOCCANCELLED:
			fLSPEditorWrapper->CharAdded(0);
			break;
		case SCN_AUTOCSELECTION: {
			fLSPEditorWrapper->SelectedCompletion(notification->text);
			break;
		}
		case SCN_MODIFIED: {
			if (notification->modificationType & SC_MOD_INSERTTEXT) {
				fLSPEditorWrapper->didChange(notification->text, notification->length, notification->position, 0);
				EvaluateIdleTime();
			}
			if (notification->modificationType & SC_MOD_BEFOREDELETE) {
				fLSPEditorWrapper->didChange("", 0, notification->position, notification->length);
			}
			if (notification->modificationType & SC_MOD_DELETETEXT) {
					fLSPEditorWrapper->CharAdded(0);
					EvaluateIdleTime();
			}
			if (notification->linesAdded != 0)
				if (gCFG["show_linenumber"])
					_RedrawNumberMargin(false);
			break;
		}
		case SCN_CALLTIPCLICK: {
			GMessage click = {{"what",kCallTipClick},{"position", (int32)notification->position}};
			Looper()->PostMessage(&click, this);
		}
		case SCN_DWELLSTART: {
			fLSPEditorWrapper->StartHover(notification->position);
			break;
		}
		case SCN_DWELLEND: {
			fLSPEditorWrapper->EndHover();
			break;
		}
		case SCN_INDICATORRELEASE: {
			fLSPEditorWrapper->IndicatorClick(notification->position);
			break;
		}
		case SCN_SAVEPOINTLEFT: {
			_UpdateSavePoint(true);
			break;
		}
		case SCN_SAVEPOINTREACHED: {
			_UpdateSavePoint(false);
			break;
		}
		case SCN_UPDATEUI: {
			_BraceHighlight();
			// Selection/Position has changed
			if (notification->updated & SC_UPDATE_SELECTION) {
				// Ugly hack to enable mouse selection scrolling
				// in both directions
				int32 position = SendMessage(SCI_GETCURRENTPOS, UNSET, UNSET);
				int32 anchor = SendMessage(SCI_GETANCHOR, UNSET, UNSET);
				if (anchor != position) {
					int32 line = SendMessage(SCI_LINEFROMPOSITION, position, UNSET);
					if (line == SendMessage(SCI_GETFIRSTVISIBLELINE, UNSET, UNSET))
						SendMessage(SCI_SETFIRSTVISIBLELINE, line - 1, UNSET);
					else
						SendMessage(SCI_SCROLLCARET, UNSET, UNSET);
				}

				// Send position to main window so it can update the menus.
				SendPositionChanges();
				// Update status bar
				UpdateStatusBar();
			}
			break;
		}
	}
}


filter_result
Editor::BeforeKeyDown(BMessage* message)
{
	int8 key = message->GetInt8("byte", 0);
	switch (key) {
		case B_UP_ARROW:
		case B_DOWN_ARROW:
			return OnArrowKey(key);
		default:
			break;
	};

	return B_DISPATCH_MESSAGE;
}


filter_result
Editor::OnArrowKey(int8 key)
{
	if (SendMessage(SCI_CALLTIPACTIVE, 0, 0)) {
		if (key == B_DOWN_ARROW)
			fLSPEditorWrapper->NextCallTip();
		else
			fLSPEditorWrapper->PrevCallTip();

		return B_SKIP_MESSAGE;
	}
	return B_DISPATCH_MESSAGE;
}


void
Editor::_UpdateSavePoint(bool modified)
{
	fModified = modified;
	BMessage message(EDITOR_UPDATE_SAVEPOINT);
	message.AddRef("ref", &fFileRef);
	message.AddBool("modified", fModified);
	fTarget.SendMessage(&message);
}


void
Editor::OverwriteToggle()
{
	SendMessage(SCI_SETOVERTYPE, !IsOverwrite(), UNSET);
	UpdateStatusBar();
}


void
Editor::Paste()
{
	if (SendMessage(SCI_CANPASTE, UNSET, UNSET))
		SendMessage(SCI_PASTE, UNSET, UNSET);
}


void
Editor::Redo()
{
	SendMessage(SCI_REDO, UNSET, UNSET);
}


status_t
Editor::Reload()
{
	status_t status;
	BFile file;
	//TODO errors should be notified
	if ((status = file.SetTo(&fFileRef, B_READ_ONLY)) != B_OK)
		return status;
	if ((status = file.InitCheck()) != B_OK)
		return status;
	if ((status = file.Lock()) != B_OK)
		return status;

	// Enable external modifications of readonly file/buffer
	bool readOnly = IsReadOnly();
	if (readOnly == true)
		SendMessage(SCI_SETREADONLY, 0, UNSET);

	Sci_Position position = SendMessage(SCI_GETCURRENTPOS, UNSET, UNSET);
	off_t size;
	file.GetSize(&size);

	char* buffer = new char[size + 1];
	off_t len = file.Read(buffer, size);
	buffer[size] = '\0';
	SendMessage(SCI_CLEARALL, UNSET, UNSET);
	SendMessage(SCI_SETTEXT, 0, (sptr_t) buffer);
	delete[] buffer;

	if (readOnly == true)
		SendMessage(SCI_SETREADONLY, 1, UNSET);

	if (len != size)
		return B_ERROR;

	if ((status = file.Unlock()) != B_OK)
		return status;

	SendMessage(SCI_EMPTYUNDOBUFFER, UNSET, UNSET);
	SendMessage(SCI_SETSAVEPOINT, UNSET, UNSET);
	SendMessage(SCI_SETSEL, position, position);
	GrabFocus();

	return B_OK;
}

//TODO: too similar to ReplaceAndFindNext
int
Editor::ReplaceAndFindPrevious(const BString& selection,
									const BString& replacement, int flags, bool wrap)
{
	int retValue = REPLACE_NONE;
	int position = SendMessage(SCI_GETCURRENTPOS, UNSET, UNSET);

	if (IsSearchSelected(selection, flags) == true) {
		SendMessage(SCI_REPLACESEL, UNUSED, (sptr_t)replacement.String());
		SendMessage(SCI_SETTARGETRANGE, position + replacement.Length(), 0);
		retValue = REPLACE_DONE;
	} else {
		SendMessage(SCI_SETTARGETRANGE, position, 0);
	}

	position = SendMessage(SCI_SEARCHINTARGET, selection.Length(),
											(sptr_t) selection.String());

	if (position != -1) {
		SendMessage(SCI_SETSEL, position, position + selection.Length());
		retValue = REPLACE_DONE;
	} else if (wrap == true) {
		// position == -1: not found or reached file start, ensue second case
		int endDoc = SendMessage(SCI_GETTEXTLENGTH, UNSET, UNSET);
		SendMessage(SCI_SETTARGETRANGE, endDoc, 0);
		position = SendMessage(SCI_SEARCHINTARGET, selection.Length(),
												(sptr_t) selection.String());
		if (position != -1) {
			SendMessage(SCI_SETSEL, position, position + selection.Length());
			retValue = REPLACE_DONE;
		}
	}

	return retValue;
}


int
Editor::ReplaceAndFindNext(const BString& selection, const BString& replacement,
															int flags, bool wrap)
{
	int retValue = REPLACE_NONE;
	int position = SendMessage(SCI_GETCURRENTPOS, UNSET, UNSET);
	int endPosition = SendMessage(SCI_GETTEXTLENGTH, UNSET, UNSET);

	if (IsSearchSelected(selection, flags) == true) {
		//SendMessage(SCI_CHARRIGHT, UNSET, UNSET);
		SendMessage(SCI_REPLACESEL, UNUSED, (sptr_t)replacement.String());
		SendMessage(SCI_SETTARGETRANGE, position + replacement.Length(), endPosition);
		retValue = REPLACE_DONE;
	} else {
		SendMessage(SCI_SETTARGETRANGE, position, endPosition);
	}

	position = SendMessage(SCI_SEARCHINTARGET, selection.Length(),
											(sptr_t) selection.String());
	if (position != -1) {
		SendMessage(SCI_SETSEL, position, position + selection.Length());
		retValue = REPLACE_DONE;
	} else if (wrap == true) {
		// position == -1: not found or reached file end, ensue second case
		SendMessage(SCI_TARGETWHOLEDOCUMENT, UNSET, UNSET);
		position = SendMessage(SCI_SEARCHINTARGET, selection.Length(),
												(sptr_t) selection.String());
		if (position != -1) {
			SendMessage(SCI_SETSEL, position, position + selection.Length());
			retValue = REPLACE_DONE;
		}
	}

	return retValue;
}


int32
Editor::ReplaceAll(const BString& selection, const BString& replacement, int flags)
{
	int32 count = 0;

	SendMessage(SCI_TARGETWHOLEDOCUMENT, UNSET, UNSET);
	SendMessage(SCI_SETSEARCHFLAGS, flags, UNSET);

	int position;
	int endPosition = SendMessage(SCI_GETTARGETEND, 0, 0);

	SendMessage(SCI_BEGINUNDOACTION, 0, 0);

	do {
		position = SendMessage(SCI_SEARCHINTARGET, selection.Length(),
												(sptr_t) selection.String());
		if (position != -1) {
			SendMessage(SCI_REPLACETARGET, -1, (sptr_t) replacement.String());
			count++;

			// Found occurrence, message window
			ReplaceMessage(position, selection, replacement);

			SendMessage(SCI_SETTARGETRANGE, position + replacement.Length(), endPosition);
		}
	} while (position != -1);

	SendMessage(SCI_ENDUNDOACTION, 0, 0);

	BMessage message(EDITOR_REPLACE_ALL_COUNT);
	message.AddInt32("count", count);
	fTarget.SendMessage(&message);

	return count;
}


void
Editor::ReplaceMessage(int position, const BString& selection,
							const BString& replacement)
{
	BMessage message(EDITOR_REPLACE_ONE);
	message.AddRef("ref", &fFileRef);
	int line = SendMessage(SCI_LINEFROMPOSITION, position, UNSET) + 1;
	int column = SendMessage(SCI_GETCOLUMN, position, UNSET) + 1;
	column -= selection.Length();
	message.AddInt32("line", line);
	message.AddInt32("column", column);
	message.AddString("selection", selection);
	message.AddString("replacement", replacement);
	fTarget.SendMessage(&message);
}


int
Editor::ReplaceOne(const BString& selection, const BString& replacement)
{
	if (selection == Selection()) {
		SendMessage(SCI_REPLACESEL, UNUSED, (sptr_t)replacement.String());
		int position = SendMessage(SCI_GETCURRENTPOS, UNSET, UNSET);
		// Found occurrence, message window
		ReplaceMessage(position, selection, replacement);

		return REPLACE_DONE;
	}
	return REPLACE_NONE;
}


ssize_t
Editor::SaveToFile()
{
	BFile file;
	status_t status = file.SetTo(&fFileRef, B_READ_WRITE | B_ERASE_FILE | B_CREATE_FILE);
	if (status != B_OK)
		return 0;

	// TODO warn user
	if ((status = file.Lock()) != B_OK)
		return 0;

	off_t size = SendMessage(SCI_GETLENGTH, UNSET, UNSET);
	file.Seek(0, SEEK_SET);
	char* buffer = new char[size + 1];

	SendMessage(SCI_GETTEXT, size + 1, (sptr_t)buffer);

	off_t bytes = file.Write(buffer, size);
	file.Flush();

	// TODO warn user
	if ((status = file.Unlock()) != B_OK)
		return 0;

	delete[] buffer;

	SendMessage(SCI_SETSAVEPOINT, UNSET, UNSET);

	fLSPEditorWrapper->didSave();

	return bytes;
}


void
Editor::ScrollCaret()
{
	SendMessage(SCI_SCROLLCARET, UNSET, UNSET);
}


void
Editor::SelectAll()
{
	SendMessage(SCI_SELECTALL, UNSET, UNSET);
}


const BString
Editor::Selection()
{
	int32 size = SendMessage(SCI_GETSELTEXT, 0, 0);
	char text[size + 1];
	SendMessage(SCI_GETSELTEXT, 0, (sptr_t)text);
	return text;
}


const BString
Editor::GetSymbol()
{
	int32 position = SendMessage(SCI_GETCURRENTPOS);
	int32 start = SendMessage(SCI_WORDSTARTPOSITION, position);
	int32 end = SendMessage(SCI_WORDENDPOSITION, position);
	int32 size = end - start;
	char text[size];
	GetText(start, size, text);
	return text;
}


// it sends Selection/Position changes.
void
Editor::SendPositionChanges()
{
	int32 position = GetCurrentPosition();
	int line = SendMessage(SCI_LINEFROMPOSITION, position, UNSET) + 1;
	int column = SendMessage(SCI_GETCOLUMN, position, UNSET) + 1;
	fCurrentLine = line;
	fCurrentColumn = column;

	BMessage message(EDITOR_POSITION_CHANGED);
	message.AddRef("ref", &fFileRef);
	message.AddInt32("line", fCurrentLine);
	message.AddInt32("column", fCurrentColumn);
	fTarget.SendMessage(&message);
}


void
Editor::UpdateStatusBar()
{
	Sci_Position pos = SendMessage(SCI_GETCURRENTPOS, 0, 0);
	int line = SendMessage(SCI_LINEFROMPOSITION, pos, 0);
	int column = SendMessage(SCI_GETCOLUMN, pos, 0);
	BMessage update(editor::StatusView::UPDATE_STATUS);
	update.AddInt32("line", line + 1);
	update.AddInt32("column", column + 1);
	update.AddString("overwrite", IsOverwriteString());//EndOfLineString());
	update.AddString("readOnly", ModeString());
	update.AddString("eol", _EndOfLineString());
	update.AddString("editorconfig", fHasEditorConfig ? "\xF0\x9F\x97\x8E" : "\xF0\x9F\x8C\x90");
	update.AddString("trim_trialing_whitespace", fEditorConfig.TrimTrailingWhitespace ? "TRIM" : "NOTRIM");
	update.AddString("indent_style", fEditorConfig.IndentStyle == IndentStyle::Tab ? "TAB" : "SPACE");
	update.AddInt32("indent_size", fEditorConfig.IndentSize);

	fStatusView->SetStatus(&update);
}


status_t
Editor::SetFileRef(entry_ref* ref)
{
	if (ref == nullptr)
		return B_ERROR;

	fFileRef = *ref;
	fFileName = BString(fFileRef.name);

	UpdateStatusBar();
	return B_OK;
}


void
Editor::SetReadOnly(bool readOnly)
{
	if (!readOnly) {
		SendMessage(SCI_SETREADONLY, 0, UNSET);
		UpdateStatusBar();
		return;
	}

	if (IsModified()) {
		BString text(B_TRANSLATE("Save changes to file \"%filename%\"?"));
		text.ReplaceFirst("%filename%", Name());

		BAlert* alert = new BAlert(B_TRANSLATE("Save dialog"), text,
 			B_TRANSLATE("Cancel"), B_TRANSLATE("Don't save"), B_TRANSLATE("Save"),
 			B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);

		alert->SetShortcut(0, B_ESCAPE);

		int32 choice = alert->Go();
		if (choice == 0)
			return ;
		else if (choice == 2) {
			SaveToFile();
		}
	}

	fModified = false;

	SendMessage(SCI_SETREADONLY, 1, UNSET);
	UpdateStatusBar();
}


status_t
Editor::SetSavedCaretPosition()
{
	if (!gCFG["save_caret"])
		return B_ERROR; //TODO maybe tweak

	status_t status;
	// Get caret position
	BNode node(&fFileRef);
	if ((status = node.InitCheck()) != B_OK)
		return status;
	int32 pos = 0;
	ssize_t bytes = 0;
	bytes = node.ReadAttr("be:caret_position", B_INT32_TYPE, 0, &pos, sizeof(pos));
	if (bytes < (int32) sizeof(pos))
		return B_ERROR; //TODO maybe tweak + cast

	SendMessage(SCI_ENSUREVISIBLEENFORCEPOLICY,
			SendMessage(SCI_LINEFROMPOSITION, pos, UNSET), UNSET);

	SendMessage(SCI_GOTOPOS, pos, UNSET);

	return B_OK;
}


int
Editor::SetSearchFlags(bool matchCase, bool wholeWord, bool wordStart,
			bool regExp, bool posix)
{
	int flags = 0;
	if (matchCase == true)
		flags |= SCFIND_MATCHCASE;
	if (wholeWord == true)
		flags |= SCFIND_WHOLEWORD;
	if (wordStart == true)
		flags |= SCFIND_WORDSTART;

	return flags;
}


void
Editor::SetTarget(const BMessenger& target)
{
    fTarget = target;
}


status_t
Editor::StartMonitoring()
{
	// start monitoring this file for changes
	BEntry entry(&fFileRef, true);

	status_t status;
	if ((status = entry.GetNodeRef(&fNodeRef)) != B_OK) {
		LogErrorF("Can't get a node_ref! (%s) (%s)", fFileRef.name, strerror(status));
		return status;
	}
	if ((status = watch_node(&fNodeRef, B_WATCH_NAME | B_WATCH_STAT, fTarget)) != B_OK) {
		LogErrorF("Can't start watch_node a node_ref! (%s) (%s)", fFileRef.name, strerror(status));
		return status;
	}
	return	B_OK;
}


status_t
Editor::StopMonitoring()
{
	status_t status;
	if ((status = watch_node(&fNodeRef, B_STOP_WATCHING, fTarget)) != B_OK) {
		LogErrorF("Can't stop watch_node a node_ref! (%s) (%s)", fFileRef.name, strerror(status));
		return status;
	}
	return B_OK;
}


void
Editor::ToggleFolding()
{
	SendMessage(SCI_FOLDALL, SC_FOLDACTION_TOGGLE, UNSET);
}


void
Editor::ShowLineEndings(bool show)
{
	SendMessage(SCI_SETVIEWEOL, show ? 1 : 0, UNSET);
}


void
Editor::ShowWhiteSpaces(bool show)
{
	SendMessage(SCI_SETVIEWWS, show ? SCWS_VISIBLEALWAYS : SCWS_INVISIBLE, UNSET);
}


bool
Editor::LineEndingsVisible()
{
	return SendMessage(SCI_GETVIEWEOL, UNSET, UNSET);
}


bool
Editor::WhiteSpacesVisible()
{
	return !(SendMessage(SCI_GETVIEWWS, UNSET, UNSET) == SCWS_INVISIBLE);
}


void
Editor::Undo()
{
	SendMessage(SCI_UNDO, UNSET, UNSET);
}


void
Editor::Completion()
{
	fLSPEditorWrapper->StartCompletion();
}


void
Editor::Format()
{
	fLSPEditorWrapper->Format();
}


void
Editor::GoToDefinition()
{
	fLSPEditorWrapper->GoTo(LSPEditorWrapper::GOTO_DEFINITION);
}


void
Editor::GoToDeclaration()
{
	fLSPEditorWrapper->GoTo(LSPEditorWrapper::GOTO_DECLARATION);
}


void
Editor::GoToImplementation()
{
	fLSPEditorWrapper->GoTo(LSPEditorWrapper::GOTO_IMPLEMENTATION);
}

void
Editor::Rename()
{
	// Getting the symbol from the language server would require many async steps.
	// We instead ask Scintilla to deliver it which should be almost if not entirely accurate

	// remove invalid leading characters
	const std::string symbol = GetSymbol().String();
	const std::regex leadingChars("^\\W+");
	std::string str = std::regex_replace(symbol, leadingChars, "");

	BString label(B_TRANSLATE("Rename symbol '%symbol_name%':"));
	label.ReplaceFirst("%symbol_name%", str.c_str());

	auto alert = new GTextAlert(B_TRANSLATE("Rename"), label, str.c_str());
	auto result = alert->Go();
	if (result.Button == GAlertButtons::OkButton)
		fLSPEditorWrapper->Rename(result.Result.String());
}


void
Editor::SwitchSourceHeader()
{
	entry_ref foundRef;
	if (FindSourceOrHeader(&fFileRef, &foundRef) == B_OK) {
		BMessage refs(B_REFS_RECEIVED);
        refs.AddRef("refs", &foundRef);
        be_app->PostMessage(&refs);
	}
}


void
Editor::SetProjectFolder(ProjectFolder* proj)
{
	fProjectFolder = proj;
	if (proj) {
		LSPProjectWrapper* lspProject = proj->GetLSPServer(fFileType.c_str());
		if (lspProject)
			fLSPEditorWrapper->SetLSPServer(lspProject);
		else
			fLSPEditorWrapper->UnsetLSPServer();
	}
	else
		fLSPEditorWrapper->UnsetLSPServer();

	// BMessage empty;
	// SetProblems(&empty);
	SetProblems();
}


void
Editor::SetZoom(int32 zoom)
{
	SendMessage(SCI_SETZOOM, zoom, 0);
	_RedrawNumberMargin(true);
}


void
Editor::ContextMenu(BPoint point)
{
	EditorContextMenu::Show(this, point);
}


void
Editor::_ApplyExtensionSettings()
{
	BFont font = be_fixed_font;
	BString fontFamily = gCFG["edit_fontfamily"];
	if (!fontFamily.IsEmpty()){
		font.SetFamilyAndStyle(fontFamily, nullptr);
	}
	int32 fontSize = gCFG["edit_fontsize"];
	if (fontSize > 0)
		font.SetSize(fontSize);

	Styler::ApplyGlobal(this, gCFG["editor_style"], &font);

	if (gCFG["syntax_highlight"] && fFileType != "") {
		fSyntaxAvailable = true;
		fFoldingAvailable = true;
		fBracingAvailable = true;
		fParsingAvailable = true;
		fCommenter = "";

		auto styles = Languages::ApplyLanguage(this, fFileType.c_str());
		Styler::ApplyLanguage(this, styles);
	}
}


void
Editor::_MaintainIndentation(char ch)
{
	int eolMode = SendMessage(SCI_GETEOLMODE, 0, 0);
	int currentLine = SendMessage(SCI_LINEFROMPOSITION, SendMessage(SCI_GETCURRENTPOS, 0, 0), 0);
	int lastLine = currentLine - 1;

	if (((eolMode == SC_EOL_CRLF || eolMode == SC_EOL_LF) && ch == '\n') ||
		(eolMode == SC_EOL_CR && ch == '\r')) {
		int indentAmount = 0;
		if (lastLine >= 0) {
			indentAmount = SendMessage(SCI_GETLINEINDENTATION, lastLine, 0);
		}
		if (indentAmount > 0) {
			_SetLineIndentation(currentLine, indentAmount);
		}
	}
}


void
Editor::_SetLineIndentation(int line, int indent)
{
	if (indent < 0)
		return;

	Sci_CharacterRange crange;
	std::tie(crange.cpMin, crange.cpMax) = Get<Sci::Properties::Selection>();
	Sci_CharacterRange crangeStart = crange;
	int posBefore = SendMessage(SCI_GETLINEINDENTPOSITION, line, 0);
	SendMessage(SCI_SETLINEINDENTATION, line, indent);
	int posAfter = SendMessage(SCI_GETLINEINDENTPOSITION, line, 0);
	int posDifference = posAfter - posBefore;
	if (posAfter > posBefore) {
		if (crange.cpMin >= posBefore) {
			crange.cpMin += posDifference;
		}
		if (crange.cpMax >= posBefore) {
			crange.cpMax += posDifference;
		}
	} else if(posAfter < posBefore) {
		if (crange.cpMin >= posAfter) {
			if (crange.cpMin >= posBefore) {
				crange.cpMin += posDifference;
			} else {
				crange.cpMin = posAfter;
			}
		}
		if (crange.cpMax >= posAfter) {
			if (crange.cpMax >= posBefore) {
				crange.cpMax += posDifference;
			} else {
				crange.cpMax = posAfter;
			}
		}
	}
	if ((crangeStart.cpMin != crange.cpMin) || (crangeStart.cpMax != crange.cpMax)) {
		Set<Sci::Properties::Selection>({static_cast<int>(crange.cpMin), static_cast<int>(crange.cpMax)});
	}
}


void
Editor::_BraceHighlight()
{
	if (fBracingAvailable == true) {
		Sci_Position pos = SendMessage(SCI_GETCURRENTPOS, 0, 0);
		// highlight indent guide
		int line = SendMessage(SCI_LINEFROMPOSITION, pos, 0);
		int indentation = SendMessage(SCI_GETLINEINDENTATION, line, 0);
		SendMessage(SCI_SETHIGHLIGHTGUIDE, indentation, 0);
		// highlight braces
		if (_BraceMatch(pos - 1) == false) {
			_BraceMatch(pos);
		}
	} else {
		SendMessage(SCI_BRACEBADLIGHT, -1, 0);
	}
}


bool
Editor::_BraceMatch(int pos)
{
	char ch = SendMessage(SCI_GETCHARAT, pos, 0);
	if (ch == '(' || ch == ')' || ch == '[' || ch == ']' || ch == '{' || ch == '}') {
		int match = SendMessage(SCI_BRACEMATCH, pos, 0);
		if (match == -1) {
			SendMessage(SCI_BRACEBADLIGHT, pos, 0);
		} else {
			SendMessage(SCI_BRACEHIGHLIGHT, pos, match);
		}
	} else {
		SendMessage(SCI_BRACEBADLIGHT, -1, 0);
		return false;
	}
	return true;
}


// Leave line leading spaces when commenting/decommenting
// Erase spaces from comment char(s) to first non-space when decommenting
void
Editor::_CommentLine(int32 position)
{
	if (fCommenter.empty())
		return;

	const std::string lineCommenter = fCommenter + ' ';
	int32 lineNumber = SendMessage(SCI_LINEFROMPOSITION, position, UNSET);
	int32 lineLength = SendMessage(SCI_LINELENGTH, lineNumber, UNSET);
	char *lineBuffer = new char[lineLength];
	SendMessage(SCI_GETLINE, lineNumber, (sptr_t)lineBuffer);
	std::string line(lineBuffer);
	delete[] lineBuffer;

	// Calculate offset of first non-space
	std::size_t offset = line.find_first_not_of("\t ");
	if (offset == std::string::npos)
		return;

	if (line.substr(offset, fCommenter.length()) != fCommenter) {
		// Not starting with a comment, comment out
		SendMessage(SCI_INSERTTEXT, position + offset, (sptr_t)lineCommenter.c_str());
	} else {
		 // It was a comment line, decomment erasing commenter trailing spaces
		std::size_t spaces = line.substr(offset + fCommenter.length()).find_first_not_of("\t ");
		SendMessage(SCI_DELETERANGE, position + offset, fCommenter.length() + spaces);
	}
}


void
Editor::DuplicateCurrentLine()
{
	int32 lineNumber = SendMessage(SCI_LINEFROMPOSITION, GetCurrentPosition(), UNSET);
	SendMessage(SCI_LINEDUPLICATE, lineNumber, UNSET);
}


void
Editor::DeleteSelectedLines() {
	SendMessage(SCI_BEGINUNDOACTION, 0, UNSET);
	int32 start = SendMessage(SCI_GETSELECTIONSTART, 0, UNSET);
	int32 startLineNumber = SendMessage(SCI_LINEFROMPOSITION, start, UNSET);
	int32 end = SendMessage(SCI_GETSELECTIONEND, 0, UNSET);
	int32 endLineNumber = SendMessage(SCI_LINEFROMPOSITION, end, UNSET);
	for (int32 i = startLineNumber; i <= endLineNumber; i++) {
		SendMessage(SCI_LINEDELETE, i, UNSET);
	}
	SendMessage(SCI_ENDUNDOACTION, 0, UNSET);
}


void
Editor::CommentSelectedLines()
{
	int32 start = SendMessage(SCI_GETSELECTIONSTART, 0, UNSET);
	int32 startLineNumber = SendMessage(SCI_LINEFROMPOSITION, start, UNSET);
	int32 end = SendMessage(SCI_GETSELECTIONEND, 0, UNSET);
	int32 endLineNumber = SendMessage(SCI_LINEFROMPOSITION, end, UNSET);
	SendMessage(SCI_BEGINUNDOACTION, 0, UNSET);
	for (int32 i = startLineNumber; i <= endLineNumber; i++) {
		int32 position = SendMessage(SCI_POSITIONFROMLINE, i, UNSET);
		_CommentLine(position);
	}
	SendMessage(SCI_ENDUNDOACTION, 0, UNSET);
}


int32
Editor::EndOfLine()
{
	return SendMessage(SCI_GETEOLMODE, UNSET, UNSET);
}


void
Editor::_EndOfLineAssign(char *buffer, int32 size)
{
	// Empty file, use default LF
	if (size == 0) {
		SendMessage(SCI_SETEOLMODE, SC_EOL_LF, UNSET);
		return;
	}

	int32 eol;
	switch (buffer[size - 1]) {
		case '\n':
			if (size > 1 && buffer[size - 2] == '\r')
				eol = SC_EOL_CRLF;
			else
				eol = SC_EOL_LF;
			break;
		case '\r':
			eol = SC_EOL_CR;
			break;
		default:
			eol = SC_EOL_LF;
			break;
	}

	SendMessage(SCI_SETEOLMODE, eol, UNSET);
}


void
Editor::_HighlightBraces()
{
	if (fBracingAvailable == true) {
		SendMessage(SCI_STYLESETFORE, STYLE_BRACELIGHT, 0xFF0000);
		SendMessage(SCI_STYLESETBOLD, STYLE_BRACELIGHT, 1);
		SendMessage(SCI_STYLESETFORE, STYLE_BRACEBAD, 0x0000FF);
		SendMessage(SCI_STYLESETBOLD, STYLE_BRACEBAD, 1);
		// Indentation guides
		SendMessage(SCI_STYLESETFORE, STYLE_INDENTGUIDE, 0xA0A0A0);
		SendMessage(SCI_SETINDENTATIONGUIDES, SC_IV_REAL, UNSET);
	}
}


void
Editor::_RedrawNumberMargin(bool forced)
{
	if (!gCFG["show_linenumber"]) {
		SendMessage(SCI_SETMARGINWIDTHN, sci_NUMBER_MARGIN, 0);
		return;
	}

	int linesLog10 = log10(SendMessage(SCI_GETLINECOUNT, UNSET, UNSET));
	linesLog10 += 2;

	if (linesLog10 != fLinesLog10 || forced) {
		fLinesLog10 = linesLog10;
		float zoom = 1 + ((float)SendMessage(SCI_GETZOOM)/100.0);
		int pixelWidth = SendMessage(SCI_TEXTWIDTH, STYLE_LINENUMBER, (sptr_t) "9") * zoom;
		SendMessage(SCI_SETMARGINWIDTHN, sci_NUMBER_MARGIN, pixelWidth * fLinesLog10);
	}
}


void
Editor::_SetFoldMargin(bool enabled)
{
	const int foldEnabled = SendMessage(SCI_GETPROPERTYINT, (uptr_t) "fold", 0);
	const int32 fontSize = SendMessage(SCI_STYLEGETSIZE, 32);
	const int32 foldWidth = IsFoldingAvailable() && foldEnabled && enabled ? fontSize * 0.95 : 0;
	SendMessage(SCI_SETMARGINWIDTHN, sci_FOLD_MARGIN, foldWidth);
}


void
Editor::SetProblems()
{
	// make absolutely sure we're locked
	if (!Window()->IsLocked()) {
		debugger("The looper must be locked !");
	}
	fProblems.what = EDITOR_UPDATE_DIAGNOSTICS;
	fProblems.AddRef("ref", &fFileRef);
	Window()->PostMessage(&fProblems);
}


void
Editor::SetDocumentSymbols(const BMessage* symbols)
{
	// make absolutely sure we're locked
	if (!Window()->IsLocked()) {
		debugger("The looper must be locked !");
	}
	if (symbols->HasMessage("symbol"))
		fSymbolsStatus = STATUS_HAS_SYMBOLS;
	else
		fSymbolsStatus = STATUS_NO_SYMBOLS;

	fDocumentSymbols = *symbols;
	fDocumentSymbols.what = EDITOR_UPDATE_SYMBOLS;
	fDocumentSymbols.AddRef("ref", &fFileRef);
	fDocumentSymbols.AddInt32("status", fSymbolsStatus);

	std::set<std::pair<std::string, int32> >::const_iterator iterator;
	for (iterator = fCollapsedSymbols.begin(); iterator != fCollapsedSymbols.end(); iterator++) {
		fDocumentSymbols.AddString("collapsed_name", iterator->first.c_str());
		fDocumentSymbols.AddInt32("collapsed_kind", iterator->second);
	}
	Window()->PostMessage(&fDocumentSymbols);
}


void
Editor::GetDocumentSymbols(BMessage* symbols) const
{
	// make absolutely sure we're locked
	if (!Window()->IsLocked()) {
		debugger("The looper must be locked !");
	}

	*symbols = fDocumentSymbols;

	if (!symbols->HasRef("ref")) {
		// Always add ref so we can identify the file (in FunctionsOutlineView)
		symbols->AddRef("ref", &fFileRef);
	}

	if (!symbols->HasInt32("status"))
		symbols->AddInt32("status", fSymbolsStatus);

	// TODO: Refactor:
	// we should add the "collapsed" property to the symbol, so that
	// we can do a single pass in the Outline view
	symbols->RemoveName("collapsed_name");
	symbols->RemoveName("collapsed_kind");
	std::set<std::pair<std::string, int32> >::const_iterator iterator;
	for (iterator = fCollapsedSymbols.begin(); iterator != fCollapsedSymbols.end(); iterator++) {
		symbols->AddString("collapsed_name", iterator->first.c_str());
		symbols->AddInt32("collapsed_kind", iterator->second);
	}
}

void
Editor::EvaluateIdleTime()
{
	if (fIdleHandler == nullptr || fIdleHandler->SetInterval(kIdleTimeout) != B_OK) {
		LogInfo("EvaluateIdleTime: Re-arming IdleHandler...");
		if (fIdleHandler != nullptr)
			delete fIdleHandler;

		// create a message to update the project
		BMessage message(kIdle);
		fIdleHandler = new BMessageRunner(BMessenger(this), &message, kIdleTimeout, 1);
		if (fIdleHandler->InitCheck() != B_OK) {
			LogInfo("EvaluateIdleTime: Could not create fIdleHandler. Deleting it");
			if (fIdleHandler != nullptr) {
				delete fIdleHandler;
				fIdleHandler = nullptr;
			}
		} else {
			LogInfo("EvaluateIdleTime: fIdleHandler re-armed.");
		}
	}
}