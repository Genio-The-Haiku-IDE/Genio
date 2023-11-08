/*
 * Copyright 2017 A. Mosca 
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef Genio_NAMESPACE_H
#define Genio_NAMESPACE_H

#include <Catalog.h>
#include <String.h>

namespace GenioNames
{
	const BString kApplicationName(B_TRANSLATE_SYSTEM_NAME("Genio"));
	const BString kApplicationSignature("application/x-vnd.Genio");
	const BString kSettingsFileName("Genio.settings");
	const BString kSettingsFilesToReopen("files_to_reopen.settings");
	const BString kSettingsProjectsToReopen("workspace.settings");
	const BString kProjectSettingsFile(".genio");

	BString GetSignature();
}

#endif // Genio_NAMESPACE_H
