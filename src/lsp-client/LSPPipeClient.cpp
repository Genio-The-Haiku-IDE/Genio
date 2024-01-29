/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 *
 * Source code derived from AGMSScriptOCron
 * 	Copyright (c) 2018 by Alexander G. M. Smith.
 *
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "LSPPipeClient.h"
#include "Log.h"
#include "LSPReaderThread.h"
#include <Messenger.h>

status_t
LSPPipeClient::Start(const char **argv, int32 argc)
{
	status_t image_status = fPipeImage.Init(argv, argc, false, true);
	if (image_status == B_OK)
		LSPPipeClient::Run();
	return image_status;
}

void
LSPPipeClient::Close()
{
	fPipeImage.Close();
}

LSPPipeClient::~LSPPipeClient()
{
	Close();
	ForceQuit();
}

bool
LSPPipeClient::ReadHeaderLine(char* header, size_t maxlen)
{
	int hasRead = 0;
	size_t length = 0;
	while ((hasRead = fPipeImage.Read(&header[length], 1)) != -1) {
		if (hasRead == 0 || length >= maxlen-1) // pipe eof or protection
			return false;

		if (header[length] == '\n') {
			break;
		}
		length++;
	}
	return true;
}
int
LSPPipeClient::ReadMessageHeader()
{
	char szReadBuffer[255];
	int len = 0;
	while(ReadHeaderLine(szReadBuffer, 255)) {
		if (strncmp(szReadBuffer, "Content-Length: ", 16) == 0) {
			len = atoi(szReadBuffer + 16);
		} else if (strncmp(szReadBuffer, "\r\n", 2) == 0) {
			break;
		} else {
			LogTrace("Unsuported LSP message header: %s", szReadBuffer);
		}
	}
	return len;
}

int
LSPPipeClient::Read(int length, std::string &out)
{
	int readSize = 0;
	int hasRead;
	out.resize(length);
	while ((hasRead = fPipeImage.Read(&out[readSize], length)) != -1) {
		if (hasRead == 0) // pipe eof
			return 0;

		readSize += hasRead;
		if (readSize >= length) {
			break;
		}
	}

	return readSize;
}


bool
LSPPipeClient::Write(std::string &in)
{
	int hasWritten = 0;
	int writeSize = 0;
	int totalSize = in.length();

	if (fWriteLock.Lock()) // for production code: WithTimeout(1000000) == B_OK)
	{
		while ((hasWritten = fPipeImage.Write(&in[writeSize], totalSize)) != -1) {
			writeSize += hasWritten;
			if (writeSize >= totalSize || hasWritten == 0) {
				break;
			}
		}
		fWriteLock.Unlock();
	}
	return (hasWritten != 0);
}


bool
LSPPipeClient::readMessage(std::string &json)
{
	json.clear();
	int length = ReadMessageHeader();
	if (length == 0)
		return false;
	//SkipLine();
	//std::string read;
	if (Read(length, json) == 0)
		return false;
	LogTrace("Client - rcv %d:\n%s\n", length, json.c_str());
	return true;
}
bool
LSPPipeClient::writeMessage(std::string &content)
{
	std::string header = "Content-Length: " + std::to_string(content.length()) +
                       "\r\n\r\n" + content;
	LogTrace("Client: - snd \n%s\n", content.c_str());
	return Write(header);
}


LSPPipeClient::LSPPipeClient(uint32 what, BMessenger& msgr)
			   : AsyncJsonTransport(what, msgr)
			   , fReaderThread(nullptr)
{

}

pid_t
LSPPipeClient::GetChildPid()
{
	return fPipeImage.GetChildPid();
}
void
LSPPipeClient::ForceQuit()
{
	if (fReaderThread)
		fReaderThread->Suspend();

	Close();
	PostMessage(B_QUIT_REQUESTED);
}

void
LSPPipeClient::KillThread()
{
	if (fReaderThread)
		fReaderThread->Kill();
}

bool
LSPPipeClient::HasQuitBeenRequested()
{
	return fReaderThread && fReaderThread->HasQuitBeenRequested();
}

thread_id
LSPPipeClient::Run()
{
	fReaderThread = new LSPReaderThread(*this);
	fReaderThread->Start();
	return BLooper::Run();
}

void
LSPPipeClient::Quit()
{
	KillThread();
	return BLooper::Quit();
}
