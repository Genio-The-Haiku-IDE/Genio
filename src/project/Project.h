/*
 * Copyright 2017..2018 A. Mosca <amoscaster@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef PROJECT_H
#define PROJECT_H

#include <String.h>
#include <vector>

#include "ProjectTitleItem.h"
#include "TPreferences.h"

class LSPClientWrapper;

class Project {
public:
								Project(BString const& name);
								~Project();

			void				Activate();
			BString				BasePath() const { return fProjectDirectory; }
			BString	const		BuildCommand();
			BString	const		CleanCommand();
			void				Deactivate();
			BString	const		ExtensionedName() const { return fExtensionedName; }
	std::vector<BString> const	FilesList();
			bool				IsActive() { return isActive; }
			BString	const		Name() const { return fName; }
			status_t			Open(bool activate);
			bool 				ReleaseModeEnabled();
			bool				RunInTerminal() { return fRunInTerminal; }
			BString	const		Scm();
			void				SetReleaseMode(bool releaseMode);
	std::vector<BString> const	SourcesList();
			BString	const	 	Target();
			ProjectTitleItem*	Title() const { return fProjectTitle; }
			BString				Type() const { return fType; }
			
			LSPClientWrapper*	GetLSPClient() { return fLSPClientWrapper; }

private:

private:
			BString				fName;
			BString const		fExtensionedName;
			BString				fProjectDirectory;
			BString				fType;
			bool				fRunInTerminal;
			bool				isActive;
			ProjectTitleItem*	fProjectTitle;
		std::vector<BString>	fFilesList;
		std::vector<BString>	fSourcesList;
			LSPClientWrapper*	fLSPClientWrapper;

};


#endif // PROJECT_H

