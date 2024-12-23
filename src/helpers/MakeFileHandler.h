/*
 * Copyright 2024, the Genio team
 * All rights reserved. Distributed under the terms of the MIT license.
 */
 
#pragma once

#include <String.h>

class MakeFileHandler {
public:
	MakeFileHandler();
	MakeFileHandler(const char* path);
	virtual ~MakeFileHandler();
	
	status_t SetTo(const char* path);
	
	void GetTargetName(BString &name) const;
	void SetTargetName(const BString& name);

	void GetTargetDirectory(BString& dir) const;
	void SetTargetDirectory(const BString& dir);

	void GetFullTargetName(BString& fullName) const;

private:
	BString fTargetName;
	BString fTargetDir;
};
