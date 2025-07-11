/*
 * Copyright 2025 Andrea Anzani <andrea.anzani@gmail.com>
 * Copyright 2017 A. Mosca <amoscaster@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once

#include <Locker.h>
#include <MessageFilter.h>
#include <Messenger.h>
#include <MessageRunner.h>

#include <set>
#include <string>
#include <utility>

#include "EditorId.h"
#include "LSPCapabilities.h"
#include "ScintillaView.h"

class LSPEditorWrapper;
class ProjectFolder;

namespace editor {
	class StatusView;
}

enum {
	EDITOR_POSITION_CHANGED			= 'Epch',
	EDITOR_UPDATE_SAVEPOINT			= 'EUSP',
	EDITOR_UPDATE_DIAGNOSTICS		= 'diag',
	EDITOR_UPDATE_SYMBOLS			= 'symb'
};

enum IndentStyle {
	Tab,
	Space
};

/*
 * Not very smart: NONE,SKIP,DONE are Status
 * while the others are Function placeholders
 *
 */
enum {
	REPLACE_NONE = -1,
	REPLACE_SKIP = 0,
	REPLACE_DONE = 1,
	REPLACE_ONE,
	REPLACE_NEXT,
	REPLACE_PREVIOUS,
	REPLACE_ALL
};

constexpr auto sci_NUMBER_MARGIN = 0;
constexpr auto sci_BOOKMARK_MARGIN = 1;
constexpr auto sci_FOLD_MARGIN = 2;
constexpr auto sci_COMMENT_MARGIN = 3;

constexpr auto sci_BOOKMARK = 0; //Marker

struct EditorConfig {
	enum IndentStyle	IndentStyle;
	int32				IndentSize;
	int32				EndOfLine;
	bool				TrimTrailingWhitespace;
	bool				InsertFinalNewline; // Not implemented
};



class Editor : public BScintillaView {
public:
	enum symbols_status {
		STATUS_UNKNOWN			= 0, // "<empty string>"
		STATUS_NO_CAPABILITY	= 1, // "No outline available"
		STATUS_REQUESTED		= 2, // "Creating outline"
		STATUS_HAS_SYMBOLS		= 3, // <list of symbols (if any)>
	};

								Editor(entry_ref* ref, const BMessenger& target);
								~Editor();
			editor_id			Id() { return fId; }
			BString				Name() const { return fFileName; }
			void				SetProjectFolder(ProjectFolder*);
			ProjectFolder*		GetProjectFolder() const { return fProjectFolder; }
			filter_result		BeforeKeyDown(BMessage*);
			filter_result		BeforeMouseMoved(BMessage* message);
			filter_result		BeforeModifiersChanged(BMessage* message);
			void				GrabFocus();


			// Cut, Copy and Paste interface
			bool				CanCopy();
			void				Copy();
			bool				CanCut();
			void				Cut();
			bool				CanPaste();
			void				Paste();

			// Undo and Redo interface
			bool				CanRedo();
			void				Redo();
			bool				CanUndo();
			void				Undo();

			// File interface
		const BString			FilePath() const;
			entry_ref *const	FileRef() { return &fFileRef; }
			status_t			SetFileRef(entry_ref* ref);
			node_ref *const		NodeRef() { return &fNodeRef; }
			status_t			LoadFromFile();
			status_t			SaveToFile();
			status_t			Reload();
			status_t			StartMonitoring();
			status_t			StopMonitoring();
			std::string			FileType() const { return fFileType; }
			void				SetFileType(const std::string& fileType) { fFileType = fileType; }
			status_t			SetSavedCaretPosition();

			//
			void				LoadEditorConfig();
			void				ApplySettings();
			void				ApplyEdit(const std::string& info);
			void				TrimTrailingWhitespace();

			void				GoToLine(int32 line);
			void				GoToLSPPosition(int32 line, int character);

			bool				IsFoldingAvailable() const { return fFoldingAvailable; }
			bool				IsModified() const { return fModified; }
			bool				IsTextSelected();
			bool				IsOverwrite();
			bool				IsReadOnly();
			void				SetReadOnly(bool readOnly = true);
			int32				EndOfLine();



			void				SetProblems();

			void				SetDocumentSymbols(const BMessage* symbols, Editor::symbols_status status);
			void				GetDocumentSymbols(BMessage* symbols) const;

			void				SetCommentLineToken(const std::string& commenter) { fCommenter = commenter; }
			void				SetCommentBlockTokens(const std::string& startBlock,
												const std::string& endBlock);

			LSPEditorWrapper*	GetLSPEditorWrapper() { return fLSPEditorWrapper; }
			bool				HasLSPServer() const;
			bool				HasLSPCapability(const LSPCapability cap) const;

