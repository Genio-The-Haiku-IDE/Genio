/*
 * Copyright 2017 A. Mosca 
 * All rights reserved. Distributed under the terms of the MIT license.
 *
 * SettingsWindow class
 *
 * This class manages general settings from Window->Settings menu.
 * (Program settings, like restoring window position, and Project-specific
 * settings are managed elsewhere).
 *
 * Settings are stored in Genio/Genio.settings file in default user settings
 * directory.
 *
 * Features:
 *
 * Settings are managed through Pages(read Views) mapped with an OutlineView List.
 *
 * Modifications are user-visible and persistent across pages (orange).
 * Saved modifications are user-visible and persistent across pages (light blue).
 * On executing a new app version, New controls may be user-visible (green).
 *
 * Revert and Apply buttons are appliable everywhere in presence of modifications.
 *
 * Only modified items are saved on disk.
 *
 */
#include "SettingsWindow.h"

#include <Application.h>
#include <Catalog.h>
#include <GridLayoutBuilder.h>
#include <GroupLayoutBuilder.h>
#include <LayoutBuilder.h>
#include <SeparatorView.h>
#include <iostream>

#include "DefaultSettingsKeys.h"
#include "GenioNamespace.h"
#include "Logger.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SettingsWindow"

//static const rgb_color kSettingsRed = { 255, 60, 40, 255 };
static const rgb_color kSettingsOrange = { 255, 69, 0, 255 };
static const rgb_color kSettingsAzure = { 100, 200, 255, 255 };
static const rgb_color kSettingsGreen = { 50, 150, 50, 255 };

enum {
	MSG_BRACE_MATCHING_TOGGLED				= 'brma',
	MSG_BUTTON_APPLY_CLICKED				= 'buap',
	MSG_BUTTON_DEFAULT_CLICKED				= 'bude',
	MSG_BUTTON_CANCEL_CLICKED				= 'bucl',
	MSG_BUTTON_REVERT_CLICKED				= 'bure',
	MSG_EDGE_LINE_COLUMN_CHANGED			= 'elco',
	MSG_EDITOR_FONT_CHANGED					= 'edfo',
	MSG_EDITOR_FONT_SIZE_CHANGED			= 'edfs',
	MSG_ENABLE_NOTIFICATIONS_TOGGLED		= 'enno',
	MSG_FOLDING_ENABLED						= 'fold',
	MSG_FULL_PATH_TOGGLED					= 'fupa',
	MSG_MARK_CARET_LINE_TOGGLED				= 'mcli',
	MSG_PAGE_CHOSEN							= 'page',
	MSG_PROJECT_DIRECTORY_EDITED			= 'prdi',
	MSG_REOPEN_PROJECTS_TOGGLED				= 'repr',
	MSG_REOPEN_FILES_TOGGLED				= 'refi',
	MSG_SAVE_CARET_TOGGLED					= 'sace',
	MSG_TRIM_WHITESPACE_TOGGLED				= 'trws',
	MSG_SHOW_EDGE_LINE_TOGGLED				= 'seli',
	MSG_SHOW_LINE_NUMBER_TOGGLED			= 'slnu',
	MSG_SHOW_COMMENT_MARGIN_TOGGLED			= 'scmt',
	MSG_SHOW_OUTPUT_PANES_TOGGLED			= 'sopa',
	MSG_SHOW_PROJECTS_PANES_TOGGLED			= 'sppa',
	MSG_SHOW_TOOLBAR_TOGGLED				= 'shto',
	MSG_SYNTAX_HIGHLIGHT_TOGGLED			= 'syhi',
	MSG_TAB_WIDTH_CHANGED					= 'tawi',
	MSG_LOG_DESTINATION_CHANGED				= 'lodc',
	MSG_LOG_LEVEL_CHANGED					= 'lolc',
	MSG_WRAP_CONSOLE_ENABLED				= 'wcen',
	MSG_CONSOLE_BANNER_ENABLED				= 'cben',
	MSG_BUILD_ON_SAVE_ENABLED				= 'sabd',
	MSG_SAVE_ON_BUILD_ENABLED				= 'bdsa'
};

SettingsWindow::SettingsWindow()
	:
	BWindow(BRect(0, 0, 799, 599), "Settings", B_MODAL_WINDOW,
													B_ASYNCHRONOUS_CONTROLS |
													B_NOT_ZOOMABLE |
													B_NOT_RESIZABLE |
													B_AVOID_FRONT |
													B_AUTO_UPDATE_SIZE_LIMITS |
													B_CLOSE_ON_ESCAPE)
	, fCurrentView("GeneralPage")
	, fAppliedModifications(0)
{
	_InitWindow();

	// Map Items
	_MapPages();

	// Set fSettingsOutline minimum width
	// done after _MapPages because it fills fViewPageMap
	float maxTitleSize = 0;
	ViewPageMap::const_iterator i;
	for (i = fViewPageMap.begin(); i != fViewPageMap.end(); i++) {
		float titleSize = fSettingsOutline->StringWidth(i->first->Text());
		if (titleSize > maxTitleSize)
			maxTitleSize = titleSize;
	}
	fSettingsOutline->SetExplicitMinSize(BSize(maxTitleSize + 20, B_SIZE_UNSET));

	// Modified and Applied Controls List
	fModifiedList = new BObjectList<BControl>();
	fAppliedList = new BObjectList<BControl>();

	// Load control values from file
	_LoadFromFile(nullptr, true);
}

SettingsWindow::~SettingsWindow()
{
	delete fModifiedList;
	delete fAppliedList;
	delete fGeneralTitle;
	delete fGeneralStartupItem;
	delete fEditorTitle;
	delete fEditorVisualItem;
	delete fNotificationsTitle;
	delete fBuildTitle;
}

void
SettingsWindow::DispatchMessage(BMessage* message, BHandler* handler)
{
	if (message->what == B_KEY_DOWN) {
		int8 key;
		int32 modifiers;
		if (message->FindInt8("byte", 0, &key) == B_OK
			&& message->FindInt32("modifiers", &modifiers) == B_OK) {
			if ((modifiers & B_SHIFT_KEY && key == 'D')) {
				fDefaultButton->SetEnabled(true);
			}

		}
	}
	// Maybe press and hold for default button
//	if (message->what == B_KEY_UP)
//				fDefaultButton->SetEnabled(false);

	BWindow::DispatchMessage(message, handler);
}

