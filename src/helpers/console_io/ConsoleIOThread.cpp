/*
 * Copyright 2018 A. Mosca 
 *
 * Adapted from Expander::ExpanderThread class
 * Copyright 2004-2010, Jérôme Duval. All rights reserved.
 * Original code from ZipOMatic by jonas.sundstrom@kirilla.com
 *
 * PipeCommand function taken from Haiku add-ons/print/drivers/postscript/FilterIO.cpp
 *
 * Distributed under the terms of the MIT License.
 */
#include "ConsoleIOThread.h"

#include <Autolock.h>
#include <Debug.h>
#include <Messenger.h>

#include <errno.h>
#include <image.h>
#include <signal.h>
#include <termios.h>
#include <unistd.h>

#include "Log.h"
#include "PipeImage.h"

ConsoleIOThread::ConsoleIOThread(BMessage* cmd_message, const BMessenger& consoleTarget)
	:
	GenericThread("ConsoleIOThread", B_NORMAL_PRIORITY, cmd_message),
	fTarget(consoleTarget),
	fExternalProcessId(-1),
	fConsoleOutput(nullptr),
	fConsoleError(nullptr),
	fIsDone(false)
{
	SetDataStore(new BMessage(*cmd_message));
}


ConsoleIOThread::~ConsoleIOThread()
{
	ClosePipes();
}


status_t
ConsoleIOThread::_RunExternalProcess()
{
	ASSERT(fExternalProcessId < 0);
	BAutolock lock(fProcessIDLock);
	BString cmd;
	status_t status = GetDataStore()->FindString("cmd", &cmd);
	if (status != B_OK)
		return status;

	BString type("none");
	GetDataStore()->FindString("cmd_type", &type);
	fCmdType = type;

	int32 argc = 3;
	const char** argv = new const char * [argc + 1];

	argv[0] = strdup("/bin/sh");
	argv[1] = strdup("-c");
	argv[2] = strdup(cmd.String());
	argv[argc] = nullptr;

	status = fPipeImage.Init(argv, argc, true, false);

	delete[] argv;

	if (status != B_OK)
		return status;

	fExternalProcessId = fPipeImage.GetChildPid();

	// lower the command priority since it is a background task.
	set_thread_priority(fExternalProcessId, B_LOW_PRIORITY);

	status = resume_thread(fExternalProcessId);
	if (status != B_OK) {
		kill_thread(fExternalProcessId);
		fExternalProcessId = -1;
		return status;
	}

	int flags = fcntl(fPipeImage.GetStdOutFD(), F_GETFL, 0);
	flags |= O_NONBLOCK;
	fcntl(fPipeImage.GetStdOutFD(), F_SETFL, flags);
	flags = fcntl(fPipeImage.GetStdErrFD(), F_GETFL, 0);
	flags |= O_NONBLOCK;
	fcntl(fPipeImage.GetStdErrFD(), F_SETFL, flags);

	// avoid having some buffered stuff in the output
	// of the new process. It looks like when forking we are
	// inheriting output buffers of the main process.
	_CleanPipes();

	fConsoleOutput = fdopen(fPipeImage.GetStdOutFD(), "r");
	if (fConsoleOutput == nullptr) {
		LogErrorF("Can't open ConsoleOutput file! (%d) [%s]", errno, strerror(errno));
		kill_thread(fExternalProcessId);
		fExternalProcessId = -1;
		return errno;
	}
	fConsoleError = fdopen(fPipeImage.GetStdErrFD(), "r");
	if (fConsoleError == nullptr) {
		LogErrorF("Can't open ConsoleError file! (%d) [%s]", errno, strerror(errno));
		kill_thread(fExternalProcessId);
		fExternalProcessId = -1;
		return errno;
	}

	return B_OK;
}


/* virtual */
status_t
ConsoleIOThread::ThreadStartup(void)
{
	return _RunExternalProcess();
}


