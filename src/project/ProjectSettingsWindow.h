/*
 * Copyright 2017..2018 A. Mosca <amoscaster@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef PROJECT_SETTINGS_WINDOW_H
#define PROJECT_SETTINGS_WINDOW_H

#include <Box.h>
#include <Button.h>
#include <CheckBox.h>
#include <MenuField.h>
#include <ScrollView.h>
#include <StringView.h>
#include <TextControl.h>
#include <Window.h>
#include <vector>

#include "ProjectFolder.h"


class ProjectSettingsWindow : public BWindow
{
public:
								ProjectSettingsWindow(ProjectFolder *project);
	virtual						~ProjectSettingsWindow();

	virtual void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();
private:
			void				_CloseProject();
			void				_InitWindow();
			void				_LoadProject();
			void				_SaveChanges();

			ProjectFolder		*fProject;
			// int32				fProjectsCount;
			BBox* 				fProjectBox;
			BString		 		fProjectBoxLabel;
			BString		 		fProjectBoxProjectLabel;

			BBox* 				fBuildCommandsBox;
			BMenuField*			fProjectMenuField;
			BTextControl* 		fProjectTargetText;
			BTextControl* 		fBuildCommandText;
			BTextControl* 		fCleanCommandText;
			BTextControl* 		fProjectScmText;
			BTextControl* 		fProjectTypeText;
			BString				fTargetString;
			BString				fBuildString;
			BString				fCleanString;
			BString				fProjectScmString;
			BString				fProjectTypeString;
			BBox* 				fRuntimeBox;
			BTextControl* 		fRunArgsText;
			BString				fRunArgsString;
			// BBox* 				fProjectParselessBox;
			// BStringView*		fProjectParselessBoxLabel;
			// BTextView*			fParselessText;
			// BScrollView*		fParselessScroll;
		// std::vector<BString>	fParselessList;
			// TPreferences*		fIdmproFile;
};


#endif // PROJECT_SETTINGS_WINDOW_H