void
SettingsWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case MSG_BRACE_MATCHING_TOGGLED: {
			bool modified = fBraceMatch->Value() !=
								GenioNames::Settings.brace_match;
				_ManageModifications(fBraceMatch, modified);
			break;
		}
		case MSG_BUTTON_APPLY_CLICKED:
			_ApplyModifications();
			break;
		case MSG_BUTTON_DEFAULT_CLICKED: {
			_RevertModifications();
			_StoreToFileDefaults();
			_LoadFromFile(nullptr, true);
			_CleanApplied();
			break;
		}
		case MSG_BUTTON_CANCEL_CLICKED: {
			PostMessage(B_QUIT_REQUESTED);
			break;
		}
		case MSG_BUTTON_REVERT_CLICKED: {
			_RevertModifications();
			break;
		}
		case MSG_EDGE_LINE_COLUMN_CHANGED: {
			bool modified = strcmp(fEdgeLineColumn->Text(),
					GenioNames::Settings.edgeline_column)  != 0;
				_ManageModifications(fEdgeLineColumn, modified);

			break;
		}
		case MSG_EDITOR_FONT_SIZE_CHANGED: {
			// TODO SetViewColor not working
			if (fEditorFontSizeOP->Value() > 0)
				fEditorFont.SetSize(fEditorFontSizeOP->Value());
			else
				fEditorFont.SetSize(be_plain_font->Size());

			fPreviewText->SetFont(&fEditorFont);

			bool modified = fEditorFontSizeOP->Value() !=
								GenioNames::Settings.edit_fontsize;
				_ManageModifications(fEditorFontSizeOP, modified);
			break;
		}
		case MSG_ENABLE_NOTIFICATIONS_TOGGLED: {
			bool modified = fEnableNotifications->Value() !=
								GenioNames::Settings.enable_notifications;
				_ManageModifications(fEnableNotifications, modified);
			break;
		}
		case MSG_FOLDING_ENABLED: {
			bool modified = fEnableFolding->Value() !=
								GenioNames::Settings.enable_folding;
				_ManageModifications(fEnableFolding, modified);
			break;
		}
		case MSG_FULL_PATH_TOGGLED: {
			bool modified = fFullPathWindowTitle->Value() !=
								GenioNames::Settings.fullpath_title;
				_ManageModifications(fFullPathWindowTitle, modified);
			break;
		}
		case MSG_MARK_CARET_LINE_TOGGLED: {
			bool modified = fMarkCaretLine->Value() !=
								GenioNames::Settings.mark_caretline;
				_ManageModifications(fMarkCaretLine, modified);
			break;
		}
		case MSG_PAGE_CHOSEN: {
			int32 selection = fSettingsOutline->CurrentSelection();
			if (selection >= 0) {
				BStringItem *item = dynamic_cast<BStringItem*>(fSettingsOutline->ItemAt(selection));
				if (item != nullptr)
					_ShowView(item);
			}
			break;
		}
		case MSG_PROJECT_DIRECTORY_EDITED: {
			bool modified = strcmp(fProjectsDirectory->Text(),
					GenioNames::Settings.projects_directory) != 0;
				_ManageModifications(fProjectsDirectory, modified);

			break;
		}
		case MSG_REOPEN_PROJECTS_TOGGLED: {
			bool modified = fReopenProjects->Value() !=
								GenioNames::Settings.reopen_projects;
				_ManageModifications(fReopenProjects, modified);
			break;
		}
		case MSG_REOPEN_FILES_TOGGLED: {
			bool modified = fReopenFiles->Value() !=
								GenioNames::Settings.reopen_files;
				_ManageModifications(fReopenFiles, modified);
			break;
		}
		case MSG_SAVE_CARET_TOGGLED: {
			bool modified = fSaveCaret->Value() !=
								GenioNames::Settings.save_caret;
				_ManageModifications(fSaveCaret, modified);
			break;
		}
		case MSG_TRIM_WHITESPACE_TOGGLED: {
			bool modified = fTrimWhitespace->Value() !=
								GenioNames::Settings.trim_trailing_whitespace;
				_ManageModifications(fTrimWhitespace, modified);
			break;
		}
		case MSG_SHOW_EDGE_LINE_TOGGLED: {
			bool modified = fShowEdgeLine->Value() !=
								GenioNames::Settings.show_edgeline;
				_ManageModifications(fShowEdgeLine, modified);
			break;
		}
		case MSG_SHOW_LINE_NUMBER_TOGGLED: {
			bool modified = fShowLineNumber->Value() !=
								GenioNames::Settings.show_linenumber;
				_ManageModifications(fShowLineNumber, modified);
			break;
		}
		case MSG_SHOW_COMMENT_MARGIN_TOGGLED: {
			bool modified = fShowCommentMargin->Value() !=
								GenioNames::Settings.show_commentmargin;
				_ManageModifications(fShowCommentMargin, modified);
			break;
		}
		case MSG_SHOW_OUTPUT_PANES_TOGGLED: {
			bool modified = fShowOutputPanes->Value() != GenioNames::Settings.show_output;
				_ManageModifications(fShowOutputPanes, modified);
		}
		case MSG_SHOW_PROJECTS_PANES_TOGGLED: {
			bool modified = fShowProjectsPanes->Value() !=
								GenioNames::Settings.show_projects;
				_ManageModifications(fShowProjectsPanes, modified);
			break;
		}
		case MSG_SHOW_TOOLBAR_TOGGLED: {
			bool modified = fShowToolBar->Value() != GenioNames::Settings.show_toolbar;
				_ManageModifications(fShowToolBar, modified);
			break;
		}
		case MSG_SYNTAX_HIGHLIGHT_TOGGLED: {
			bool modified = fSyntaxHighlight->Value() !=
								GenioNames::Settings.syntax_highlight;
				_ManageModifications(fSyntaxHighlight, modified);
			break;
		}
		case MSG_TAB_WIDTH_CHANGED: {
			// TODO SetViewColor not working
			bool modified = fTabWidthSpinner->Value() !=
								GenioNames::Settings.tab_width;
				_ManageModifications(fTabWidthSpinner, modified);
			break;
		}
		case MSG_LOG_DESTINATION_CHANGED: {
			bool modified = fLogDestination->Value() !=
								GenioNames::Settings.log_destination;
				_ManageModifications(fLogDestination, modified);
		}
		case MSG_LOG_LEVEL_CHANGED: {
			bool modified = fLogLevel->Value() !=
								GenioNames::Settings.log_level;
				_ManageModifications(fLogLevel, modified);
		}
		case MSG_WRAP_CONSOLE_ENABLED: {
			bool modified = fWrapConsoleEnabled->Value() !=
								GenioNames::Settings.wrap_console;
				_ManageModifications(fWrapConsoleEnabled, modified);
			break;
		}
		case MSG_CONSOLE_BANNER_ENABLED: {
			bool modified = fConsoleBannerEnabled->Value() !=
								GenioNames::Settings.console_banner;
				_ManageModifications(fConsoleBannerEnabled, modified);
			break;
		}
		case MSG_BUILD_ON_SAVE_ENABLED: {
			bool modified = fBuildOnSave->Value() !=
								GenioNames::Settings.build_on_save;
				_ManageModifications(fBuildOnSave, modified);
			break;
		}
		case MSG_SAVE_ON_BUILD_ENABLED: {
			bool modified = fSaveOnBuild->Value() !=
								GenioNames::Settings.save_on_build;
				_ManageModifications(fSaveOnBuild, modified);
			break;
		}
		default: {
			BWindow::MessageReceived(msg);
			break;
		}
	}
}