status_t
ConsoleIOThread::ExecuteUnit()
{
	if (fExternalProcessId < 0)
		return B_NO_INIT;

	// read output and error from command
	// send it to window
	BString outStr("");
	BString errStr("");
	status_t status = GetFromPipe(outStr, errStr);
	if (status == B_OK) {
		fLastOutputString += outStr;
		fLastErrorString  += errStr;

		if (!fLastOutputString.IsEmpty()) {
			if (fLastOutputString.EndsWith("\n")) {
				OnStdOutputLine(fLastOutputString);
				fLastOutputString = "";
			}
		}

		if (!fLastErrorString.IsEmpty()) {
			if (fLastErrorString.EndsWith("\n")) {
				OnStdErrorLine(fLastErrorString);
				fLastErrorString = "";
			}
		}

		if (!IsProcessAlive() && outStr.IsEmpty() && errStr.IsEmpty()) {
			printf("DONE\n");
			return EOF;
		}
	}

	// streams are non blocking, sleep every 1ms
	snooze(1000);
	return status;
}


void
ConsoleIOThread::OnStdOutputLine(const BString& stdOut)
{
	BMessage out_message(CONSOLEIOTHREAD_STDOUT);
	out_message.AddString("stdout", stdOut);
	fTarget.SendMessage(&out_message);
}


void
ConsoleIOThread::OnStdErrorLine(const BString& stdErr)
{
	BMessage err_message(CONSOLEIOTHREAD_STDERR);
	err_message.AddString("stderr", stdErr);
	fTarget.SendMessage(&err_message);
}


status_t
ConsoleIOThread::GetFromPipe(BString& stdOut, BString& stdErr)
{
	if (fConsoleOutput) {
		stdOut = fgets(fConsoleOutputBuffer, LINE_MAX, fConsoleOutput);
	}
	if (fConsoleError) {
		stdErr = fgets(fConsoleOutputBuffer, LINE_MAX, fConsoleError);
	}
	return (::feof(fConsoleOutput) && ::feof(fConsoleError)) ? EOF : B_OK;
}


void
ConsoleIOThread::PushInput(BString text)
{
	fPipeImage.Write(text.String(), text.Length());
}


void
ConsoleIOThread::ThreadExitNotification()
{
	BMessage message(CONSOLEIOTHREAD_EXIT);
	message.AddString("cmd_type", fCmdType);
	fTarget.SendMessage(&message);
}


status_t
ConsoleIOThread::ThreadShutdown()
{
	ClosePipes();
	fIsDone = true;
	ThreadExitNotification();

	// the job is done, let's wait to be killed..
	// (avoid to quit and to reach the 'delete this')
	while (true) {
		snooze(1000);
	}

	return B_OK;
}


void
ConsoleIOThread::ClosePipes()
{
	if (fConsoleOutput)
		::fclose(fConsoleOutput);
	if (fConsoleError)
		::fclose(fConsoleError);

	fConsoleOutput = fConsoleError = nullptr;

	fPipeImage.Close();
}


bool
ConsoleIOThread::IsProcessAlive() const
{
	thread_info info;
	return get_thread_info(fExternalProcessId, &info) == B_OK;
}



status_t
ConsoleIOThread::Kill(void)
{
	return GenericThread::Kill();
}


status_t
ConsoleIOThread::InterruptExternal()
{
	BAutolock lock(fProcessIDLock);
	if (IsProcessAlive()) {
		status_t status = send_signal(-fExternalProcessId, SIGTERM);
		status = wait_for_thread_etc(fExternalProcessId, B_RELATIVE_TIMEOUT, 2000000, nullptr); //2 seconds
		if (status != B_OK) {
			// It looks like we are not able to wait for the thead to close.
			// let's cut the pipes and kill it.
			ClosePipes();
			status = Kill();
		}
		fExternalProcessId = -1;
		return status;
	}

	return B_ERROR;
}


void
ConsoleIOThread::_CleanPipes()
{
	// pipes are set to non-blocking so we should never be stuck here.
	while (read(fPipeImage.GetStdOutFD(), fConsoleOutputBuffer, LINE_MAX) > 0) {
		// loop
	}
	while (read(fPipeImage.GetStdErrFD(), fConsoleOutputBuffer, LINE_MAX) > 0) {
		// loop
	}
}
