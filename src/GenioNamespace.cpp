/*
 * Copyright 2017 A. Mosca 
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include "GenioNamespace.h"

#include <Application.h>
#include <AppFileInfo.h>
#include <Roster.h>
#include <StringList.h>

namespace GenioNames
{

BString
GetSignature()
{
	static BString appSignature;
	if (appSignature.IsEmpty()) {
		char signature[B_MIME_TYPE_LENGTH];
		app_info appInfo;
		if (be_app->GetAppInfo(&appInfo) == B_OK) {
			BFile file(&appInfo.ref, B_READ_ONLY);
			if (file.InitCheck() == B_OK) {
				BAppFileInfo appFileInfo(&file);
				if (appFileInfo.InitCheck() == B_OK && appFileInfo.GetSignature(signature) == B_OK) {
						appSignature.SetTo(signature);
				}
			}
		}
	}

	return appSignature;
}

} // namespace GenioNames
