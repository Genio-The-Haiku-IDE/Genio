/*
 * Copyright 2017..2018 A. Mosca <amoscaster@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef NEW_PROJECT_WINDOW_H
#define NEW_PROJECT_WINDOW_H

#include <Box.h>
#include <CheckBox.h>
#include <FilePanel.h>
#include <ListView.h>
#include <map>
#include <String.h>
#include <TextControl.h>
#include <TextView.h>
#include <Window.h>

#include "ProjectParser.h"
#include "TitleItem.h"
#include "TPreferences.h"


typedef std::map<BStringItem*, BString const> ProjectTypeMap;
typedef std::pair<BStringItem*, BString const> ProjectTypePair;
typedef std::map<BStringItem*, BString const>::const_iterator ProjectTypeIterator;

class NewProjectWindow : public BWindow
{
public:
								NewProjectWindow();
								~NewProjectWindow();

	virtual	bool				QuitRequested();
			void 				MessageReceived(BMessage *msg);

private:
			status_t			_CreateProject();
			status_t			_CreateAppProject();
			status_t			_CreateAppMenuProject();
			status_t			_CreateCargoProject();
			status_t			_CreateHelloCplusProject();
			status_t			_CreateHelloCProject();
			status_t			_CreatePrinciplesProject();
			status_t			_CreateEmptyProject();
			status_t			_CreateHaikuSourcesProject();
			status_t			_CreateLocalSourcesProject();
			status_t			_CreateSkeleton();
			void				_InitWindow();
			void				_MapItems();
			void				_OnEditingHaikuAppText();
			void				_OnEditingLocalAppText();
			void				_RemoveStaleEntries(const char* dirpath);
			bool				_FindMakefile(BString& target);
			bool				_ParseMakefile(BString& target, const BEntry* entry);
			void				_UpdateControlsState(int32 selection);
			void				_UpdateControlsData(int32 selection);
			status_t			_WriteMakefile();
			status_t			_WriteAppfiles();
			status_t			_WriteAppMenufiles();
			status_t			_WriteHelloCplusfile();
			status_t			_WriteHelloCfile();
			status_t			_WritePrinciplesfile();
			status_t			_WriteHelloCMakefile();

private:
			BOutlineListView*	fTypeListView;
			BScrollView*		typeListScrollView;

			TPreferences*		fProjectFile;
			BString				fProjectExtensionedName;

			BString				f_primary_architecture;
			BString				f_cargo_binary;
			BString				f_project_directory;

			TitleItem*			haikuItem;
			BStringItem*		appItem;
			BStringItem* 		appMenuItem;
/*
			BStringItem* 		appLayoutItem;
			BStringItem*		sharedItem;
			BStringItem*		staticItem;
			BStringItem*		driverItem;
			BStringItem*		trackerItem;
*/
			TitleItem*			genericItem;
			BStringItem*		helloCplusItem;
			BStringItem*		helloCItem;
			BStringItem*		principlesItem;
			BStringItem*		emptyItem;
			TitleItem*			importItem;
			BStringItem*		sourcesItem;
			BStringItem*		existingItem;

			TitleItem*			rustItem;
			BStringItem*		cargoItem;

			BTextView*			fProjectDescription;
			BScrollView*		fScrollText;

			BBox*				fProjectBox;
			BTextControl*		fProjectNameText;
			BCheckBox*			fGitEnabled;
			BTextControl*		fProjectTargetText;
			BCheckBox*			fRunInTeminal;
			BTextControl*		fProjectsDirectoryText;
			BTextControl*		fAddFileText;
			BCheckBox*			fAddHeader;
			BTextControl*		fAddSecondFileText;
			BCheckBox*			fAddSecondHeader;
			BTextControl*		fHaikuAppDirText;
			BButton*			fBrowseHaikuAppButton;
			BFilePanel*			fOpenPanel;
			BTextControl*		fLocalAppDirText;
			BButton*			fBrowseLocalAppButton;
			BTextControl*		fCargoPathText;
			BCheckBox*			fCargoBinEnabled;
			BCheckBox*			fCargoVcsEnabled;

			BButton*			fCreateButton;
			BButton*			fExitButton;

			BStringItem*		fCurrentItem;

};

#endif // NEW_PROJECT_WINDOW_H
