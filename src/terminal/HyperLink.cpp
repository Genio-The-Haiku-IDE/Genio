/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "HyperLink.h"

#include <Application.h>
#include <Entry.h>
#include <cstdio>
#include <errno.h>
#include <stdlib.h>

#include "TermConst.h"


HyperLink::HyperLink()
	:
	fAddress(),
	fType(TYPE_URL)
{
}


HyperLink::HyperLink(const BString& address, Type type)
	:
	fText(address),
	fAddress(address),
	fType(type)
{
}


HyperLink::HyperLink(const BString& text, const BString& address, Type type)
	:
	fText(text),
	fAddress(address),
	fType(type)
{
}

status_t
HyperLink::Open()
{
	if (!IsValid())
		return B_BAD_VALUE;

	BMessage msg(B_REFS_RECEIVED);
	if (GetType() != TYPE_URL) {
		// Remove line and column indication
		if (GetType() == TYPE_PATH_WITH_LINE ||
			GetType() == TYPE_PATH_WITH_LINE_AND_COLUMN) {
			int32 pos = fAddress.FindFirst(":");
			if (pos > 0 && pos < fAddress.Length()) {
				BString lineText = fAddress;
				lineText.Remove(0, pos + 1);
				fAddress.Remove(pos, fAddress.Length() - pos);
				char* extraText = nullptr;
				int32 line = ::strtol(lineText.String(), &extraText, 10);
				if (line > 0)
					msg.AddInt32("start:line", line);
			}
		}
		entry_ref ref;
		if (get_ref_for_path(fAddress.String(), &ref) == B_OK &&
			BEntry(&ref).IsDirectory() != true) {
					
			msg.AddRef("refs", &ref);
			msg.AddBool("openWithPreferred", true);

			be_app->PostMessage(&msg);
			return B_OK;
		}
	}

	// open with the "open" program
	BString address(fAddress);
	address.CharacterEscape(kShellEscapeCharacters, '\\');
	BString commandLine;
	commandLine.SetToFormat("/bin/open %s", address.String());
	return system(commandLine) == 0 ? B_OK : errno;
}
