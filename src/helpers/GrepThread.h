/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef GrepThread_H
#define GrepThread_H


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
	virtual	void	OnStdOutputLine(const BString& stdOut);
	virtual void	OnStdErrorLine(const BString& stdErr) {};
	virtual void	OnThreadShutdown();
private:
	char fCurrentFileName[B_PATH_NAME_LENGTH];
	char fLine[MAX_LINE_LEN];
	char fNextFileName[B_PATH_NAME_LENGTH];
	BMessage fCurrentMessage;
	entry_ref fCurrentRef;
};


#endif // GrepThread_H
