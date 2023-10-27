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

int32
CompareVersion(const BString appVersion, const BString fileVersion)
{
	BStringList appVersionList, fileVersionList;

	if (appVersion == fileVersion)
		return 0;

	appVersion.Split(".", true, appVersionList);
	fileVersion.Split(".", true, fileVersionList);

	// Shoud not happen, but return just in case
	if (appVersionList.CountStrings() != fileVersionList.CountStrings())
		// TODO notify
		return 0;

	for (int32 index = 0; index < appVersionList.CountStrings(); index++) {
		if (appVersionList.StringAt(index) == fileVersionList.StringAt(index))
			continue;
		if (appVersionList.StringAt(index) < fileVersionList.StringAt(index))
			return -1;
		else if (appVersionList.StringAt(index) > fileVersionList.StringAt(index))
			return 1;
	}
	// Should not get here
	return 0;
}

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

/*
 * retcode: IsEmpty(), "0.0.0.0", version
 */
BString
GetVersionInfo()
{
	BString version("");
	version_info info = {0};
	app_info appInfo;

	if (be_app->GetAppInfo(&appInfo) == B_OK) {
		BFile file(&appInfo.ref, B_READ_ONLY);
		if (file.InitCheck() == B_OK) {
			BAppFileInfo appFileInfo(&file);
			if (appFileInfo.InitCheck() == B_OK) {
				appFileInfo.GetVersionInfo(&info, B_APP_VERSION_KIND);
				version << info.major << "." << info.middle
					<< "." << info.minor << "." << info.internal;
			}
		}
	}

	return version;
}


} // namespace GenioNames
