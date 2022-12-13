/*
 * Copyright 2018 A. Mosca <amoscaster@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef ADD_TO_PROJECT_WINDOW_H
#define ADD_TO_PROJECT_WINDOW_H

#include <Box.h>
#include <Button.h>
#include <OutlineListView.h>
#include <ScrollView.h>
#include <String.h>
#include <TextControl.h>
#include <Window.h>

#include <string>

#include "TitleItem.h"


class AddToProjectWindow : public BWindow
{
public:
								AddToProjectWindow(const BString&  projectName,
									const BString& itemPath);
	virtual						~AddToProjectWindow();

	virtual void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();
private:
			void				_Add();
			status_t			_CreateFile(BFile& file, const std::string& filePath);
			BString const		_CurrentDirectory(const BString& itemPath);
			void				_InitWindow();
			void				_ItemChosen(int32 selection);
			status_t			_WriteCCppSourceFile(const std::string& filePath,
									const std::string& fileLeaf);
			status_t			_WriteCppHeaderFile(const std::string& filePath,
									const std::string& fileLeaf);
			status_t			_WriteGenericMakefile(const std::string& filePath);
			status_t			_WriteHaikuMakefile(const std::string& filePath);


			BString	const		fProjectName;
			BString				fCurrentDirectory;
			BOutlineListView*	fItemListView;
			BScrollView*		fItemScrollView;
			BStringItem*		fCppSourceItem;
			BStringItem*		fCSourceItem;
			BStringItem* 		fCppHeaderItem;
			BStringItem* 		fCppClassItem;
			BStringItem* 		fCppMakefileItem;
			BStringItem* 		fHaikuMakefileItem;
			BStringItem* 		fFileItem;
			BStringItem*		fCurrentItem;

			BTextControl* 		fFileName;
			BTextControl* 		fExtensionShown;
			BTextControl* 		fDirectoryPath;
			BButton* 			fAddButton;
};



#endif // ADD_TO_PROJECT_WINDOW_H