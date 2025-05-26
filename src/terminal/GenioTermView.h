/*
 * Copyright 2025, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once

#include <Archivable.h>
#include <SupportDefs.h>

enum {
	TERMVIEW_CLEAR = 'clea'
};

class GenioTermView  {
public:
	static	BArchivable*		Instantiate(BMessage* data);
};


