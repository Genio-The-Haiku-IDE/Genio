/*
 * Copyright 2017..2018 A. Mosca <amoscaster@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef PROJECT_SETTINGS_WINDOW_H
#define PROJECT_SETTINGS_WINDOW_H

#include <memory>
#include <vector>

#include <Box.h>
#include <Button.h>
#include <CheckBox.h>
#include <MenuField.h>
#include <ScrollView.h>
#include <StringView.h>
#include <TextControl.h>
#include <Window.h>

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
			void 				_LoadDefaults();

			ProjectFolder		*fProject;
			BBox* 				fProjectBox;
			BString		 		fProjectBoxLabel;
			BString		 		fProjectBoxProjectLabel;

			BBox* 				fBuildCommandsBox;
			BTextControl* 		fReleaseProjectTargetText;
			BTextControl* 		fDebugProjectTargetText;
			BTextControl* 		fReleaseBuildCommandText;
			BTextControl* 		fDebugBuildCommandText;
			BTextControl* 		fReleaseCleanCommandText;
			BTextControl* 		fDebugCleanCommandText;
			BTextControl* 		fReleaseExecuteArgsText;
			BTextControl* 		fDebugExecuteArgsText;
			BString				fTargetString;
			BString				fBuildString;
			BString				fCleanString;
			BBox* 				fTargetBox;
			BCheckBox*			fRunInTerminal;

			BString				fRunArgsString;

};


#endif // PROJECT_SETTINGS_WINDOW_H
