/*
 * Copyright 2017 A. Mosca 
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef SETTINGS_WINDOW_H
#define SETTINGS_WINDOW_H

#include <Box.h>
#include <Button.h>
#include <CheckBox.h>
#include <ObjectList.h>
#include <OptionPopUp.h>
#include <OutlineListView.h>
#include <ScrollView.h>
#include <Spinner.h>
#include <StatusBar.h>
#include <StringView.h>
#include <TextControl.h>
#include <Window.h>
#include <map>

#include "TitleItem.h"
#include "TPreferences.h"


class SettingsWindow : public BWindow
{
public:
								SettingsWindow();
	virtual						~SettingsWindow();

	virtual	void				DispatchMessage(BMessage* message,
									BHandler* target);
	virtual void				MessageReceived(BMessage* message);
	virtual bool				QuitRequested();
private:
			void				_ApplyModifications();
			void				_ApplyOrphans();
			void				_CleanApplied();
			void				_InitWindow();
			status_t			_LoadFromFile(BControl* control,
									 bool loadAll = false);
			void				_ManageModifications(BControl* control,
									 bool isModified = true);
			void				_MapPages();
			BView*				_PageBuildView();
			BView*				_PageEditorView();
			BView*				_PageEditorViewVisual();
			BView*				_PageGeneralView();
			BView*				_PageGeneralViewStartup();
			BView*				_PageNotificationsView();
			void				_RevertModifications();
			void				_ShowOrphans();
			void				_ShowView(BStringItem *);
			status_t			_StoreToFile(BControl* control);
			status_t			_StoreToFileDefaults();
			void				_UpdateText();
			void				_UpdateTrailing();
private: // Controls

typedef std::map<BStringItem*, BString> ViewPageMap;
typedef std::pair<BStringItem*, BString> ViewPagePair;
typedef std::map<BStringItem*, BString>::const_iterator ViewPageIterator;

			ViewPageMap			fViewPageMap;
			TPreferences*		fWindowSettingsFile;
			BObjectList<BControl>*		fModifiedList;
			BObjectList<BControl>*		fAppliedList;
			BObjectList<BControl>*		fOrphansList;

			BOutlineListView*	fSettingsOutline;
			BScrollView*		fSettingsScroll;

			BView*				fSettingsBaseView;
			BString				fNextView;
			BString				fCurrentView;

			// Items
			TitleItem*			fGeneralTitle;
			BStringItem*		fGeneralStartupItem;
			TitleItem*			fEditorTitle;
			BStringItem*		fEditorVisualItem;
			TitleItem*			fNotificationsTitle;
			TitleItem*			fBuildTitle;

			// General page
			BTextControl*		fProjectsDirectory;
			BBox*				fGeneralBox;
			BButton*			fBrowseProjectsButton;
			BCheckBox*			fFullPathWindowTitle;
			BOptionPopUp*		fLogDestination;

			// GeneralStartup page
			BBox*				fGeneralStartupBox;
			BCheckBox*			fReopenProjects;
			BCheckBox*			fReopenFiles;
			BCheckBox*			fShowToolBar;
			BCheckBox*			fShowProjectsPanes;
			BCheckBox*			fShowOutputPanes;

			// Editor page
			BBox*				fEditorBox;
			BOptionPopUp*		fFontMenuOP;
			BOptionPopUp*		fEditorFontSizeOP;
			BCheckBox*			fSyntaxHighlight;
			BSpinner*			fTabWidthSpinner;
			BCheckBox*			fBraceMatch;
			BCheckBox*			fSaveCaret;

			// EditorVisual page
			BBox*				fEditorVisualBox;
			BTextControl*		fEdgeLineColumn;
			BCheckBox*			fShowEdgeLine;
			BCheckBox*			fShowLineNumber;
			BCheckBox*			fShowCommentMargin;
			BCheckBox*			fMarkCaretLine;
			BCheckBox*			fEnableFolding;

			// Notifications page
			BBox*				fNotificationsBox;
			BCheckBox*			fEnableNotifications;

			// Build page
			BBox*				fBuildBox;
			BCheckBox*			fWrapConsoleEnabled;
			BCheckBox*			fConsoleBannerEnabled;

			// Buttons
			BButton*			fApplyButton;
			BButton*			fDefaultButton;
			BButton*			fExitButton;
			BButton*			fRevertButton;

			BStatusBar*			fStatusBar;

private: // File Data
			int32				fControlsCount;
			int32				fControlsDone;
			int32				fAppliedModifications;
			int32				fUseCount;
			BFont				fEditorFont;
			BStringView* 		fPreviewText;
};


#endif // SETTINGS_WINDOW_H
