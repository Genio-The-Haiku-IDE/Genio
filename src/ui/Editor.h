/*
 * Copyright 2017 A. Mosca <amoscaster@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef EDITOR_H
#define EDITOR_H

#include <Entry.h>
#include <File.h>
#include <Messenger.h>
#include <ScintillaView.h>
#include <String.h>
#include <string>
#include <Locker.h>
#include <MessageFilter.h>

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
	EDITOR_UPDATE_DIAGNOSTICS		= 'diag'
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

constexpr auto sci_BOOKMARK = 0;

// Colors
static constexpr auto kLineNumberBack = 0xD3D3D3;
static constexpr auto kWhiteSpaceFore = 0x3030C0;
static constexpr auto kWhiteSpaceBack = 0xB0B0B0;
static constexpr auto kSelectionBackColor = 0x80FFFF;
static constexpr auto kCaretLineBackColor = 0xF8EFE9;
static constexpr auto kEdgeColor = 0xE0E0E0;
static constexpr auto kMarkerForeColor = 0x80FFFF;
static constexpr auto kMarkerBackColor = 0x3030C0;

constexpr auto kNoBrace = 0;
constexpr auto kBraceMatch = 1;
constexpr auto kBraceBad = 2;

class Editor : public BScintillaView {

public:
								Editor(entry_ref* ref, const BMessenger& target);
								~Editor();
	virtual	void 				MessageReceived(BMessage* message);

			void				ApplySettings();
			void				BookmarkClearAll(int marker);
			bool				BookmarkGoToNext();
			bool				BookmarkGoToPrevious();
			void				BookmarkToggle(int position);
			void				TrimTrailingWhitespace();
			bool				CanClear();
			bool				CanCopy();
			bool				CanCut();
			bool				CanPaste();
			bool				CanRedo();
			bool				CanUndo();
			void				Clear();
			void				CommentSelectedLines();
			void				Copy();
			int32				CountLines();
			void				Cut();
			void				DuplicateCurrentLine();
			void				DeleteSelectedLines();
			BString	const		EndOfLineString();
			void				EndOfLineConvert(int32 eolMode);
			void				EnsureVisiblePolicy();
		const BString			FilePath() const;
			entry_ref *const	FileRef() { return &fFileRef; }
			int32				Find(const BString&  text, int flags, bool backwards = false);
			int					FindInTarget(const BString& search, int flags, int startPosition, int endPosition);
			int32				FindMarkAll(const BString& text, int flags);
			int					FindNext(const BString& search, int flags, bool wrap);
			int					FindPrevious(const BString& search, int flags, bool wrap);
			int32				GetCurrentPosition();
			void				GoToLine(int32 line);
			void				GoToLSPPosition(int32 line, int character);
			void				GrabFocus();
			bool				IsFoldingAvailable() { return fFoldingAvailable; }
			bool				IsModified() { return fModified; }
			bool				IsOverwrite();
			BString const		IsOverwriteString();
			bool				IsParsingAvailable() { return fParsingAvailable; }
			bool				IsReadOnly();
			bool				IsSearchSelected(const BString& search, int flags);
			bool				IsTextSelected();
			status_t			LoadFromFile();
			BString const		ModeString();
			BString				Name() const { return fFileName; }
			node_ref *const		NodeRef() { return &fNodeRef; }
			void				NotificationReceived(SCNotification* n);
			void				OverwriteToggle();
			void				Paste();
			void				Redo();
			status_t			Reload();
			int					ReplaceAndFindNext(const BString& selection,
									const BString& replacement, int flags, bool wrap);
			int32				ReplaceAll(const BString& selection,
									const BString& replacement, int flags);
			void 				ReplaceMessage(int position, const BString& selection,
									const BString& replacement);
			int					ReplaceOne(const BString& selection,
									const BString& replacement);
			ssize_t				SaveToFile();
			void				ScrollCaret();
			void				SelectAll();
	const 	BString				Selection();
			void				SendPositionChanges();
			status_t			SetFileRef(entry_ref* ref);
			void				SetReadOnly(bool readOnly = true);
			status_t			SetSavedCaretPosition();
			int					SetSearchFlags(bool matchCase, bool wholeWord,
									bool wordStart,	bool regExp, bool posix);
			void				SetTarget(const BMessenger& target);
			status_t			StartMonitoring();
			status_t			StopMonitoring();
			void				ToggleFolding();
			void				ShowLineEndings(bool show);
			void				ShowWhiteSpaces(bool show);
			bool				LineEndingsVisible();
			bool				WhiteSpacesVisible();


			void				SetProjectFolder(ProjectFolder*);
			ProjectFolder*		GetProjectFolder() { return fProjectFolder; }
			void				SetZoom(int32 zoom);
			void				Undo();
			void				Completion();
			void				Format();
			void				GoToDefinition();
			void				GoToDeclaration();
			void				GoToImplementation();
			void				SwitchSourceHeader();
			void				UncommentSelection();

			void 				ContextMenu(BPoint point);

			void				SetProblems(const BMessage* diagnostics);
			void				GetProblems(BMessage* diagnostics);

			filter_result		BeforeKeyDown(BMessage*);
			filter_result		OnArrowKey(int8 ch);

			std::string			FileType() { return fFileType; }

private:
			void				UpdateStatusBar();
			void				_ApplyExtensionSettings();
			void				_MaintainIndentation(char c);
			void				_SetLineIndentation(int line, int indent);
			void				_BraceHighlight();
			bool				_BraceMatch(int pos);
			void				_CommentLine(int32 position);
			int32				_EndOfLine();
			void				_EndOfLineAssign(char *buffer, int32 size);
			void				_HighlightBraces();
			void				_HighlightFile();
			void				_RedrawNumberMargin(bool forced = false);
			void				_SetFoldMargin();
			void				_UpdateSavePoint(bool modified);

			template<typename T>
			typename T::type	Get() { return T::Get(this); }
			template<typename T>
			void				Set(typename T::type value) { T::Set(this, value); }

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

			LSPEditorWrapper*		fLSPEditorWrapper;
			ProjectFolder*		fProjectFolder;
			editor::StatusView*			fStatusView;

			BMessage	fProblems;
			BLocker		fProblemsLock;

};

#endif // EDITOR_H
