/*
 * Copyright 2024, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 *
 * Code freely taken from haiku Terminal:
 * Copyright 2001-2023, Haiku, Inc. All rights reserved.
 * Copyright 2003-2004 Kian Duffy, myob@users.sourceforge.net
 * Parts Copyright 1998-1999 Kazuho Okui and Takashi Murai.
 * All rights reserved. Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Stefano Ceccherini, stefano.ceccherini@gmail.com
 *		Kian Duffy, myob@users.sourceforge.net
 *		Y.Hayakawa, hida@sawada.riec.tohoku.ac.jp
 *		Jonathan Schleifer, js@webkeks.org
 *		Simon South, simon@simonsouth.net
 *		Ingo Weinhold, ingo_weinhold@gmx.de
 *		Clemens Zeidler, haiku@Clemens-Zeidler.de
 *		Siarzhuk Zharski, zharik@gmx.li
 */

#pragma once


#include <SupportDefs.h>
#include <OS.h>
#include <Messenger.h>
#include "Task.h"

#define READ_BUF_SIZE 2048

enum MTermMessages {
	kMTOutputText	=	'ouot',
	kMTReadDone		= 	'rdon'
};


using namespace Genio::Task;

class MTerm{
public:
			MTerm(const BMessenger& msgr);
			~MTerm();
			void Run(int argc, const char* const* argv);
			void Kill(); //exp

			void	Write(const void* buffer, size_t size);
private:
			status_t _Spawn(int argc, const char* const* argv);
			status_t _ReadThread();

	static int32	_RunReaderThread(void* data);

	pid_t				fExecProcessID;
	int					fFd;
	BMessenger			fMessenger;
	Task<status_t>* 	fReadTask;
};


