/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once

#include "GenericThread.h"

class LSPPipeClient;
class LSPReaderThread : public GenericThread {
public:
		LSPReaderThread(LSPPipeClient& looper);

		status_t ExecuteUnit() override;
private:
	LSPPipeClient&	fTransport;
};