bool
SettingsWindow::QuitRequested()
{
	return true;
}


void
SettingsWindow::_ApplyModifications()
{
	int32 modifications = fModifiedList->CountItems();
	for (int32 item = modifications - 1; item >= 0 ; item--) {
		if (_StoreToFile(fModifiedList->ItemAt(item)) != B_OK) {
		// TODO notify
			continue;
		}
		fModifiedList->ItemAt(item)->SetViewColor(kSettingsAzure);
		fModifiedList->ItemAt(item)->Invalidate();
		fAppliedList->AddItem(fModifiedList->ItemAt(item));
		fModifiedList->RemoveItem(fModifiedList->ItemAt(item));
		fAppliedModifications++;
	}

	GenioNames::SaveSettingsVars();

	fApplyButton->SetEnabled(false);
	fRevertButton->SetEnabled(false);
	GenioNames::LoadSettingsVars();

	// TODO: Advertise changed settings: currently they aren't "live",
	// and often require closing and reopening editor windows, or
	// even closing and reopening Genio

	// If full path window title has been set off, clean title
	// TODO: and do otherwise, if it's on
	if (GenioNames::Settings.fullpath_title == false)
		be_app->WindowAt(0)->SetTitle(GenioNames::kApplicationName);

	// TODO: Move this to the proper place
	Logger::SetDestination(GenioNames::Settings.log_destination);
	Logger::SetLevel(log_level(GenioNames::Settings.log_level));
}


void
SettingsWindow::_CleanApplied()
{
	for (int32 item = fAppliedList->CountItems() -1; item >= 0 ; item--) {
		if (fAppliedList->ItemAt(item) == nullptr)
			continue;
		fAppliedList->ItemAt(item)->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		fAppliedList->ItemAt(item)->Invalidate();
		fAppliedList->RemoveItem(fAppliedList->ItemAt(item));
		fAppliedModifications --;
	}
}

void
SettingsWindow::_InitWindow()
{
	// Outline
	fSettingsOutline = new BOutlineListView("SettingsOutline", B_SINGLE_SELECTION_LIST);
	fSettingsOutline->SetSelectionMessage(new BMessage(MSG_PAGE_CHOSEN));

	fSettingsScroll = new BScrollView("SettingsScroll",
		fSettingsOutline, B_FRAME_EVENTS | B_WILL_DRAW, false, true, B_FANCY_BORDER);

	// List Items
	fGeneralTitle = new TitleItem(B_TRANSLATE("General"));
	fGeneralStartupItem = new BStringItem(B_TRANSLATE("Startup"), 1, true);
	fEditorTitle = new TitleItem(B_TRANSLATE("Editor"));
	fEditorVisualItem = new BStringItem(B_TRANSLATE("Visual"), 1, true);
	fNotificationsTitle = new TitleItem(B_TRANSLATE("Notifications"));
	fBuildTitle = new TitleItem(B_TRANSLATE("Build"));

	fSettingsOutline->AddItem(fGeneralTitle);
	fSettingsOutline->AddItem(fGeneralStartupItem);
	fSettingsOutline->AddItem(fEditorTitle);
	fSettingsOutline->AddItem(fEditorVisualItem);
	fSettingsOutline->AddItem(fNotificationsTitle);
	fSettingsOutline->AddItem(fBuildTitle);

	// Base view for settings Pages
	fSettingsBaseView = new BView("SettingsBaseView", 0, new BGroupLayout(B_VERTICAL));
	fSettingsBaseView->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	// Status Bar
	fStatusBar = new BStatusBar("StatusBar");
	fStatusBar->SetBarHeight(1.0);

	// Buttons
	fRevertButton = new BButton(B_TRANSLATE("Revert"), new BMessage(MSG_BUTTON_REVERT_CLICKED));
	fDefaultButton = new BButton(B_TRANSLATE("Default"), new BMessage(MSG_BUTTON_DEFAULT_CLICKED));
	fExitButton = new BButton(B_TRANSLATE("Cancel"), new BMessage(MSG_BUTTON_CANCEL_CLICKED));
	fApplyButton = new BButton(B_TRANSLATE("Apply"), new BMessage(MSG_BUTTON_APPLY_CLICKED));

	fRevertButton->SetEnabled(false);
	fDefaultButton->SetEnabled(false);
	fApplyButton->SetEnabled(false);

	// Window layout
	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.AddGroup(B_HORIZONTAL)
			.Add(fSettingsScroll, 4.0f)
			.Add(fSettingsBaseView, 9.0f)
		.End() // central group
		.Add(fStatusBar)
		.AddGroup(B_HORIZONTAL)
			.Add(fRevertButton)
			.Add(fDefaultButton)
			.AddGlue()
			.Add(fExitButton)
			.Add(fApplyButton)
		.End() // buttons group
	;

	// Load all Pages
	fSettingsBaseView->AddChild(_PageGeneralView());
	fSettingsBaseView->AddChild(_PageGeneralViewStartup());
	fSettingsBaseView->AddChild(_PageEditorView());
	fSettingsBaseView->AddChild(_PageEditorViewVisual());
	fSettingsBaseView->AddChild(_PageNotificationsView());
	fSettingsBaseView->AddChild(_PageBuildView());

	// Hide all except first one
//	fSettingsBaseView->FindView("GeneralPage")->Hide();
	fSettingsBaseView->FindView("GeneralStartupPage")->Hide();
	fSettingsBaseView->FindView("EditorPage")->Hide();
	fSettingsBaseView->FindView("EditorVisualPage")->Hide();
	fSettingsBaseView->FindView("NotificationsPage")->Hide();
	fSettingsBaseView->FindView("BuildPage")->Hide();

	// Select first one
	fSettingsOutline->Select(0);

	// Center window
	CenterOnScreen();
}