			// Scripting methods
			const 	BString		Selection();
			void				SetSelection(int32 start, int32 end);
			const 	BString		GetSymbol();
			void				Insert(BString text, int32 start = -1);
			void				Append(BString text);
			const 	BString		GetLine(int32 lineNumber);
			void				InsertLine(BString text, int32 lineNumber);
			int32				CountLines();
			int32				GetCurrentLineNumber();

protected:
			virtual	void 		MessageReceived(BMessage* message);
			void				SetTarget(const BMessenger& target);
			void				NotificationReceived(SCNotification* n);

private:

			int					ReplaceAndFindNext(const BString& selection,
									const BString& replacement, int flags, bool wrap);
			int					ReplaceAndFindPrevious(const BString& selection,
									const BString& replacement, int flags, bool wrap);
			int32				ReplaceAll(const BString& selection,
									const BString& replacement, int flags);
			int					ReplaceOne(const BString& selection,
									const BString& replacement);
			int					SetSearchFlags(bool matchCase, bool wholeWord,
									bool wordStart,	bool regExp, bool posix);
			int32				FindMarkAll(const BString& text, int flags);
			int					FindNext(const BString& search, int flags, bool wrap);
			int					FindPrevious(const BString& search, int flags, bool wrap);
			int					FindInTarget(const BString& search, int flags, int startPosition, int endPosition);
			int32				Find(const BString&  text, int flags, bool backwards, bool onWrap);
			filter_result		OnArrowKey(int8 ch);
			void				SetZoom(int32 zoom);
			void				Completion();
			void				Format();
			void				GoToDefinition();
			void				GoToDeclaration();
			void				GoToImplementation();
			void				Rename();
			void				SwitchSourceHeader();
			void				UncommentSelection();

			void 				ContextMenu(BPoint point);
			void				ToggleFolding();
			void				ShowLineEndings(bool show);
			void				ShowWhiteSpaces(bool show);
			bool				LineEndingsVisible();
			bool				WhiteSpacesVisible();

			void				ScrollCaret();
			void				SelectAll();
	const 	BRect				GetSymbolSurroundingRect();
			void				SendPositionChanges();
			BString const		ModeString();
			void				OverwriteToggle();
			BString const		IsOverwriteString();
			bool				IsSearchSelected(const BString& search, int flags);
			void				CommentSelectedLines();

			void				DuplicateCurrentLine();
			void				DeleteSelectedLines();
			void				EndOfLineConvert(int32 eolMode);
			void				EnsureVisiblePolicy();
			bool				CanClear();
			void				Clear();
			void				BookmarkClearAll(int marker);
			bool				BookmarkGoToNext();
			bool				BookmarkGoToPrevious();
			void				BookmarkToggle(int position);
			int32				GetCurrentPosition();
			BString	const		_EndOfLineString();
			void				UpdateStatusBar();
			void				_ApplyExtensionSettings();
			void 				_LoadResources(BMessage *message);
			void				_MaintainIndentation(char c);
			void				_SetLineIndentation(int line, int indent);
			void				_BraceHighlight();
			bool				_BraceMatch(int pos);
			void				_CommentLine(int32 position);
			void				_EndOfLineAssign(char *buffer, int32 size);
			void				_HighlightBraces();
			void				_RedrawNumberMargin(bool forced = false);
			void				_SetFoldMargin(bool enabled);
			void				_UpdateSavePoint(bool modified);
			void				_NotifyFindStatus(const char* status);

			template<typename T>
			typename T::type	Get() { return T::Get(this); }
			template<typename T>
			void				Set(typename T::type value) { T::Set(this, value); }

			void	EvaluateIdleTime();

private:
			editor_id			fId;
			entry_ref			fFileRef;
			bool				fModified;
			BString				fFileName;
			node_ref			fNodeRef;
			BMessenger			fTarget;

			bool				fBracingAvailable;
			std::string			fFileType;
			bool				fFoldingAvailable;
			std::string			fCommenter;
			int					fLinesLog10;

			int					fCurrentLine;
			int					fCurrentColumn;

			LSPEditorWrapper*	fLSPEditorWrapper;
			ProjectFolder*		fProjectFolder;
			editor::StatusView*	fStatusView;

			BMessage			fDocumentSymbols;
			std::set<std::pair<std::string, int32> > fCollapsedSymbols;

			// editorconfig
			bool				fHasEditorConfig;
			EditorConfig		fEditorConfig;

			BMessageRunner*		fIdleHandler;

			Sci_Position		fLastWordStartPosition = -1;
			Sci_Position		fLastWordEndPosition = -1;
};
