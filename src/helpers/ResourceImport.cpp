/*
 * Copyright 2024, The Genio team
 * All rights reserved. Distributed under the terms of the MIT license.
 * Author: Nexus6 <nexus6.haiku@icloud.com>
 */

#include "ResourceImport.h"

#include <cstdio>
#include <cstring>

#include <DataIO.h>
#include <Entry.h>
#include <Node.h>
#include <NodeInfo.h>
#include <String.h>

#include "Log.h"

ResourceImport::ResourceImport(entry_ref &ref, int32 index)
{
	status_t status = fFile.SetTo(&ref, B_READ_ONLY);
	if (status == B_OK) {
		off_t size;
		if (fFile.GetSize(&size) != B_OK)
			return;
		uint8 fileBuffer[size];
		BMallocIO stringBuffer;
		stringBuffer.SetSize(size);
		fFile.Read(fileBuffer, size);

		BString type;
		BNode node(&ref);
		BNodeInfo nodeInfo;
		nodeInfo.SetTo(&node);
		if (nodeInfo.InitCheck() == B_OK) {
			char mimeType[B_MIME_TYPE_LENGTH];
			if (nodeInfo.GetType(mimeType) == B_OK) {
				BString mimeTypeString(mimeType);
				if (mimeTypeString == "image/x-hvif")
					type = "#'VICN'";
			}
		}

		if (_Import((const uint8*)fileBuffer, size,	&stringBuffer, ref.name, type, index) != B_OK)
			LogError("Opening file for import failed with error %s", strerror(status));
		else
			fRdefArray.SetTo((const char*)stringBuffer.Buffer());
	} else {
		LogError("Opening file for import failed with error %s", strerror(status));
	}
}


ResourceImport::~ResourceImport()
{
}


status_t
ResourceImport::_Import(const uint8* source, size_t sourceSize, BPositionIO* stream,
	BString name, BString type, int32 index) const
{
	// write header
	BString indexString;
	if (index != -1)
		indexString << index;
	else
		indexString << "<your resource id here>";

	char buffer[2048];
	snprintf(buffer, sizeof(buffer), "\nresource(%s, \"%s\") %s array {\n",
		indexString.String(),
		name.String(), type.String());
	size_t size = strlen(buffer);

	ssize_t written = stream->Write(buffer, size);
	if (written < 0)
		return (status_t)written;
	if (written < (ssize_t)size)
		return B_ERROR;

	status_t ret = B_OK;
	const uint8* b = source;

	// print one line (32 values)
	while (sourceSize >= 32) {
		snprintf(buffer, sizeof(buffer),
					"	$\"%.2X%.2X%.2X%.2X"
						"%.2X%.2X%.2X%.2X"
						"%.2X%.2X%.2X%.2X"
						"%.2X%.2X%.2X%.2X"
						"%.2X%.2X%.2X%.2X"
						"%.2X%.2X%.2X%.2X"
						"%.2X%.2X%.2X%.2X"
						"%.2X%.2X%.2X%.2X\"\n",
						b[0], b[1], b[2], b[3],
						b[4], b[5], b[6], b[7],
						b[8], b[9], b[10], b[11],
						b[12], b[13], b[14], b[15],
						b[16], b[17], b[18], b[19],
						b[20], b[21], b[22], b[23],
						b[24], b[25], b[26], b[27],
						b[28], b[29], b[30], b[31]);

		size = strlen(buffer);
		written = stream->Write(buffer, size);
		if (written != (ssize_t)size) {
			if (written >= 0)
				ret = B_ERROR;
			else
				ret = (status_t)written;
			break;
		}

		sourceSize -= 32;
		b += 32;
	}
	// beginning of last line
	if (ret >= B_OK && sourceSize > 0) {
		snprintf(buffer, sizeof(buffer), "	$\"");
		size = strlen(buffer);
		written = stream->Write(buffer, size);
		if (written != (ssize_t)size) {
			if (written >= 0)
				ret = B_ERROR;
			else
				ret = (status_t)written;
		}
	}
	// last line (up to 32 values)
	bool endQuotes = sourceSize > 0;
	if (ret >= B_OK && sourceSize > 0) {
		for (size_t i = 0; i < sourceSize; i++) {
			snprintf(buffer, sizeof(buffer), "%.2X", b[i]);
			size = strlen(buffer);
			written = stream->Write(buffer, size);
			if (written != (ssize_t)size) {
				if (written >= 0)
					ret = B_ERROR;
				else
					ret = (status_t)written;
				break;
			}
		}
	}
	if (ret >= B_OK) {
		// finish
		snprintf(buffer, sizeof(buffer), endQuotes ? "\"\n};\n" : "};\n");
		size = strlen(buffer);
		written = stream->Write(buffer, size);
		if (written != (ssize_t)size) {
			if (written >= 0)
				ret = B_ERROR;
			else
				ret = (status_t)written;
		}
	}

	return ret;
}
