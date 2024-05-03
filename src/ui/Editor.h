/*
 * Copyright 2017 A. Mosca <amoscaster@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef EDITOR_H
#define EDITOR_H

#include <string>

#include <Locker.h>
#include <MessageFilter.h>
#include <Messenger.h>
#include "ScintillaView.h"
#include <MessageRunner.h>
#include <set>
#include <utility>

#include "LSPCapabilities.h"


class LSPEditorWrapper;
class ProjectFolder;

namespace editor {
	class StatusView;
}

enum {
	EDITOR_FIND_COUNT				= 'Efco',
	EDITOR_FIND_NEXT_MISS			= 'Efnm',
	EDITOR_FIND_PREV_MISS			= 'Efpm',
	EDITOR_FIND_SET_MARK			= 'Efsm',
	EDITOR_POSITION_CHANGED			= 'Epch',
	EDITOR_REPLACE_ONE				= 'Eron',
	EDITOR_REPLACE_ALL_COUNT		= 'Erac',
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
		STATUS_NOT_INITIALIZED = 0,
		STATUS_NO_SYMBOLS = 1,
		STATUS_HAS_SYMBOLS = 2
	};
								Editor(entry_ref* ref, const BMessenger& target);
								~Editor();
	virtual	void 				MessageReceived(BMessage* message);
			void				LoadEditorConfig();
			void				ApplySettings();
			void				ApplyEdit(std::string info);
			void				TrimTrailingWhitespace();
			bool				CanCopy();
			bool				CanCut();
			bool				CanPaste();
			bool				CanRedo();
			bool				CanUndo();
			void				Copy();
			void				Cut();
			int32				EndOfLine();
		const BString			FilePath() const;
			entry_ref *const	FileRef() { return &fFileRef; }



			void				GoToLine(int32 line);
			void				GoToLSPPosition(int32 line, int character);
			void				GrabFocus();
			bool				IsFoldingAvailable() { return fFoldingAvailable; }
			bool				IsModified() { return fModified; }

			bool				IsParsingAvailable() { return fParsingAvailable; }
			void				NotificationReceived(SCNotification* n);
			void				Paste();
			void				Redo();
			status_t			Reload();

			bool				IsReadOnly();
			bool				IsTextSelected();
			status_t			LoadFromFile();
			BString				Name() const { return fFileName; }
			node_ref *const		NodeRef() { return &fNodeRef; }
			bool				IsOverwrite();

			ssize_t				SaveToFile();
			status_t			SetFileRef(entry_ref* ref);
			void				SetReadOnly(bool readOnly = true);
			status_t			SetSavedCaretPosition();
			void				SetTarget(const BMessenger& target);
			status_t			StartMonitoring();
			status_t			StopMonitoring();

			void				SetProjectFolder(ProjectFolder*);
			ProjectFolder*		GetProjectFolder() const { return fProjectFolder; }
			void				Undo();

			void				SetProblems();

			void				SetDocumentSymbols(const BMessage* symbols);
			void				GetDocumentSymbols(BMessage* symbols) const;

			filter_result		BeforeKeyDown(BMessage*);

			std::string			FileType() const { return fFileType; }
			void				SetFileType(std::string fileType) { fFileType = fileType; }

			void				SetCommentLineToken(std::string commenter){ fCommenter = commenter; }
			void				SetCommentBlockTokens(std::string startBlock, std::string endBlock){ /*TODO! */}

			LSPEditorWrapper*	GetLSPEditorWrapper() { return fLSPEditorWrapper; }
			bool				HasLSPServer() const;
			bool				HasLSPCapability(const LSPCapability cap);


private:

			int					ReplaceAndFindNext(const BString& selection,
									const BString& replacement, int flags, bool wrap);
			int					ReplaceAndFindPrevious(const BString& selection,
									const BString& replacement, int flags, bool wrap);
			int32				ReplaceAll(const BString& selection,
									const BString& replacement, int flags);
			void 				ReplaceMessage(int position, const BString& selection,
									const BString& replacement);
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
	const 	BString				Selection();
	const 	BString				GetSymbol();
	const 	BRect				GetSymbolSurroundingRect();
			void				SendPositionChanges();
			BString const		ModeString();
			void				OverwriteToggle();
			BString const		IsOverwriteString();
			bool				IsSearchSelected(const BString& search, int flags);
			int32				GetCurrentPosition();
			void				CommentSelectedLines();
			int32				CountLines();

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

			template<typename T>
			typename T::type	Get() { return T::Get(this); }
			template<typename T>
			void				Set(typename T::type value) { T::Set(this, value); }

			void	EvaluateIdleTime();

private:

			entry_ref			fFileRef;
			bool				fModified;
			BString				fFileName;
			node_ref			fNodeRef;
			BMessenger			fTarget;

			bool				fBracingAvailable;
			std::string			fFileType;
			bool				fFoldingAvailable;
			bool				fSyntaxAvailable;
			bool				fParsingAvailable;
			std::string			fCommenter;
			int					fLinesLog10;

			int					fCurrentLine;
			int					fCurrentColumn;

			LSPEditorWrapper*	fLSPEditorWrapper;
			ProjectFolder*		fProjectFolder;
			editor::StatusView*	fStatusView;

			BMessage			fProblems;
			BMessage			fDocumentSymbols;
			symbols_status		fSymbolsStatus;
			std::set<std::pair<std::string, int32> > fCollapsedSymbols;

			// editorconfig
			bool				fHasEditorConfig;
			EditorConfig		fEditorConfig;

			BMessageRunner*		fIdleHandler;
};

#endif // EDITOR_H
