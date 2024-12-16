/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once

#include <GroupView.h>
#include <Messenger.h>
#include <ObjectList.h>

class BButton;
class BCheckBox;
class BTextView;
class MTerm;
class KeyTextViewScintilla;

#define kTermViewDone	'tvdo'

class MTermView : public BGroupView {
public:
								MTermView(const BString& name, const BMessenger& target);
								~MTermView();

	virtual	void				MessageReceived(BMessage* message);
	virtual	void				AttachedToWindow();

			void				Clear();


			KeyTextViewScintilla*		TextView();

			status_t			RunCommand(BMessage* cmd_message);

			void	ApplyStyle();


private:
			void				EnableStopButton(bool doIt);
			void				_Init();
			void				_HandleOutput(const BString& info);
			void				_BannerMessage(BString status);
			void				_EnsureStopped();

			BMessenger					fWindowTarget;
			KeyTextViewScintilla*		fKeyTextView;
			BButton*					fClearButton;
			BButton*					fStopButton;
			BString						fBannerClaim;
			MTerm* 						fMTerm;
};
