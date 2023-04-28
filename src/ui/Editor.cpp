/*
 * Copyright 2017 A. Mosca <amoscaster@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "Editor.h"

#include <Alert.h>
#include <Application.h>
#include <Catalog.h>
#include <Control.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <ILexer.h>
#include <SciLexer.h>
#include <Lexilla.h>
#include <Volume.h>

#include <iostream>
#include <sstream>

#include "GenioCommon.h"
#include "GenioNamespace.h"
#include "keywords.h"
#include "ProjectFolder.h"
#include "EditorContextMenu.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Editor"

using namespace GenioNames;

// Differentiate unset parameters from 0 ones
// in scintilla messages
#define UNSET 0
#define UNUSED 0

#include "FileWrapper.h"
#include "EditorStatusView.h"

//#define USE_LINEBREAKS_ATTRS

Editor::Editor(entry_ref* ref, const BMessenger& target)
	:
	BScintillaView(ref->name, 0, true, true)
	, fFileRef(*ref)
	, fModified(false)
	, fBraceHighlighted(kNoBrace)
	, fBracingAvailable(false)
	, fFoldingAvailable(false)
	, fSyntaxAvailable(false)
	, fParsingAvailable(false)
	, fCommenter("")
	, fCurrentLine(-1)
	, fCurrentColumn(-1)
	, fProjectFolder(NULL)
{
	fStatusView = new editor::StatusView(this);
	fFileName = BString(ref->name);
	SetTarget(target);

	// Filter notifying changes
/*	SendMessage(SCI_SETMODEVENTMASK,SC_MOD_INSERTTEXT
									 | SC_MOD_DELETETEXT
									 | SC_MOD_CHANGESTYLE
									 | SC_MOD_CHANGEFOLD
									 | SC_PERFORMED_USER
									 | SC_PERFORMED_UNDO
									 | SC_PERFORMED_REDO
									 | SC_MULTISTEPUNDOREDO
									 | SC_LASTSTEPINUNDOREDO
									 | SC_MOD_CHANGEMARKER
									 | SC_MOD_BEFOREINSERT
									 | SC_MOD_BEFOREDELETE
									 | SC_MULTILINEUNDOREDO
									 | SC_MODEVENTMASKALL
									 , UNSET);
*/

// SendMessage(SCI_SETYCARETPOLICY, CARET_SLOP | CARET_STRICT | CARET_JUMPS, 20);
// CARET_SLOP  CARET_STRICT  CARET_JUMPS  CARET_EVEN
	std::string s = "file://";
	s += BPath(&fFileRef).Path();//.String();
	fFileWrapper = new FileWrapper(s.c_str(), this);
	
}

Editor::~Editor()
{
	// Stop monitoring
	StopMonitoring();

	// Set caret position
	if (Settings.save_caret == true) {
		BNode node(&fFileRef);
		if (node.InitCheck() == B_OK) {
			int32 pos = GetCurrentPosition();
			node.WriteAttr("be:caret_position", B_INT32_TYPE, 0, &pos, sizeof(pos));
		}
	}
	
	fFileWrapper->UnsetLSPClient();
	delete fFileWrapper;
	fFileWrapper = NULL;
}

void
Editor::MessageReceived(BMessage* message)
{
	switch (message->what) {

		default:
			BScintillaView::MessageReceived(message);
			break;
	}
}

