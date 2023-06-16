/*
 * Copyright 2018 A. Mosca <amoscaster@gmail.com>
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

#include <Messenger.h>

#include <errno.h>
#include <image.h>
#include <iostream>
#include <signal.h>
#include <termios.h>
#include <unistd.h>

#include "GenioNamespace.h"
#include "Log.h"

 //lock access play with stdin/stdout (from LSPCLient.h)
extern BLocker *g_LockStdFilesPntr;

extern char **environ;



ConsoleIOThread::ConsoleIOThread(BMessage* cmd_message, const BMessenger& consoleTarget)
	:
	GenericThread("ConsoleIOThread", B_NORMAL_PRIORITY, cmd_message)
	, fTarget(consoleTarget)
	, fProcessId(-1)
	, fStdIn(-1)
	, fStdOut(-1)
	, fStdErr(-1)
	, fConsoleOutput(nullptr)
	, fConsoleError(nullptr)
	, fIsDone(false)
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
	status_t status = B_OK;
	BString cmd;

	if ((status = GetDataStore()->FindString("cmd", &cmd)) != B_OK)
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

	fProcessId = PipeCommand(argc, argv, fStdIn, fStdOut, fStdErr);

	delete [] argv;

	if (fProcessId < 0)
		return B_ERROR;

	// lower the command priority since it is a background task.
	set_thread_priority(fProcessId, B_LOW_PRIORITY);

	resume_thread(fProcessId);

	int flags = fcntl(fStdOut, F_GETFL, 0);
	flags |= O_NONBLOCK;
	fcntl(fStdOut, F_SETFL, flags);
	flags = fcntl(fStdErr, F_GETFL, 0);
	flags |= O_NONBLOCK;
	fcntl(fStdErr, F_SETFL, flags);

	_CleanPipes();

	fConsoleOutput = fdopen(fStdOut, "r");
	if (fConsoleOutput == nullptr) {
		LogErrorF("Can't open ConsoleOutput file! (%d) [%s]", errno, strerror(errno));
		return B_ERROR;
	}
	fConsoleError = fdopen(fStdErr, "r");
	if (fConsoleError == nullptr) {
		LogErrorF("Can't open ConsoleError file! (%d) [%s]", errno, strerror(errno));
		return B_ERROR;
	}

	return B_OK;
}

status_t
ConsoleIOThread::ExecuteUnit(void)
{
	// first time: let's setup the external process.
	// this way we always enter in the same managed loop
	status_t status = B_OK;

	if (fProcessId == -1) {
		status = _RunExternalProcess();
	} else {

		// read output and error from command
		// send it to window
		BString outStr("");
		BString errStr("");
		status = GetFromPipe(outStr, errStr);
		if (status == B_OK) {
			if (outStr != "")
				OnStdOutputLine(outStr);

			if (errStr != "")
				OnStdErrorLine(errStr);

			if (IsProcessAlive() != B_OK && outStr.IsEmpty() && errStr.IsEmpty()) {
				printf("IsProcessAlive %d\n", IsProcessAlive());
				return EOF;
			}
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
	return (feof(fConsoleOutput) && feof(fConsoleError)) ? EOF : B_OK;
}

void
ConsoleIOThread::PushInput(BString text)
{
	write(fStdIn, text.String(), text.Length());
}

void
ConsoleIOThread::OnThreadShutdown()
{
	BMessage message(CONSOLEIOTHREAD_EXIT);
	message.AddString("cmd_type", fCmdType);
	fTarget.SendMessage(&message);
}

status_t
ConsoleIOThread::ThreadShutdown(void)
{
	ClosePipes();
	OnThreadShutdown();
	fIsDone = true;

	// the job is done, let's wait to be killed..
	// (avoid to quit and to reach the 'delete this')

	while(true) {
		sleep(1);
	}

	return B_OK;
}

void
ConsoleIOThread::ExecuteUnitFailed(status_t status)
{
	Quit();
}


thread_id
ConsoleIOThread::PipeCommand(int argc, const char** argv, int& in, int& out,
	int& err, const char** envp)
{
	// This function written by Peter Folk <pfolk@uni.uiuc.edu>
	// and published in the BeDevTalk FAQ
	// http://www.abisoft.com/faq/BeDevTalk_FAQ.html#FAQ-209
	if (!envp)
		envp = (const char**)environ;

	// Save current FDs
	g_LockStdFilesPntr->Lock();
	int old_in  =  dup(0);
	int old_out  =  dup(1);
	int old_err  =  dup(2);
	g_LockStdFilesPntr->Unlock();

	int filedes[2];

	// Create new pipe FDs as stdin, stdout, stderr
	pipe(filedes);  dup2(filedes[0], 0); close(filedes[0]);
	in = filedes[1];  // Write to in, appears on cmd's stdin
	pipe(filedes);  dup2(filedes[1], 1); close(filedes[1]);
	out = filedes[0]; // Read from out, taken from cmd's stdout
	pipe(filedes);  dup2(filedes[1], 2); close(filedes[1]);
	err = filedes[0]; // Read from err, taken from cmd's stderr

	// "load" command.
	thread_id ret  =  load_image(argc, argv, envp);
	if (ret < B_OK)
		goto cleanup;

	// thread ret is now suspended.

	setpgid(ret, ret);

cleanup:
	// Restore old FDs
	g_LockStdFilesPntr->Lock();
	close(0); dup(old_in); close(old_in);
	close(1); dup(old_out); close(old_out);
	close(2); dup(old_err); close(old_err);
	g_LockStdFilesPntr->Unlock();
	/* Theoretically I should do loads of error checking, but
	   the calls aren't very likely to fail, and that would
	   muddy up the example quite a bit.  YMMV. */

	return ret;
}

status_t
ConsoleIOThread::SuspendExternal()
{
	thread_info info;
	status_t status = get_thread_info(fProcessId, &info);
	if (status == B_OK)
		return send_signal(-fProcessId, SIGSTOP);
	else
		return status;
}

status_t
ConsoleIOThread::ResumeExternal()
{
	thread_info info;
	status_t status = get_thread_info(fProcessId, &info);
	if (status == B_OK)
		return send_signal(-fProcessId, SIGCONT);
	else
		return status;
}

void
ConsoleIOThread::ClosePipes()
{
	if (fConsoleOutput)
		fclose(fConsoleOutput);
	if (fConsoleError)
		fclose(fConsoleError);

	fConsoleOutput = fConsoleError = nullptr;

	close(fStdIn);
	close(fStdOut);
	close(fStdErr);

	fStdIn = fStdOut = fStdErr = -1;
}

status_t
ConsoleIOThread::IsProcessAlive()
{
	thread_info info;
	return get_thread_info(fProcessId, &info);
}

status_t
ConsoleIOThread::InterruptExternal()
{
	status_t status = IsProcessAlive();
	if (status == B_OK) {
		status = send_signal(-fProcessId, SIGTERM);
		return wait_for_thread_etc(fProcessId, 0, 4000, &status);
	}

	return status;
}


void
ConsoleIOThread::_CleanPipes()
{
	int32 maxSteps = PIPE_BUF / LINE_MAX;
	//let's clean the current duplicated pipes.
	for (int i=0;i<maxSteps;i++) {
		read(fStdOut, fConsoleOutputBuffer, LINE_MAX);
		read(fStdErr, fConsoleOutputBuffer, LINE_MAX);
	}
}
