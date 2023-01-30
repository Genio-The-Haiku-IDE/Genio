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
#include <ObjectList.h>

#include "ConsoleIOThread.h"

class BButton;
class BCheckBox;
class WordTextView;


class ConsoleIOView : public BGroupView {
public:
								ConsoleIOView(const BString& name, const BMessenger& target);
								~ConsoleIOView();

	static	ConsoleIOView*	Create(const BString& name, const BMessenger& target);
									// throws

	virtual	void				MessageReceived(BMessage* message);
	virtual	void				AttachedToWindow();

			void				Clear();
			void				ConsoleOutputReceived(
									int32 fd, const BString& output);
			void				EnableStopButton(bool doIt);

private:
			struct OutputInfo;
			typedef BObjectList<OutputInfo> OutputInfoList;

private:
			void				_Init();
			void				_HandleConsoleOutput(OutputInfo* info);

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
			OutputInfoList*		fPendingOutput;
};



#endif	// CONSOLE_IO_VIEW_H_
