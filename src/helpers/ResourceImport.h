/*
 * Copyright 2024, The Genio team
 * All rights reserved. Distributed under the terms of the MIT license.
 * Author: Nexus6 <nexus6.haiku@icloud.com>
 */
#pragma once

#include <optional>

#include <DataIO.h>
#include <File.h>
#include <FilePanel.h>
#include <String.h>
#include <SupportDefs.h>

class ResourceImport {
public:
				ResourceImport(entry_ref &ref, int32 index = -1);
				~ResourceImport();

				BString GetArray() const { return fRdefArray; }
private:
	status_t 	_Import(BDataIO* const source, size_t sourceSize, BPositionIO* stream,
					BString name, BString type, int32 index) const;
	BString		fRdefArray;
	BFile		fFile;
};


