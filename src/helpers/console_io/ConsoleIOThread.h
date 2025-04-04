/*
 * Copyright 2018 A. Mosca <amoscaster@gmail.com>
 *
 * Adapted from Expander::ExpanderThread class
 * Copyright 2004-2010, Jérôme Duval. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 * Original code from ZipOMatic by jonas.sundstrom@kirilla.com
 */

/*
 * ConsoleIOThread is the worker class for console I/O (build log, terminal
 * program input/output, etc.).
 * It gets the command (via message) from main window, executes it in pipes
 * and sends the streams to the visual class, ConsoleIOView (via messages).
 * Some logic is also sent, like enabling and disabling Stop button, and start,
 * end, error banners.
 * When the thread is over, or in case of error, a message is sent to the main
 * window.
 * The only exception is when the user presses the stop button. In that case it
 * is the visual class itself that sends a message to main window.
 * All end messages sent to main window contain the command type in order to
 * apply some logic (reenable build/run buttons and menus).
 * stdin is handled via DispatchMessage in main window.
 */
#pragma once

#include "GenericThread.h"

#include <Locker.h>
#include <Message.h>
#include <Messenger.h>
#include <String.h>

#include <cstdio>

#include "PipeImage.h"

enum {
	CONSOLEIOTHREAD_EXIT				= 'Cexi',
	CONSOLEIOTHREAD_STDOUT				= 'Csou',
	CONSOLEIOTHREAD_STDERR				= 'Cser'
};

class ConsoleIOThread : public GenericThread {
public:
								ConsoleIOThread(BMessage* cmd_message,
									const BMessenger& consoleTarget);

	virtual						~ConsoleIOThread();

			status_t			InterruptExternal();

			bool				IsDone() const { return fIsDone; };

protected:
	virtual	void	OnStdOutputLine(const BString& stdOut);
	virtual void	OnStdErrorLine(const BString& stdErr);
	virtual void	ThreadExitNotification();
	BMessenger		fTarget;

private:
			void				PushInput(BString text);
			bool				IsProcessAlive() const;
			status_t			GetFromPipe(BString& stdOut, BString& stdErr);
			void				ClosePipes();
			status_t			ThreadStartup() override;
			status_t			ExecuteUnit() override;
			status_t			ThreadShutdown() override;

			void				_CleanPipes();
			status_t			_RunExternalProcess();

	virtual status_t			Kill(void);

			thread_id			fExternalProcessId;
			FILE*				fConsoleOutput;
			FILE*				fConsoleError;
			char				fConsoleOutputBuffer[LINE_MAX];
			BString 			fCmdType;
			bool				fIsDone;
			bool				fFailed;
			BString				fLastOutputString;
			BString				fLastErrorString;
			BLocker				fProcessIDLock;
			PipeImage			fPipeImage;
};