void
Editor::ApplySettings()
{
	// White spaces color
	SendMessage(SCI_SETWHITESPACESIZE, 4, UNSET);
	SendMessage(SCI_SETWHITESPACEFORE, 1, kWhiteSpaceFore);
	SendMessage(SCI_SETWHITESPACEBACK, 1, kWhiteSpaceBack);

	// Selection background
	SendMessage(SCI_SETSELBACK, 1, kSelectionBackColor);

	// Font & Size
	SendMessage(SCI_STYLESETFONT, STYLE_DEFAULT, (sptr_t) "Noto Sans Mono");
	int32 fontSize = Settings.edit_fontsize;
	if (fontSize < 0)
		fontSize = be_plain_font->Size();
	SendMessage(SCI_STYLESETSIZE, STYLE_DEFAULT, fontSize);
	SendMessage(SCI_STYLECLEARALL, UNSET, UNSET);

	// Highlighting
	if (Settings.syntax_highlight == B_CONTROL_ON) {
		_ApplyExtensionSettings();
		_HighlightFile();
	}
	// Brace match
	if (Settings.brace_match == B_CONTROL_ON)
		_HighlightBraces();

	// Caret line visible
	if (Settings.mark_caretline == true) {
		SendMessage(SCI_SETCARETLINEVISIBLE, 1, UNSET);
		SendMessage(SCI_SETCARETLINEBACK, kCaretLineBackColor, UNSET);
	}

	// Edge line
	if (Settings.show_edgeline == true) {
		SendMessage(SCI_SETEDGEMODE, EDGE_LINE, UNSET);

		std::string column(Settings.edgeline_column);
		int32 col;
		std::istringstream (column) >>  col;
		SendMessage(SCI_SETEDGECOLUMN, col, UNSET);
		SendMessage(SCI_SETEDGECOLOUR, kEdgeColor, UNSET);
	}

	// Tab width
	if (fFileType == "rust") {
		// Use rust style (4 spaces)
		SendMessage(SCI_SETTABWIDTH, 4, UNSET);
		SendMessage(SCI_SETUSETABS, false, UNSET);
	} else
		SendMessage(SCI_SETTABWIDTH, Settings.tab_width, UNSET);

	// MARGINS
	SendMessage(SCI_SETMARGINS, 4, UNSET);
	SendMessage(SCI_STYLESETBACK, STYLE_LINENUMBER, kLineNumberBack);

	SendMessage(SCI_SETMOUSEDWELLTIME, 1000);
	

	// Bookmark margin
	SendMessage(SCI_SETMARGINTYPEN, sci_BOOKMARK_MARGIN, SC_MARGIN_SYMBOL);
	SendMessage(SCI_SETMARGINSENSITIVEN, sci_BOOKMARK_MARGIN, 1);
	SendMessage(SCI_MARKERDEFINE, sci_BOOKMARK, SC_MARK_BOOKMARK);
	SendMessage(SCI_MARKERSETFORE, sci_BOOKMARK, kMarkerForeColor);
	SendMessage(SCI_MARKERSETBACK, sci_BOOKMARK, kMarkerBackColor);
 	SendMessage(SCI_SETMARGINMASKN, sci_BOOKMARK_MARGIN, (1 << sci_BOOKMARK));

	// Folding
	if (Settings.enable_folding == B_CONTROL_ON)
		_SetFoldMargin();

	// Line commenter margin
	if (Settings.show_commentmargin == true && !fCommenter.empty()) {
		SendMessage(SCI_SETMARGINWIDTHN, sci_COMMENT_MARGIN, 12);
		SendMessage(SCI_SETMARGINSENSITIVEN, sci_COMMENT_MARGIN, 1);
	}
	
	fFileWrapper->ApplySettings();
	
	//custom ContextMenu!
	SendMessage(SCI_USEPOPUP, SC_POPUP_NEVER, 0);
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
	if(bookmark == -1)
		bookmark = SendMessage(SCI_MARKERNEXT, 0, (1 << sci_BOOKMARK));
	if(bookmark != -1)
		GoToLine(bookmark+1);
	
	return true;
}

