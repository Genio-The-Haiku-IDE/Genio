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

#include <Messenger.h>

#include <errno.h>
#include <image.h>
#include <iostream>
#include <signal.h>
#include <termios.h>
#include <unistd.h>

#include "GenioNamespace.h"

 //lock access play with stdin/stdout (from LSPCLient.h)
extern BLocker *g_LockStdFilesPntr;

extern char **environ;

ConsoleIOThread::ConsoleIOThread(BMessage* cmd_message,
					const BMessenger& windowTarget, const BMessenger& consoleTarget)
	:
	GenericThread("ConsoleIOThread", B_NORMAL_PRIORITY, cmd_message)
	, fWindowTarget(windowTarget)
	, fConsoleTarget(consoleTarget)
	, fThreadId(-1)
	, fStdIn(-1)
	, fStdOut(-1)
	, fStdErr(-1)
	, fConsoleOutput(nullptr)
	, fConsoleError(nullptr)
{
	SetDataStore(new BMessage(*cmd_message));
}

ConsoleIOThread::~ConsoleIOThread()
{
}

status_t
ConsoleIOThread::ThreadStartup()
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

	fThreadId = PipeCommand(argc, argv, fStdIn, fStdOut, fStdErr);

	delete [] argv;

	if (fThreadId < 0)
		return fThreadId;

	// lower the command priority since it is a background task.
	set_thread_priority(fThreadId, B_LOW_PRIORITY);

	resume_thread(fThreadId);

	int flags = fcntl(fStdOut, F_GETFL, 0);
	flags |= O_NONBLOCK;
	fcntl(fStdOut, F_SETFL, flags);
	flags = fcntl(fStdErr, F_GETFL, 0);
	flags |= O_NONBLOCK;
	fcntl(fStdErr, F_SETFL, flags);

	fConsoleOutput = fdopen(fStdOut, "r");
	fConsoleError = fdopen(fStdErr, "r");

	// Enable Stop button in view
	BMessage button_message(CONSOLEIOTHREAD_ENABLE_STOP_BUTTON);
	button_message.AddBool("enable", true);
	fConsoleTarget.SendMessage(&button_message);

	// Let console view know the cmd_type so Stop action will post it
	button_message.AddString("cmd_type", fCmdType);

	if (GenioNames::Settings.console_banner == true)
		_BannerMessage("started   ");

	return B_OK;
}

status_t
ConsoleIOThread::ExecuteUnit(void)
{
	// read output and error from command
	// send it to window
	BMessage out_message(CONSOLEIOTHREAD_STDOUT);
	BString output_string("");

	output_string = fgets(fConsoleOutputBuffer , LINE_MAX,
		fConsoleOutput);

	if (output_string != "") {
		out_message.AddString("stdout", output_string);
		
		fConsoleTarget.SendMessage(&out_message);
	}

	BMessage err_message(CONSOLEIOTHREAD_STDERR);
	BString error_string("");

	error_string = fgets(fConsoleOutputBuffer, LINE_MAX,
		fConsoleError);

	if (error_string != "") {
		err_message.AddString("stderr", error_string);
		fConsoleTarget.SendMessage(&err_message);

	}

	if (feof(fConsoleOutput) && feof(fConsoleError))
		return EOF;

	// streams are non blocking, sleep every 1ms
	snooze(1000);

	return B_OK;
}

void
ConsoleIOThread::PushInput(BString text)
{
	write(fStdIn, text.String(), text.Length());
}

status_t
ConsoleIOThread::ThreadShutdown(void)
{
	close(fStdIn);
	close(fStdOut);
	close(fStdErr);

	// Disable Stop button in view
	BMessage button_message(CONSOLEIOTHREAD_ENABLE_STOP_BUTTON);
	button_message.AddBool("enable", false);
	fConsoleTarget.SendMessage(&button_message);

	if (GenioNames::Settings.console_banner == true)
		_BannerMessage("ended   --");

	return B_OK;
}

void
ConsoleIOThread::ThreadStartupFailed(status_t status)
{
	BMessage message(CONSOLEIOTHREAD_STOP);
	message.AddString("cmd_type", "startfail");
	fWindowTarget.SendMessage(&message);

	BString banner;
	banner << __PRETTY_FUNCTION__ << ": " << strerror(status) << "   ";
	_BannerMessage(banner);

	Quit();
}

void
ConsoleIOThread::ExecuteUnitFailed(status_t status)
{
	if (status == EOF) {
		// thread has finished, been quit or killed, we don't know
		BMessage message(CONSOLEIOTHREAD_EXIT);
		message.AddString("cmd_type", fCmdType);
		fWindowTarget.SendMessage(&message);
	} else {
		// explicit error - communicate error to Window
		BMessage message(CONSOLEIOTHREAD_ERROR);
		message.AddString("cmd_type", fCmdType);
		fWindowTarget.SendMessage(&message);
	}

	Quit();
}

/*
 * ThreadShutdown only returns B_OK so this will never be called.
 */
void
ConsoleIOThread::ThreadShutdownFailed(status_t status)
{
	BString banner;
	banner << __PRETTY_FUNCTION__ << ": " << strerror(status) << "   ";
	_BannerMessage(banner);
}


thread_id
ConsoleIOThread::PipeCommand(int argc, const char** argv, int& in, int& out,
	int& err, const char** envp)
{
	// This function written by Peter Folk 
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
	status_t status = get_thread_info(fThreadId, &info);

	if (status == B_OK)
		return send_signal(-fThreadId, SIGSTOP);
	else
		return status;
}

status_t
ConsoleIOThread::ResumeExternal()
{
	thread_info info;
	status_t status = get_thread_info(fThreadId, &info);

	if (status == B_OK)
		return send_signal(-fThreadId, SIGCONT);
	else
		return status;
}

status_t
ConsoleIOThread::InterruptExternal()
{
	thread_info info;
	status_t status = get_thread_info(fThreadId, &info);

	if (status == B_OK) {
		status = send_signal(-fThreadId, SIGINT);
		WaitOnExternal();
	}

	return status;
}

status_t
ConsoleIOThread::WaitOnExternal()
{
	thread_info info;
	status_t status = get_thread_info(fThreadId, &info);

	if (status == B_OK)
		return wait_for_thread(fThreadId, &status);
	else
		return status;
}

void
ConsoleIOThread::_BannerMessage(BString status)
{
	BString banner;
	banner  << "--------------------------------"
			<< "   "
			<< fCmdType
			<< " "
			<< status
			<< "--------------------------------\n";

	BMessage banner_message(CONSOLEIOTHREAD_PRINT_BANNER);
	banner_message.AddString("banner", banner);
	fConsoleTarget.SendMessage(&banner_message);
}

