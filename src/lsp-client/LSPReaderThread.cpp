/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "LSPReaderThread.h"
#include "LSPPipeClient.h"

LSPReaderThread::LSPReaderThread(LSPPipeClient& looper)
	:
	GenericThread("LSPReaderThread"), fTransport(looper)
{
}


status_t
LSPReaderThread::ExecuteUnit()
{
	if (fTransport.readStep() == false) {
		printf("LSPReaderThread::ExecuteUnit(void) false\n");
		Quit();
		fTransport.Close();  //TODO: this should be performed by the Client itself not from the thread!
		return B_ERROR;
	}
	return B_OK;
}
