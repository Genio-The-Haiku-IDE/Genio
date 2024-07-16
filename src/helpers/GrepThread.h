/*
 * Copyright 2023, Andrea Anzani 
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once

#include "ConsoleIOThread.h"

#define MAX_LINE_LEN B_PATH_NAME_LENGTH * 2

enum {
	MSG_REPORT_RESULT = 'mrre',
	MSG_GREP_DONE = 'mgrd'
};

class GrepThread : public ConsoleIOThread {
public:
	GrepThread(BMessage* cmd_message, const BMessenger& consoleTarget);

protected:
			void	OnStdOutputLine(const BString& stdOut) override;
			void	OnStdErrorLine(const BString& stdErr) override {};
			void	ThreadExitNotification() override;
private:
	char fCurrentFileName[B_PATH_NAME_LENGTH];
	char fLine[MAX_LINE_LEN];
	char fNextFileName[B_PATH_NAME_LENGTH];
	BMessage fCurrentMessage;
	entry_ref fCurrentRef;
};
