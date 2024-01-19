/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once

#include <string>
#include <SupportDefs.h>
#include <MessageFilter.h>
#include <Messenger.h>
#include <View.h>

#include "LSPCapabilities.h"
#include "EditorMessages.h"

class ProjectFolder;
class LSPEditorWrapper;


class BWindow;
class BLooper;

class Editor {

public:
								Editor() {};
			virtual				~Editor() {};

			// Every editor must be based on a view and have a target
			// to send information to.

			virtual BView*	View() = 0;

			virtual void	ApplySettings() = 0;

			virtual bool	CanCopy() = 0;
			virtual bool	CanCut() = 0;
			virtual bool	CanPaste() = 0;
			virtual bool	CanRedo() = 0;
			virtual bool	CanUndo() = 0;
			virtual void	Copy() = 0;
			virtual void	Cut() = 0;
			virtual void	Paste() = 0;
			virtual void	Redo() = 0;
			virtual void	Undo() = 0;

			virtual	const 	BString		Selection() = 0;


			//These are specific to CodeEditor
			virtual void				GoToLine(int32 line) = 0;
			virtual void				GoToLSPPosition(int32 line, int character) = 0;

			//
			virtual void				GrabFocus() = 0;

			//Editor status
			virtual int32				EndOfLine() = 0;
			virtual bool				IsModified() = 0;
			virtual bool				IsOverwrite() = 0;
			virtual bool				IsReadOnly() = 0;

			virtual BString				Name()  const = 0;

			//Editor 'reference'
			virtual node_ref *const		NodeRef()  = 0;
			virtual status_t			SetFileRef(entry_ref* ref) = 0;
			virtual const BString		FilePath() const = 0;
			virtual entry_ref *const	FileRef()  = 0;

			//I/O
			virtual status_t			LoadFromFile() = 0;
			virtual status_t			Reload() = 0;
			virtual ssize_t				SaveToFile() = 0;

			virtual void				SetReadOnly(bool readOnly = true) = 0;
			virtual status_t			SetSavedCaretPosition() = 0; //TODO: improve the name
																	 // why can't be used after laoding
																	 // the file?

			virtual status_t			StartMonitoring() = 0;
			virtual status_t			StopMonitoring() = 0;

			virtual void				SetProjectFolder(ProjectFolder*) = 0;
			virtual ProjectFolder*		GetProjectFolder() const  = 0;
			virtual void				SetProblems(const BMessage* diagnostics) = 0;
			virtual void				GetProblems(BMessage* diagnostics) = 0;

			virtual filter_result		BeforeKeyDown(BMessage*) = 0;

			virtual std::string			FileType() const  = 0;
			virtual void				SetFileType(std::string fileType)  = 0;

			// Capabilities
			virtual bool				IsFoldingAvailable() = 0;
			virtual bool				HasLSPCapability(const LSPCapability cap) = 0;
};
