/*
 * Copyright 2018 A. Mosca <amoscaster@gmail.com>
 *
 * Adapted from Debugger::ConsoleOutputView class
 * Copyright 2013-2014, Rene Gollent, rene@gollent.com
 *
 * Distributed under the terms of the MIT License
 */
#ifndef CONSOLE_IO_VIEW_H_
#define CONSOLE_IO_VIEW_H_

#include <GroupView.h>
#include <Messenger.h>
#include <ObjectList.h>

class BButton;
class BCheckBox;
class BTextView;
class ConsoleIOThread;
class WordTextView;
class ConsoleIOView : public BGroupView {
public:
								ConsoleIOView(const BString& name, const BMessenger& target);
								~ConsoleIOView();

	virtual	void				MessageReceived(BMessage* message);
	virtual	void				AttachedToWindow();

			void				Clear();
			void				ConsoleOutputReceived(
									int32 fd, const BString& output);
			void				EnableStopButton(bool doIt);

			void				SetWordWrap(bool wrap);

			BTextView*			TextView();

			status_t			RunCommand(BMessage* cmd_message);

private:
			struct OutputInfo;
			typedef BObjectList<OutputInfo> OutputInfoList;

private:
			void				_Init();
			void				_HandleConsoleOutput(OutputInfo* info);
			void				_BannerMessage(BString status);
			void				Pulse();
			void				_StopThreads();
			void				_StopCommand(status_t status);

private:
			BMessenger			fWindowTarget;

			BCheckBox*			fStdoutEnabled;
			BCheckBox*			fStderrEnabled;
			BCheckBox*			fWrapEnabled;
			BCheckBox*			fBannerEnabled;
			WordTextView*		fConsoleIOText;
			BButton*			fClearButton;
			BButton*			fStopButton;
			BString				fCmdType;
			BString				fBannerClaim;
			OutputInfoList*		fPendingOutput;
			ConsoleIOThread*	fConsoleIOThread;
};



#endif	// CONSOLE_IO_VIEW_H_