status_t
SettingsWindow::_LoadFromFile(BControl* control, bool loadAll /*= false*/)
{
	if (loadAll == true) {
		fControlsCount = 0;
		fControlsDone = 0;
	}

	// TODO manage/notify errors
	BString stringVal;
	int32 intVal;

	// General Page
	if (control == fProjectsDirectory || loadAll == true) {
		stringVal = GenioNames::Settings.projects_directory;
		fControlsCount += loadAll == true;
		fProjectsDirectory->SetText(stringVal);
	}
	if (control == fFullPathWindowTitle || loadAll == true) {
		intVal = GenioNames::Settings.fullpath_title;
		fControlsCount += loadAll == true;
		fFullPathWindowTitle->SetValue(intVal);
	}
	if (control == fLogDestination || loadAll == true) {
		intVal = GenioNames::Settings.log_destination;
		fControlsCount += loadAll == true;
		fLogDestination->SetValue(intVal);
		fControlsDone += loadAll == true;
	}
	if (control == fLogLevel || loadAll == true) {
		intVal = GenioNames::Settings.log_level;
		fControlsCount += loadAll == true;
		fLogLevel->SetValue(intVal);
		fControlsDone += loadAll == true;
	}
	// General Startup Page
	if (control == fReopenProjects || loadAll == true) {
		intVal = GenioNames::Settings.reopen_projects;
		fControlsCount += loadAll == true;
		fReopenProjects->SetValue(intVal);
		fControlsDone += loadAll == true;
	}
	if (control == fReopenFiles || loadAll == true) {
		intVal = GenioNames::Settings.reopen_files;
		fControlsCount += loadAll == true;
		fReopenFiles->SetValue(intVal);
		fControlsDone += loadAll == true;
	}
	if (control == fShowProjectsPanes || loadAll == true) {
		intVal = GenioNames::Settings.show_projects;
		fControlsCount += loadAll == true;
		fShowProjectsPanes->SetValue(intVal);
		fControlsDone += loadAll == true;
	}
	if (control == fShowOutputPanes || loadAll == true) {
		intVal = GenioNames::Settings.show_output;
		fControlsCount += loadAll == true;
		fShowOutputPanes->SetValue(intVal);
		fControlsDone += loadAll == true;
	}
	if (control == fShowToolBar || loadAll == true) {
		intVal = GenioNames::Settings.show_toolbar;
		fControlsCount += loadAll == true;
		fShowToolBar->SetValue(intVal);
		fControlsDone += loadAll == true;
	}

	// Editor Page
	if (control == fEditorFontSizeOP || loadAll == true) {
		intVal = GenioNames::Settings.edit_fontsize;
		fControlsCount += loadAll == true;
		fEditorFontSizeOP->SetValue(intVal);
		fControlsDone += loadAll == true;
	}
	if (control == fSyntaxHighlight || loadAll == true) {
		intVal = GenioNames::Settings.syntax_highlight;
		fControlsCount += loadAll == true;
		fSyntaxHighlight->SetValue(intVal);
		fControlsDone += loadAll == true;
	}
	if (control == fTabWidthSpinner || loadAll == true) {
		intVal = GenioNames::Settings.tab_width;
		fControlsCount += loadAll == true;
		fTabWidthSpinner->SetValue(intVal);
		fControlsDone += loadAll == true;
	}
	if (control == fBraceMatch || loadAll == true) {
		intVal = GenioNames::Settings.brace_match;
		fControlsCount += loadAll == true;
		fBraceMatch->SetValue(intVal);
		fControlsDone += loadAll == true;
	}
	if (control == fSaveCaret || loadAll == true) {
		intVal = GenioNames::Settings.save_caret;
		fControlsCount += loadAll == true;
		fSaveCaret->SetValue(intVal);
		fControlsDone += loadAll == true;
	}
	if (control == fTrimWhitespace || loadAll == true) {
		intVal = GenioNames::Settings.trim_trailing_whitespace;
		fControlsCount += loadAll == true;
		fTrimWhitespace->SetValue(intVal);
		fControlsDone += loadAll == true;
	}
	// Editor Visual Page
	if (control == fShowLineNumber || loadAll == true) {
		intVal = GenioNames::Settings.show_linenumber;
		fControlsCount += loadAll == true;
		fShowLineNumber->SetValue(intVal);
		fControlsDone += loadAll == true;
	}
	if (control == fShowCommentMargin || loadAll == true) {
		intVal = GenioNames::Settings.show_commentmargin;
		fControlsCount += loadAll == true;
		fShowCommentMargin->SetValue(intVal);
		fControlsDone += loadAll == true;
	}
	if (control == fMarkCaretLine || loadAll == true) {
		intVal = GenioNames::Settings.mark_caretline;
		fControlsCount += loadAll == true;
		fMarkCaretLine->SetValue(intVal);
		fControlsDone += loadAll == true;
	}
	if (control == fShowEdgeLine || loadAll == true) {
		intVal = GenioNames::Settings.show_edgeline;
		fControlsCount += loadAll == true;
		fShowEdgeLine->SetValue(intVal);
		fControlsDone += loadAll == true;
	}
	if (control == fEdgeLineColumn || loadAll == true) {
		stringVal = GenioNames::Settings.edgeline_column;
		fControlsCount += loadAll == true;
		fEdgeLineColumn->SetText(stringVal);
		fControlsDone += loadAll == true;
	}
	if (control == fEnableFolding || loadAll == true) {
		intVal = GenioNames::Settings.enable_folding;
		fControlsCount += loadAll == true;
		fEnableFolding->SetValue(intVal);
		fControlsDone += loadAll == true;
	}
	//  Notifications Page
	if (control == fEnableNotifications || loadAll == true) {
		intVal = GenioNames::Settings.enable_notifications;
		fControlsCount += loadAll == true;
		fEnableNotifications->SetValue(intVal);
		fControlsDone += loadAll == true;
	}
	//  Build Page
	if (control == fWrapConsoleEnabled || loadAll == true) {
		intVal = GenioNames::Settings.wrap_console;
		fControlsCount += loadAll == true;
		fWrapConsoleEnabled->SetValue(intVal);
		fControlsDone += loadAll == true;
	}
	if (control == fConsoleBannerEnabled || loadAll == true) {
		intVal = GenioNames::Settings.console_banner;
		fControlsCount += loadAll == true;
		fConsoleBannerEnabled->SetValue(intVal);
		fControlsDone += loadAll == true;
	}
	if (control == fBuildOnSave || loadAll == true) {
		intVal = GenioNames::Settings.build_on_save;
		fControlsCount += loadAll == true;
		fBuildOnSave->SetValue(intVal);
		fControlsDone += loadAll == true;
	}
	if (control == fSaveOnBuild || loadAll == true) {
		intVal = GenioNames::Settings.save_on_build;
		fControlsCount += loadAll == true;
		fSaveOnBuild->SetValue(intVal);
		fControlsDone += loadAll == true;
	}

	return B_OK;
}