bool
Editor::BookmarkGoToPrevious()
{
	Sci_Position pos = SendMessage(SCI_GETCURRENTPOS);
	int64 line = SendMessage(SCI_LINEFROMPOSITION, pos);
	int64 bookmark = SendMessage(SCI_MARKERPREVIOUS, line - 1, (1 << sci_BOOKMARK));
	if(bookmark == -1)
		bookmark = SendMessage(SCI_MARKERPREVIOUS, SendMessage(SCI_GETLINECOUNT), (1 << sci_BOOKMARK));
	if(bookmark != -1)
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
Editor::EndOfLineString()
{
	int32 eolMode = _EndOfLine();

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
Editor::Find(const BString&  text, int flags, bool backwards /* = false */)
{
	int position;

	SendMessage(SCI_SEARCHANCHOR, UNSET, UNSET);

	if (backwards == false)
		position = SendMessage(SCI_SEARCHNEXT, flags, (sptr_t) text.String());
	else
		position = SendMessage(SCI_SEARCHPREV, flags, (sptr_t) text.String());

	if (position != -1) {
		SendMessage(SCI_ENSUREVISIBLEENFORCEPOLICY,
			SendMessage(SCI_LINEFROMPOSITION, position, UNSET), UNSET);

		SendMessage(SCI_SCROLLCARET, UNSET, UNSET);
	}

	return position;
}

int
Editor::FindInTarget(const BString& search, int flags, int startPosition, int endPosition)
{
	SendMessage(SCI_SETTARGETSTART, startPosition, UNSET);
	SendMessage(SCI_SETTARGETEND, endPosition, UNSET);
	SendMessage(SCI_SETSEARCHFLAGS, flags, UNSET);
	int position = SendMessage(SCI_SEARCHINTARGET, search.Length(), (sptr_t) search.String());

	return position;
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

static bool sFound = false;

int
Editor::FindNext(const BString& search, int flags, bool wrap)
{
	if (sFound == true || IsSearchSelected(search, flags) == true)
		SendMessage(SCI_CHARRIGHT, UNSET, UNSET);

	int position = Find(search, flags);

	if (position != -1)
		sFound = true;
	else if (position == -1 && wrap == false) {
		sFound = false;
		BMessage message(EDITOR_FIND_NEXT_MISS);
		fTarget.SendMessage(&message);
	} else if (position == -1 && wrap == true) {
		sFound = false;
		// If wrap and not found go to saved position
		int savedPosition = SendMessage(SCI_GETCURRENTPOS, UNSET, UNSET);
		SendMessage(SCI_SETSEL, 0, 0);
		position = Find(search, flags);
		if (position == -1)
			SendMessage(SCI_GOTOPOS, savedPosition, 0);
	}
	return position;
}

int
Editor::FindPrevious(const BString& search, int flags, bool wrap)
{
	if (sFound == true)
		SendMessage(SCI_CHARLEFT, 0, 0);

	int position = Find(search, flags, true);

	if (position != -1)
		sFound = true;
	else if (position == -1 && wrap == false) {
		sFound = false;
		BMessage message(EDITOR_FIND_PREV_MISS);
		fTarget.SendMessage(&message);
	} else if (position == -1 && wrap == true) {
		sFound = false;
		// If wrap and not found go to saved position
		int savedPosition = SendMessage(SCI_GETCURRENTPOS, UNSET, UNSET);
		int endPosition = SendMessage(SCI_GETTEXTLENGTH, UNSET, UNSET);
		SendMessage(SCI_SETSEL, endPosition, endPosition);
		position = Find(search, flags, true);
		if (position == -1)
			SendMessage(SCI_GOTOPOS, savedPosition, 0);
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
	SendMessage(SCI_ENSUREVISIBLEENFORCEPOLICY, line, UNSET);
	SendMessage(SCI_GOTOLINE, line, UNSET);
	GrabFocus();
}

void
Editor::GoToLSPPosition(int32 line, int character)
{
  Sci_Position  sci_position;
  sci_position = SendMessage(SCI_POSITIONFROMLINE, line, 0);
  sci_position = SendMessage(SCI_POSITIONRELATIVE, sci_position, character);
  SendMessage(SCI_SETSEL, sci_position, sci_position);
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

/*
 * Code (editable) taken from stylededit
 */
status_t
Editor::LoadFromFile()
{
	status_t status;
	BFile file;
	struct stat st;

	if ((status = file.SetTo(&fFileRef, B_READ_ONLY)) != B_OK)
		return status;
	if ((status = file.InitCheck()) != B_OK)
		return status;
	if ((status = file.Lock()) != B_OK)
		return status;
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

	//fFileWrapper->didOpen(this, false);
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

	fFileType = Genio::file_type(fFileName.String());

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
			if (notification->ch == '\n' ||
					(notification->ch == '\r' &&
					SendMessage(SCI_GETEOLMODE, UNSET, UNSET) == SC_EOL_CR)) {
				_AutoIndentLine(); // TODO asociate extensions?
			}
			if (notification->characterSource == SC_CHARACTERSOURCE_DIRECT_INPUT)
				fFileWrapper->CharAdded(notification->ch);
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
		case SCN_AUTOCSELECTION: {
			fFileWrapper->SelectedCompletion(notification->text);
			break;
		}
		case SCN_MODIFIED: {
			if (notification->modificationType & SC_MOD_INSERTTEXT) {
				fFileWrapper->didChange(notification->text, notification->length, notification->position, 0);
			} 
			if (notification->modificationType & SC_MOD_BEFOREDELETE) {
				fFileWrapper->didChange("", 0, notification->position, notification->length);
			}
			if (notification->modificationType & SC_MOD_DELETETEXT && notification->length == 1) {
				if (SendMessage(SCI_CALLTIPACTIVE))
					fFileWrapper->ContinueCallTip();
			}
			if (notification->linesAdded != 0)
				if (Settings.show_linenumber == true)
					_RedrawNumberMargin(false);
			break;
		}
		case SCN_CALLTIPCLICK: {
			fFileWrapper->UpdateCallTip(notification->position);
			break;
		}
		case SCN_DWELLSTART: {
			fFileWrapper->StartHover(notification->position);
			break;
		}
		case SCN_DWELLEND: {
			fFileWrapper->EndHover();
			break;
		}
		case SCN_INDICATORRELEASE: {
			fFileWrapper->IndicatorClick(notification->position);
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

			// Do not trigger brace match on selection
			// as it flickers more on line selection
			if (IsTextSelected() == false  && fBracingAvailable == true)
				_CheckForBraceMatching();

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

	return B_OK;
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
	if(position != -1) {
		SendMessage(SCI_SETSEL, position, position + selection.Length());
		retValue = REPLACE_DONE;
	} else if (wrap == true) {
		// position == -1: not found or reached file end, ensue second case
		SendMessage(SCI_TARGETWHOLEDOCUMENT, UNSET, UNSET);

		position = SendMessage(SCI_SEARCHINTARGET, selection.Length(),
												(sptr_t) selection.String());
		if(position != -1) {
			SendMessage(SCI_SETSEL, position, position + selection.Length());
			retValue = REPLACE_DONE;
		}
	}

	return retValue;
}

/*
 * Adapted from Koder EditorWindow::_FindReplace
 */
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
		if(position != -1) {
			SendMessage(SCI_REPLACETARGET, -1, (sptr_t) replacement.String());
			count++;

			// Found occurrence, message window
			ReplaceMessage(position, selection, replacement);

			SendMessage(SCI_SETTARGETRANGE, position + replacement.Length(), endPosition);
		}
	} while(position != -1);

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
	status_t status;

	status = file.SetTo(&fFileRef, B_READ_WRITE | B_ERASE_FILE | B_CREATE_FILE);
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
	
	fFileWrapper->didSave();

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
	update.AddString("eol", EndOfLineString());
	
	fStatusView->SetStatus(&update);
}

void
Editor::SetEndOfLine(int32 eolFormat)
{
	if (IsReadOnly() == true)
		return;

	SendMessage(SCI_SETEOLMODE, eolFormat, UNSET);

#ifdef USE_LINEBREAKS_ATTRS
	BNode node(&fFileRef);
	node.WriteAttr("be:line_breaks", B_INT32_TYPE, 0, &eolFormat, sizeof(eolFormat));
#endif
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
Editor::SetReadOnly()
{
	if (IsModified()) {
		BString text(B_TRANSLATE("Save changes to file"));
		text << " \"" << Name() << "\" ?";
		
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
	if (Settings.save_caret == false)
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
	status_t status;

	// start monitoring this file for changes
	BEntry entry(&fFileRef, true);

	if ((status = entry.GetNodeRef(&fNodeRef)) != B_OK)
		return status;

	return	watch_node(&fNodeRef, B_WATCH_NAME | B_WATCH_STAT, fTarget);
}

status_t
Editor::StopMonitoring()
{
	return watch_node(&fNodeRef, B_STOP_WATCHING, fTarget);
}

void
Editor::ToggleFolding()
{
	SendMessage(SCI_FOLDALL, SC_FOLDACTION_TOGGLE, UNSET);
}

void
Editor::ToggleLineEndings()
{
	if (SendMessage(SCI_GETVIEWEOL, UNSET, UNSET) == false)
		SendMessage(SCI_SETVIEWEOL, 1, UNSET);
	else
		SendMessage(SCI_SETVIEWEOL, 0, UNSET);
}

void
Editor::ToggleWhiteSpaces()
{
	if (SendMessage(SCI_GETVIEWWS, UNSET, UNSET) == SCWS_INVISIBLE)
		SendMessage(SCI_SETVIEWWS, SCWS_VISIBLEALWAYS, UNSET);
	else
		SendMessage(SCI_SETVIEWWS, SCWS_INVISIBLE, UNSET);
}

void
Editor::Undo()
{
	SendMessage(SCI_UNDO, UNSET, UNSET);
}

void

Editor::Completion()
{
	fFileWrapper->StartCompletion();
}
		
void
Editor::Format()
{
	fFileWrapper->Format();
}

void
Editor::GoToDefinition()
{
	fFileWrapper->GoTo(FileWrapper::GOTO_DEFINITION);
}

void
Editor::GoToDeclaration()
{
	fFileWrapper->GoTo(FileWrapper::GOTO_DECLARATION);
}

void
Editor::GoToImplementation()
{
	fFileWrapper->GoTo(FileWrapper::GOTO_IMPLEMENTATION);
}

void
Editor::SwitchSourceHeader()
{
	fFileWrapper->SwitchSourceHeader();
}

void
Editor::SignatureHelp()
{
	fFileWrapper->SignatureHelp();
}

void
Editor::SetProjectFolder(ProjectFolder* proj)
{
	fProjectFolder = proj;
	if (proj)
		fFileWrapper->SetLSPClient(proj->GetLSPClient());
	else
		fFileWrapper->UnsetLSPClient();
	
	BMessage empty;
	SetProblems(&empty);
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
	if (fFileType == "c++") {
		fSyntaxAvailable = true;
		fFoldingAvailable = true;
		fBracingAvailable = true;
		fParsingAvailable = true;
		fCommenter = "//";
		SendMessage(SCI_SETILEXER, 0, (sptr_t)CreateLexer("cpp"));
		SendMessage(SCI_SETKEYWORDS, 0, (sptr_t)cppKeywords);
		SendMessage(SCI_SETKEYWORDS, 1, (sptr_t)haikuClasses);
	} else if (fFileType == "rust") {
		fSyntaxAvailable = true;
		fFoldingAvailable = true;
		fBracingAvailable = true;
		fCommenter = "//";
		SendMessage(SCI_SETILEXER, 0, (sptr_t)CreateLexer("rust"));
		SendMessage(SCI_SETKEYWORDS, 0, (sptr_t)rustKeywords);
	} else if (fFileType == "make") {
		fSyntaxAvailable = true;
		fBracingAvailable = true;
		fCommenter = "#";
		SendMessage(SCI_SETILEXER, 0, (sptr_t)CreateLexer("make"));
	}
}

/*
 * TODO: tune better and extensions management
 *
 */
void
Editor::_AutoIndentLine()
{
	auto tabInsertions = 0;
	auto charInsertions = 0;

	BString lineIndent = "\t";
	if (fFileType == "rust")
		lineIndent = "    ";

	// Get current line number
	int32 position = SendMessage(SCI_GETCURRENTPOS, UNSET, UNSET);

	int32 currentLine = SendMessage(SCI_LINEFROMPOSITION, position, UNSET);

	int end = SendMessage(SCI_GETLINEENDPOSITION, currentLine -1, UNSET);
	int start = SendMessage(SCI_POSITIONFROMLINE, currentLine -1, UNSET);
	// Empty line, return
	if (end == start)
		return;

	char previousLine[end - start + 1];
	SendMessage(SCI_GETLINE, currentLine -1, (sptr_t)previousLine);

	previousLine[end - start] =  '\0';

	BString lineStart;
	std::string previousString(previousLine);

	// Last line char is '{', indent
	if (previousString.back() == '{') {
		lineStart << lineIndent;
		tabInsertions += lineIndent.Length();
	}
	// TODO maybe
	else if (previousString.back() == ')') {
		lineStart << lineIndent;
		tabInsertions += lineIndent.Length();
	}
	// Last line char is ';', parse a little
	else if (previousString.back() == ';') {
		// If last word is: break, return, continue
		// deindent if possible (tab check only)
		std::size_t found = previousString.find_last_of(lineIndent);
		if (previousString.substr(found + 1) == "break;"
			|| previousString.substr(found + 1) == "return;"
			|| previousString.substr(found + 1) == "continue;")
			tabInsertions  -= lineIndent.Length();
		// TODO
		// Non void return

		// If previous line last char was ')'
		// deindent if possible
	}

	for (int pos = 0; previousLine[pos] != '\0'; pos++) {
		if (previousLine[pos] == '\t') {
			lineStart << previousLine[pos];
			tabInsertions++;
		} else if (previousLine[pos] == ' ') {
			lineStart << previousLine[pos];
			charInsertions++;
		} else
			break;
	}
	auto insertions = tabInsertions + charInsertions;
	// TODO check negative tab/char insertions

	if (insertions < 1)
		return;

	SendMessage(SCI_INSERTTEXT, position, (sptr_t)lineStart.String());
	SendMessage(SCI_GOTOPOS, position + insertions, UNSET);
}

/*
 * TODO oneline 'if' indentation guide not highlighted
 */
void
Editor::_CheckForBraceMatching()
{
	char charBefore;
	char charAfter;
	int32 positionMatch;

	int32 caretPosition = SendMessage(SCI_GETCURRENTPOS, UNSET, UNSET);
	int32 positionBefore = SendMessage(SCI_POSITIONBEFORE, caretPosition, UNSET);

	charBefore = SendMessage(SCI_GETCHARAT, positionBefore, UNSET);
	charAfter = SendMessage(SCI_GETCHARAT, caretPosition, UNSET);

	// No brace, return
	if (!_IsBrace(charBefore) && !_IsBrace(charAfter)) {
		// If there's nothing to do don't even waste a cycle
		if (fBraceHighlighted == kNoBrace) {
			return;
		} else if (fBraceHighlighted == kBraceMatch) {
			SendMessage(SCI_BRACEHIGHLIGHT, INVALID_POSITION, INVALID_POSITION);
			SendMessage(SCI_SETHIGHLIGHTGUIDE, 0, UNSET);
		} else if (fBraceHighlighted == kBraceBad)
			SendMessage(SCI_BRACEBADLIGHT, INVALID_POSITION, UNSET);

		fBraceHighlighted = kNoBrace;

		return;
	}
	// Found a brace, see if it's matched or not
	// Found before
	if (_IsBrace(charBefore) == true) {
		positionMatch = SendMessage(SCI_BRACEMATCH, positionBefore, UNUSED);
		// No match found, highlight brace bad
		if (positionMatch == -1) {
			SendMessage(SCI_BRACEBADLIGHT, positionBefore, UNSET);
			fBraceHighlighted = kBraceBad;
		}
		// Match found, highlight braces and guides
		else if (positionMatch != -1) {
			int maxPosition = MAX(positionBefore, positionMatch);
			int column = SendMessage(SCI_GETCOLUMN, maxPosition, UNSET);
			SendMessage(SCI_SETHIGHLIGHTGUIDE, column, UNSET);
			SendMessage(SCI_BRACEHIGHLIGHT, positionBefore, positionMatch);
			fBraceHighlighted = kBraceMatch;
		}
		return;
	}
	// Found after
	else if (_IsBrace(charAfter) == true) {
		positionMatch = SendMessage(SCI_BRACEMATCH, caretPosition, UNUSED);
		// No match found, highlight brace bad
		if (positionMatch == -1) {
			SendMessage(SCI_BRACEBADLIGHT, caretPosition, UNSET);
			fBraceHighlighted = kBraceBad;
		}
		// Match found, highlight braces and guides
		else if (positionMatch != -1) {
			int maxPosition = MAX(caretPosition, positionMatch);
			int column = SendMessage(SCI_GETCOLUMN, maxPosition, UNSET);
			SendMessage(SCI_SETHIGHLIGHTGUIDE, column, UNSET);
			SendMessage(SCI_BRACEHIGHLIGHT, caretPosition, positionMatch);
			fBraceHighlighted = kBraceMatch;
		}
	}
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
Editor::DuplicateCurrentLine() {
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
	for (int32 i = startLineNumber; i<=endLineNumber; i++) {
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
	for (int32 i = startLineNumber; i<=endLineNumber; i++) {
		int32 position = SendMessage(SCI_POSITIONFROMLINE, i, UNSET);
		_CommentLine(position);
	}
	SendMessage(SCI_ENDUNDOACTION, 0, UNSET);
}

int32
Editor::_EndOfLine()
{
	return SendMessage(SCI_GETEOLMODE, UNSET, UNSET);
}

void
Editor::_EndOfLineAssign(char *buffer, int32 size)
{
#ifdef USE_LINEBREAKS_ATTRS
	BNode node(&fFileRef);
	if (node.ReadAttr("be:line_breaks", B_INT32_TYPE, 0, &eol, sizeof(eol)) > 0) {
		SendMessage(SCI_SETEOLMODE, eol, UNSET);
		return;
	}
#endif

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
Editor::_HighlightFile()
{
	if (fSyntaxAvailable == true) {
		// Rust colors taken from rustbook, second edition
		if (fFileType == "rust") {
			SendMessage(SCI_STYLESETFORE, SCE_RUST_DEFAULT, 0x000000);
			SendMessage(SCI_STYLESETFORE, SCE_RUST_COMMENTBLOCK, 0x6E5B5E);
			SendMessage(SCI_STYLESETFORE, SCE_RUST_COMMENTBLOCKDOC, 0x6E5B5E);
			SendMessage(SCI_STYLESETFORE, SCE_RUST_COMMENTLINE, 0x6E5B5E);
			SendMessage(SCI_STYLESETFORE, SCE_RUST_COMMENTLINEDOC, 0x6E5B5E);
			SendMessage(SCI_STYLESETFORE, SCE_RUST_NUMBER, 0x6684E1);
			SendMessage(SCI_STYLESETFORE, SCE_RUST_WORD, 0xB854D4);
			SendMessage(SCI_STYLESETBOLD, SCE_RUST_WORD, 1);
//			SendMessage(SCI_STYLESETFORE, SCE_RUST_WORD2, );
//			SendMessage(SCI_STYLESETFORE, SCE_RUST_WORD3, );
//			SendMessage(SCI_STYLESETFORE, SCE_RUST_WORD4, );
//			SendMessage(SCI_STYLESETFORE, SCE_RUST_WORD5, );
//			SendMessage(SCI_STYLESETFORE, SCE_RUST_WORD6, );
//			SendMessage(SCI_STYLESETFORE, SCE_RUST_WORD7, );
			SendMessage(SCI_STYLESETFORE, SCE_RUST_STRING, 0x60AC39);
			SendMessage(SCI_STYLESETFORE, SCE_RUST_STRINGR, 0x60AC39);
//			SendMessage(SCI_STYLESETFORE, SCE_RUST_CHARACTER, );
//			SendMessage(SCI_STYLESETFORE, SCE_RUST_OPERATOR, );
//			SendMessage(SCI_STYLESETFORE, SCE_RUST_IDENTIFIER, );
//			SendMessage(SCI_STYLESETFORE, SCE_RUST_LIFETIME, );
			SendMessage(SCI_STYLESETFORE, SCE_RUST_MACRO, 0x6684E1);
//			SendMessage(SCI_STYLESETFORE, SCE_RUST_LEXERROR, );
//			SendMessage(SCI_STYLESETFORE, SCE_RUST_BYTESTRING, 0x60AC39);
//			SendMessage(SCI_STYLESETFORE, SCE_RUST_BYTESTRINGR, 0x60AC39);
//			SendMessage(SCI_STYLESETFORE, SCE_RUST_BYTECHARACTER, );
		} else {
			SendMessage(SCI_STYLESETFORE, SCE_C_DEFAULT, 0x000000);
			SendMessage(SCI_STYLESETFORE, SCE_C_COMMENT, 0xAA0000);
		//	SendMessage(SCI_STYLESETFORE, SCE_C_COMMENTLINE, 0x007F00);
			SendMessage(SCI_STYLESETFORE, SCE_C_COMMENTLINE, 0x007F3F);
			SendMessage(SCI_STYLESETFORE, SCE_C_COMMENTDOC, 0x3F703F);
			SendMessage(SCI_STYLESETFORE, SCE_C_NUMBER, 0x3030C0);
			SendMessage(SCI_STYLESETFORE, SCE_C_WORD, 0x7F0000);
			SendMessage(SCI_STYLESETBOLD, SCE_C_WORD, 1);
			SendMessage(SCI_STYLESETFORE, SCE_C_STRING, 0x7F007F);
			SendMessage(SCI_STYLESETFORE, SCE_C_CHARACTER, 0x7F007F);
			SendMessage(SCI_STYLESETFORE, SCE_C_UUID, 0x804080);
			SendMessage(SCI_STYLESETFORE, SCE_C_PREPROCESSOR, 0x1E6496); //0x10C0D0 //0x37B0B0 0x007F7F);
		//	SendMessage(SCI_STYLESETFORE, SCE_C_OPERATOR, 0x007F00);
		//	SendMessage(SCI_STYLESETBOLD, SCE_C_OPERATOR, 1);
		//	SendMessage(SCI_STYLESETFORE, SCE_C_IDENTIFIER, 0x808080);
			SendMessage(SCI_STYLESETFORE, SCE_C_WORD2, 0x986633);
		//	SendMessage(SCI_STYLESETBOLD, SCE_C_WORD2, 1);SCE_C_PREPROCESSORCOMMENT
			SendMessage(SCI_STYLESETFORE, SCE_C_PREPROCESSORCOMMENT, 0x808080);
			SendMessage(SCI_STYLESETFORE, SCE_C_GLOBALCLASS, 0x808080);
		}
	}
}

bool
Editor::_IsBrace(char character)
{
	return character == '('
			|| character == ')'
			|| character == '{'
			|| character == '}'
			|| character == '['
			|| character == ']';
// TODO !c++ lang add
//			|| character == '<'
//			|| character == '>';
}

void
Editor::_RedrawNumberMargin(bool forced)
{
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
Editor::_SetFoldMargin()
{
	if (IsFoldingAvailable() == true) {
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
	}
}


void
Editor::SetProblems(const BMessage* diagnostics)
{
	BAutolock lock(fProblemsLock);
	fProblems = *diagnostics;
	fProblems.what = EDITOR_UPDATE_DIAGNOSTICS;
	fProblems.AddRef("ref", &fFileRef);
	Window()->PostMessage(&fProblems);
}
void
Editor::GetProblems(BMessage* diagnostics)
{
	BAutolock lock(fProblemsLock);
	*diagnostics = fProblems;
}
