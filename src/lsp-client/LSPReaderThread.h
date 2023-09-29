/*
 * Copyright 2023, Andrea Anzani 
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef LSPReaderThread_H
#define LSPReaderThread_H


#include <SupportDefs.h>
#include "GenericThread.h"

class LSPPipeClient;

class LSPReaderThread : public GenericThread {
public:
		LSPReaderThread(LSPPipeClient& looper);
		
		status_t ExecuteUnit(void);

private:

	LSPPipeClient&	fTransport;

};


#endif // _H