void
SettingsWindow::_ManageModifications(BControl* control, bool isModified)
{
	// Item was modified
	if (isModified == true) {
		// Manage multiple modifications for the same control
		if (!fModifiedList->HasItem(control)) {
			std::cout << "modified " << control->Name() << std::endl;
			fModifiedList->AddItem(control);
		}
		// Notify user with a deep red color TODO no numbers

		control->SetViewColor(kSettingsOrange);
		control->Invalidate();

		fApplyButton->SetEnabled(true);
		fRevertButton->SetEnabled(true);
	} else {
		// Item was demodified
		if (fModifiedList->HasItem(control)) {
			control->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
			control->Invalidate();

			fModifiedList->RemoveItem(control);
			// Disable buttons if no modifications
			if (fModifiedList->IsEmpty()) {
				fRevertButton->SetEnabled(false);
			}
		}
	}
}

// Map BStringItem* - BView->Name
void
SettingsWindow::_MapPages()
{
	fViewPageMap.insert(ViewPagePair(fGeneralTitle, "GeneralPage"));
	fViewPageMap.insert(ViewPagePair(fGeneralStartupItem, "GeneralStartupPage"));
	fViewPageMap.insert(ViewPagePair(fEditorTitle, "EditorPage"));
	fViewPageMap.insert(ViewPagePair(fEditorVisualItem, "EditorVisualPage"));
	fViewPageMap.insert(ViewPagePair(fNotificationsTitle, "NotificationsPage"));
	fViewPageMap.insert(ViewPagePair(fBuildTitle, "BuildPage"));
}

// Pages

BView*
SettingsWindow::_PageBuildView()
{
	// "BuildPage" Box
	fBuildBox = new BBox("BuildPage");
	fBuildBox->SetLabel(B_TRANSLATE("Build"));

	fWrapConsoleEnabled = new BCheckBox("WrapConsoleEnabled",
		B_TRANSLATE("Wrap console"), new BMessage(MSG_WRAP_CONSOLE_ENABLED));

	fConsoleBannerEnabled = new BCheckBox("ConsoleBannerEnabled",
		B_TRANSLATE("Console banner"), new BMessage(MSG_CONSOLE_BANNER_ENABLED));

	fBuildOnSave = new BCheckBox("BuildOnSave",
		B_TRANSLATE("Build target on resource save"), new BMessage(MSG_BUILD_ON_SAVE_ENABLED));

	fSaveOnBuild = new BCheckBox("SaveOnBuild",
		B_TRANSLATE("Save changed files on build"), new BMessage(MSG_SAVE_ON_BUILD_ENABLED));

	BView* view = BGridLayoutBuilder(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
		.Add(fWrapConsoleEnabled, 0, 1)
		.Add(fConsoleBannerEnabled, 1, 1)
		.Add(fBuildOnSave, 0, 2)
		.Add(fSaveOnBuild, 1, 2)
		.Add(new BSeparatorView(B_HORIZONTAL, B_PLAIN_BORDER), 0, 3, 4)
		.Add(BSpaceLayoutItem::CreateGlue(), 0, 4)
		.SetInsets(10, 10, 10, 10)
	.View();

	fBuildBox->AddChild(view);

	return fBuildBox;

}

BView*
SettingsWindow::_PageEditorView()
{
	// "EditorPage" Box
	fEditorBox = new BBox("EditorPage");
	fEditorBox->SetLabel(B_TRANSLATE("Editor"));

	fFontMenuOP = new BOptionPopUp("FontMenu",
		B_TRANSLATE("Font:"), new BMessage(MSG_EDITOR_FONT_CHANGED));

//	fFontMenuOP->SetExplicitMaxSize(BSize(400, B_SIZE_UNSET));

	BFont font(be_plain_font);
	if (GenioNames::Settings.edit_fontsize > 0)
		font.SetSize(GenioNames::Settings.edit_fontsize);

	// preview
	fPreviewText = new BStringView("preview text",
		B_TRANSLATE("This is the font size preview"));

	fPreviewText->SetFont(&font);

//	fPreviewText->SetExplicitMaxSize(BSize(600, 40));
//fPreviewText->SetAlignment(B_ALIGN_LEFT);

	// box around preview
	BBox* fPreviewBox = new BBox("PreviewBox", B_WILL_DRAW | B_FRAME_EVENTS);
	fPreviewBox->AddChild(BGroupLayoutBuilder(B_HORIZONTAL)
		.Add(fPreviewText)
		.SetInsets(B_USE_SMALL_SPACING, B_USE_SMALL_SPACING,
			B_USE_SMALL_SPACING, B_USE_SMALL_SPACING)
		.TopView()
	);


	fEditorFontSizeOP = new BOptionPopUp("EditSizeMenu",
		B_TRANSLATE("Font size:"), new BMessage(MSG_EDITOR_FONT_SIZE_CHANGED));

	fEditorFontSizeOP->AddOption(B_TRANSLATE("Default size"), -1);

	for (int32 i = 10; i <= 18; i++) {
		BString text;
		text << i;
		fEditorFontSizeOP->AddOption(text.String(), i);
	}

	fSyntaxHighlight = new BCheckBox("SyntaxHighlight",
		B_TRANSLATE("Enable syntax highlighting"), new BMessage(MSG_SYNTAX_HIGHLIGHT_TOGGLED));
	fBraceMatch = new BCheckBox("BraceMatch",
		B_TRANSLATE("Enable brace matching"), new BMessage(MSG_BRACE_MATCHING_TOGGLED));
	fSaveCaret = new BCheckBox("SaveCaret",
		B_TRANSLATE("Save caret position"), new BMessage(MSG_SAVE_CARET_TOGGLED));
	// TODO: "Trim trailing whitespace on save" is too long: reword
	fTrimWhitespace = new BCheckBox("TrimWhitespace",
		B_TRANSLATE("Trim trailing whitespace on save"), new BMessage(MSG_TRIM_WHITESPACE_TOGGLED));
	fTabWidthSpinner = new BSpinner("TabWidth",
		B_TRANSLATE("Tab width:  "), new BMessage(MSG_TAB_WIDTH_CHANGED));
	fTabWidthSpinner->SetRange(1, 8);
	fTabWidthSpinner->SetAlignment(B_ALIGN_RIGHT);

	BView* view = BGridLayoutBuilder(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
		.Add(fFontMenuOP, 0, 0, 2)
		.Add(fEditorFontSizeOP, 3, 0)
		.Add(fPreviewBox, 0, 1, 3)
		.Add(fSyntaxHighlight, 0, 2)
		.Add(fBraceMatch, 0, 3)
		.Add(fSaveCaret, 0, 4)
		.Add(fTrimWhitespace, 0, 5)
		.Add(fTabWidthSpinner, 0, 6, 1)
		.Add(new BSeparatorView(B_HORIZONTAL, B_PLAIN_BORDER), 0, 7, 6)
		.Add(BSpaceLayoutItem::CreateGlue(), 0, 8)
		.SetInsets(10, 10, 10, 10)
	.View();

	fEditorBox->AddChild(view);

	return fEditorBox;
}

BView*
SettingsWindow::_PageEditorViewVisual()
{
	// "EditorVisualPage" Box
	fEditorVisualBox = new BBox("EditorVisualPage");
	fEditorVisualBox->SetLabel(B_TRANSLATE("Visual"));

	fShowLineNumber = new BCheckBox("ShowLineNumber",
		B_TRANSLATE("Show line number"), new BMessage(MSG_SHOW_LINE_NUMBER_TOGGLED));
	fShowCommentMargin = new BCheckBox("ShowCommentMargin",
		B_TRANSLATE("Show comment margin"), new BMessage(MSG_SHOW_COMMENT_MARGIN_TOGGLED));
	fMarkCaretLine = new BCheckBox("MarkCaretLine",
		B_TRANSLATE("Mark caret line"), new BMessage(MSG_MARK_CARET_LINE_TOGGLED));
	fShowEdgeLine = new BCheckBox("ShowEdgeLine",
		B_TRANSLATE("Show edge line"), new BMessage(MSG_SHOW_EDGE_LINE_TOGGLED));
	fEdgeLineColumn = new BTextControl("EdgeLineColumn",
		B_TRANSLATE("Edge column:"), "", new BMessage());
	fEdgeLineColumn->SetModificationMessage(new BMessage(MSG_EDGE_LINE_COLUMN_CHANGED));
	fEnableFolding = new BCheckBox("EnableFolding",
		B_TRANSLATE("Enable folding"), new BMessage(MSG_FOLDING_ENABLED));

	fEdgeLineColumn->TextView()->SetMaxBytes(3);
	for (auto i = 0; i < 256; i++)
		if (i < '0' || i > '9')
			fEdgeLineColumn->TextView()->DisallowChar(i);
	fEdgeLineColumn->TextView()->SetAlignment(B_ALIGN_RIGHT);
	float w = fEdgeLineColumn->StringWidth("123") + 10;
	fEdgeLineColumn->CreateTextViewLayoutItem()->SetExplicitMaxSize(BSize(w, B_SIZE_UNSET));

	BView* view = BGridLayoutBuilder(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
		.Add(fShowLineNumber, 0, 0)
		.Add(fEnableFolding, 1, 0)
		.Add(fShowCommentMargin, 2, 0)
		.Add(fMarkCaretLine, 0, 1)
		.Add(fShowEdgeLine, 0, 2)
		.Add(fEdgeLineColumn->CreateLabelLayoutItem(), 1, 2)
		.Add(fEdgeLineColumn->CreateTextViewLayoutItem(), 2, 2)
		.Add(BSpaceLayoutItem::CreateGlue(), 3, 2)
		.Add(new BSeparatorView(B_HORIZONTAL, B_PLAIN_BORDER), 0, 3, 4)
		.Add(BSpaceLayoutItem::CreateGlue(), 0, 4)
		.SetInsets(10, 10, 10, 10)
	.View();

	fEditorVisualBox->AddChild(view);

	return fEditorVisualBox;
}

BView*
SettingsWindow::_PageGeneralView()
{
	// "General" Box
	fGeneralBox = new BBox("GeneralPage");
	fGeneralBox->SetLabel(B_TRANSLATE("General"));

	// Text Control
	fProjectsDirectory = new BTextControl("ProjectsDirectory",
		B_TRANSLATE("Projects folder:"), "", nullptr);
	fProjectsDirectory->SetModificationMessage(new BMessage(MSG_PROJECT_DIRECTORY_EDITED));

	// Button disabled now
	fBrowseProjectsButton = new BButton(B_TRANSLATE("Browse" B_UTF8_ELLIPSIS), nullptr);
//		new BMessage(MSG_BUTTON_BROWSE_CLICKED));
	fBrowseProjectsButton->SetEnabled(false);

	// Check Box
	fFullPathWindowTitle = new BCheckBox("FullPathWindowTitle",
		B_TRANSLATE("Show full path in window title"), new BMessage(MSG_FULL_PATH_TOGGLED));

	fLogDestination = new BOptionPopUp("LogDestination",
		B_TRANSLATE("Log destination:"), new BMessage(MSG_LOG_DESTINATION_CHANGED));
	fLogDestination->AddOptionAt("Stdout", Logger::LOGGER_DEST_STDOUT, 0);
	fLogDestination->AddOptionAt("Stderr", Logger::LOGGER_DEST_STDERR, 1);
	fLogDestination->AddOptionAt("Syslog", Logger::LOGGER_DEST_SYSLOG, 2);
	fLogDestination->AddOptionAt("BeDC",   Logger::LOGGER_DEST_BEDC,   3);

	fLogLevel = new BOptionPopUp("LogLevel",
		B_TRANSLATE("Log level:"), new BMessage(MSG_LOG_LEVEL_CHANGED));
	fLogLevel->AddOptionAt("Off", 1, 0);
	fLogLevel->AddOptionAt("Error", 2, 1);
	fLogLevel->AddOptionAt("Info", 3, 2);
	fLogLevel->AddOptionAt("Debug", 4, 3);
	fLogLevel->AddOptionAt("Trace", 5, 4);

	BView* view = BGridLayoutBuilder(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
		.Add(fProjectsDirectory->CreateLabelLayoutItem(), 0, 0)
		.Add(fProjectsDirectory->CreateTextViewLayoutItem(), 1, 0, 2)
		.Add(fBrowseProjectsButton, 3, 0)
		.Add(fFullPathWindowTitle, 1, 1)
		.Add(fLogDestination, 0, 2)
		.Add(fLogLevel, 1, 2)
		.Add(new BSeparatorView(B_HORIZONTAL, B_PLAIN_BORDER), 0, 3, 4)
		.Add(BSpaceLayoutItem::CreateGlue(), 0, 4)
		.SetInsets(10, 10, 10, 10)
	.View();

	fGeneralBox->AddChild(view);

	return fGeneralBox;
}

BView*
SettingsWindow::_PageGeneralViewStartup()
{
	// "Startup" Box
	fGeneralStartupBox = new BBox("GeneralStartupPage");
	fGeneralStartupBox->SetLabel(B_TRANSLATE("Startup"));

	fReopenProjects = new BCheckBox("ReopenProjects",
		B_TRANSLATE("Reload projects"), new BMessage(MSG_REOPEN_PROJECTS_TOGGLED));

	fReopenFiles = new BCheckBox("ReopenFiles",
		B_TRANSLATE("Reload files"), new BMessage(MSG_REOPEN_FILES_TOGGLED));

	fShowProjectsPanes = new BCheckBox("ShowProjectsPanes",
		B_TRANSLATE("Show projects pane"), new BMessage(MSG_SHOW_PROJECTS_PANES_TOGGLED));

	fShowOutputPanes = new BCheckBox("ShowOutputPanes",
		B_TRANSLATE("Show output pane"), new BMessage(MSG_SHOW_OUTPUT_PANES_TOGGLED));

	fShowToolBar = new BCheckBox("ShowToolBar",
		B_TRANSLATE("Show toolbar"), new BMessage(MSG_SHOW_TOOLBAR_TOGGLED));

	BView* view = BLayoutBuilder::Grid<>(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
		.Add(fReopenProjects, 0, 0)
		.Add(fReopenFiles, 1, 0)
		.Add(fShowProjectsPanes, 0, 1)
		.Add(fShowOutputPanes, 1, 1)
		.Add(fShowToolBar, 2, 1)
		.Add(new BSeparatorView(B_HORIZONTAL, B_PLAIN_BORDER), 0, 2, 4)
		.AddGlue(0, 3)
		.SetInsets(10, 10, 10, 10)
		.View();

	fGeneralStartupBox->AddChild(view);

	return fGeneralStartupBox;
}

BView*
SettingsWindow::_PageNotificationsView()
{
	// "" Box
	fNotificationsBox = new BBox("NotificationsPage");
	fNotificationsBox->SetLabel(B_TRANSLATE("Notifications"));

	fEnableNotifications = new BCheckBox("EnableNotifications",
		B_TRANSLATE("Enable notifications"), new BMessage(MSG_ENABLE_NOTIFICATIONS_TOGGLED));

	BView* view = BGridLayoutBuilder(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
		.Add(fEnableNotifications, 0, 1)
		.Add(new BSeparatorView(B_HORIZONTAL, B_PLAIN_BORDER), 0, 2, 4)
		.Add(BSpaceLayoutItem::CreateGlue(), 0, 3)
		.SetInsets(10, 10, 10, 10)
	.View();

	fNotificationsBox->AddChild(view);

	return fNotificationsBox;
}


void
SettingsWindow::_RevertModifications()
{
	for (int32 item = fModifiedList->CountItems() -1; item >= 0 ; item--) {
		_LoadFromFile(fModifiedList->ItemAt(item), false);
		fModifiedList->ItemAt(item)->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		fModifiedList->ItemAt(item)->Invalidate();
		fModifiedList->RemoveItem(fModifiedList->ItemAt(item));
	}

	fRevertButton->SetEnabled(false);
}


// Use a map to tie views with list views
void
SettingsWindow::_ShowView(BStringItem * item)
{
	fNextView = "";

	ViewPageIterator iter = fViewPageMap.find(item);

	if (iter != fViewPageMap.end()) {
		fNextView  = iter->second;
	}

	// If a map was found show the view
	if (fNextView != "") {
		fSettingsBaseView->FindView(fCurrentView)->Hide();
		fSettingsBaseView->FindView(fNextView)->Show();
//		fSettingsBaseView->FindView(fNextView)->MakeFocus(true);
		fCurrentView = fNextView;
	}
}

status_t
SettingsWindow::_StoreToFile(BControl* control)
{
	// General Page
	if (control == fProjectsDirectory)
		GenioNames::Settings.projects_directory = fProjectsDirectory->Text();
	else if (control == fFullPathWindowTitle)
		GenioNames::Settings.fullpath_title = fFullPathWindowTitle->Value();
	else if (control == fLogDestination)
		GenioNames::Settings.log_destination = fLogDestination->Value();
	else if (control == fLogLevel)
		GenioNames::Settings.log_level = fLogLevel->Value();
	// General Startup Page
	else if (control == fReopenProjects)
		GenioNames::Settings.reopen_projects = fReopenProjects->Value();
	else if (control == fReopenFiles)
		GenioNames::Settings.reopen_files = fReopenFiles->Value();
	else if (control == fShowProjectsPanes)
		GenioNames::Settings.show_projects = fShowProjectsPanes->Value();
	else if (control == fShowOutputPanes)
		GenioNames::Settings.show_output = fShowOutputPanes->Value();
	else if (control == fShowToolBar)
		GenioNames::Settings.show_toolbar = fShowToolBar->Value();
	// Editor Page
	else if (control == fEditorFontSizeOP)
		GenioNames::Settings.edit_fontsize = fEditorFontSizeOP->Value();
	else if (control == fSyntaxHighlight)
		GenioNames::Settings.syntax_highlight = fSyntaxHighlight->Value();
	else if (control == fTabWidthSpinner)
		GenioNames::Settings.tab_width = fTabWidthSpinner->Value();
	else if (control == fBraceMatch)
		GenioNames::Settings.brace_match = fBraceMatch->Value();
	else if (control == fSaveCaret)
		GenioNames::Settings.save_caret = fSaveCaret->Value();
	else if (control == fTrimWhitespace)
		GenioNames::Settings.trim_trailing_whitespace = fTrimWhitespace->Value();
	// Editor Visual Page
	else if (control == fShowLineNumber)
		GenioNames::Settings.show_linenumber = fShowLineNumber->Value();
	else if (control == fShowCommentMargin)
		GenioNames::Settings.show_commentmargin = fShowCommentMargin->Value();
	else if (control == fMarkCaretLine)
		GenioNames::Settings.mark_caretline = fMarkCaretLine->Value();
	else if (control == fShowEdgeLine)
		GenioNames::Settings.show_edgeline = fShowEdgeLine->Value();
	else if (control == fEdgeLineColumn)
		GenioNames::Settings.edgeline_column = fEdgeLineColumn->Text();
	else if (control == fEnableFolding)
		GenioNames::Settings.enable_folding = fEnableFolding->Value();

	// Notifications Page
	else if (control == fEnableNotifications)
		GenioNames::Settings.enable_notifications = fEnableNotifications->Value();

	// Build Page
	else if (control == fWrapConsoleEnabled)
		GenioNames::Settings.wrap_console = fWrapConsoleEnabled->Value();
	else if (control == fConsoleBannerEnabled)
		GenioNames::Settings.console_banner = fConsoleBannerEnabled->Value();
	else if (control == fBuildOnSave)
		GenioNames::Settings.build_on_save = fBuildOnSave->Value();
	else if (control == fSaveOnBuild)
		GenioNames::Settings.save_on_build = fSaveOnBuild->Value();
	GenioNames::SaveSettingsVars();
	return B_OK;
}


status_t
SettingsWindow::_StoreToFileDefaults()
{
	// TODO check individual Setting
	
	// General Page
	GenioNames::Settings.projects_directory = kSKProjectsDirectory;
	GenioNames::Settings.fullpath_title = kSKFullPathTitle;
	GenioNames::Settings.log_destination = Logger::LOGGER_DEST_STDOUT;

	// General Startup Page
	GenioNames::Settings.reopen_projects = kSKReopenProjects;
	GenioNames::Settings.reopen_files = kSKReopenFiles;
	GenioNames::Settings.show_projects = kSKShowProjects;
	GenioNames::Settings.show_output = kSKShowOutput;
	GenioNames::Settings.show_toolbar = kSKShowToolBar;

	// Editor Page
	GenioNames::Settings.edit_fontsize = kSKEditorFontSize;
	GenioNames::Settings.syntax_highlight = kSKSyntaxHighlight;
	GenioNames::Settings.tab_width = kSKTabWidth;
	GenioNames::Settings.brace_match = kSKBraceMatch;
	GenioNames::Settings.save_caret = kSKSaveCaret;
	GenioNames::Settings.trim_trailing_whitespace = kSKTrimTrailingWhitespace;

	// Editor Visual Page
	GenioNames::Settings.show_linenumber = kSKShowLineNumber;
	GenioNames::Settings.show_commentmargin = kSKShowCommentMargin;
	GenioNames::Settings.mark_caretline = kSKMarkCaretLine;
	GenioNames::Settings.show_edgeline = kSKShowEdgeLine;
	GenioNames::Settings.edgeline_column = kSKEdgeLineColumn;
	GenioNames::Settings.enable_folding = kSKEnableFolding;

	// Notifications Page
	GenioNames::Settings.enable_notifications = kSKEnableNotifications;

	// Build Page
	GenioNames::Settings.wrap_console = kSKWrapConsole;
	GenioNames::Settings.console_banner = kSKConsoleBanner;
	GenioNames::Settings.editor_zoom = kSKEditorZoom;
	GenioNames::Settings.find_wrap = kSKFindWrap;
	GenioNames::Settings.find_whole_word = kSKFindWholeWord;
	GenioNames::Settings.find_match_case = kSKFindMatchCase;

	GenioNames::Settings.build_on_save = kSKBuildOnSave;
	GenioNames::Settings.save_on_build = kSKSaveOnBuild;
	return B_OK;
}
