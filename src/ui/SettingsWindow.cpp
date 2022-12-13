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
#include <GroupLayoutBuilder.h>
#include <LayoutBuilder.h>
#include <SeparatorView.h>
#include <iostream>

#include "DefaultSettingsKeys.h"
#include "GenioNamespace.h"

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
	MSG_BUTTON_EXIT_CLICKED					= 'buex',
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
	MSG_SHOW_EDGE_LINE_TOGGLED				= 'seli',
	MSG_SHOW_LINE_NUMBER_TOGGLED			= 'slnu',
	MSG_SHOW_COMMENT_MARGIN_TOGGLED			= 'scmt',
	MSG_SHOW_OUTPUT_PANES_TOGGLED			= 'sopa',
	MSG_SHOW_PROJECTS_PANES_TOGGLED			= 'sppa',
	MSG_SHOW_TOOLBAR_TOGGLED				= 'shto',
	MSG_SYNTAX_HIGHLIGHT_TOGGLED			= 'syhi',
	MSG_TAB_WIDTH_CHANGED					= 'tawi',

	MSG_WRAP_CONSOLE_ENABLED				= 'wcen',
	MSG_CONSOLE_BANNER_ENABLED				= 'cben'
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
	// General settings file
	fWindowSettingsFile = new TPreferences(GenioNames::kSettingsFileName,
											GenioNames::kApplicationName, 'IDSE');

	if (fWindowSettingsFile->InitCheck() != B_OK) {
		;// TODO Manage error
	}

	_InitWindow();

	// Map Items
	_MapPages();

	// Modified and Applied Controls List
	fModifiedList = new BObjectList<BControl>();
	fAppliedList = new BObjectList<BControl>();
	fOrphansList= new BObjectList<BControl>();

	// Load control values from file
	_LoadFromFile(nullptr, true);

	// We may be called from App if a new version is detected.
	// Orphans are new controls not yet tied to a file value
	_ShowOrphans();

}

SettingsWindow::~SettingsWindow()
{
	delete fModifiedList;
	delete fAppliedList;
	delete fOrphansList;
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
								fWindowSettingsFile->FindInt32("brace_match");
				_ManageModifications(fBraceMatch, modified);
			break;
		}
		case MSG_BUTTON_APPLY_CLICKED:
			_ApplyOrphans();
			_ApplyModifications();
			break;
		case MSG_BUTTON_DEFAULT_CLICKED: {
			_RevertModifications();
			_StoreToFileDefaults();
			_LoadFromFile(nullptr, true);
			_CleanApplied();

			BString text;
			text << B_TRANSLATE("Set for app ver: ")
				<< fWindowSettingsFile->FindString("app_version") << ",\t"
				<< B_TRANSLATE("Defaults loaded");
			fStatusBar->SetTrailingText(text);

			break;
		}
		case MSG_BUTTON_EXIT_CLICKED: {
			PostMessage(B_QUIT_REQUESTED);
			break;
		}
		case MSG_BUTTON_REVERT_CLICKED: {
			_RevertModifications();
			break;
		}
		case MSG_EDGE_LINE_COLUMN_CHANGED: {
			bool modified = strcmp(fEdgeLineColumn->Text(),
					fWindowSettingsFile->FindString("edgeline_column"))  != 0;
				_ManageModifications(fEdgeLineColumn, modified);

			break;
		}
		case MSG_EDITOR_FONT_SIZE_CHANGED: {
			// TODO SetViewColor not working
			fEditorFont.SetSize(fEditorFontSizeOP->Value());
			fPreviewText->SetFont(&fEditorFont);

			bool modified = fEditorFontSizeOP->Value() !=
								fWindowSettingsFile->FindInt32("edit_fontsize");
				_ManageModifications(fEditorFontSizeOP, modified);
			break;
		}
		case MSG_ENABLE_NOTIFICATIONS_TOGGLED: {
			bool modified = fEnableNotifications->Value() !=
								fWindowSettingsFile->FindInt32("enable_notifications");
				_ManageModifications(fEnableNotifications, modified);
			break;
		}
		case MSG_FOLDING_ENABLED: {
			bool modified = fEnableFolding->Value() !=
								fWindowSettingsFile->FindInt32("enable_folding");
				_ManageModifications(fEnableFolding, modified);
			break;
		}
		case MSG_FULL_PATH_TOGGLED: {
			bool modified = fFullPathWindowTitle->Value() !=
								fWindowSettingsFile->FindInt32("fullpath_title");
				_ManageModifications(fFullPathWindowTitle, modified);
			break;
		}
		case MSG_MARK_CARET_LINE_TOGGLED: {
			bool modified = fMarkCaretLine->Value() !=
								fWindowSettingsFile->FindInt32("mark_caretline");
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
					fWindowSettingsFile->FindString("projects_directory")) != 0;
				_ManageModifications(fProjectsDirectory, modified);

			break;
		}
		case MSG_REOPEN_PROJECTS_TOGGLED: {
			bool modified = fReopenProjects->Value() !=
								fWindowSettingsFile->FindInt32("reopen_projects");
				_ManageModifications(fReopenProjects, modified);
			break;
		}
		case MSG_REOPEN_FILES_TOGGLED: {
			bool modified = fReopenFiles->Value() !=
								fWindowSettingsFile->FindInt32("reopen_files");
				_ManageModifications(fReopenFiles, modified);
			break;
		}
		case MSG_SAVE_CARET_TOGGLED: {
			bool modified = fSaveCaret->Value() !=
								fWindowSettingsFile->FindInt32("save_caret");
				_ManageModifications(fSaveCaret, modified);
			break;
		}
		case MSG_SHOW_EDGE_LINE_TOGGLED: {
			bool modified = fShowEdgeLine->Value() !=
								fWindowSettingsFile->FindInt32("show_edgeline");
				_ManageModifications(fShowEdgeLine, modified);
			break;
		}
		case MSG_SHOW_LINE_NUMBER_TOGGLED: {
			bool modified = fShowLineNumber->Value() !=
								fWindowSettingsFile->FindInt32("show_linenumber");
				_ManageModifications(fShowLineNumber, modified);
			break;
		}
		case MSG_SHOW_COMMENT_MARGIN_TOGGLED: {
			bool modified = fShowCommentMargin->Value() !=
								fWindowSettingsFile->FindInt32("show_commentmargin");
				_ManageModifications(fShowCommentMargin, modified);
			break;
		}
		case MSG_SHOW_OUTPUT_PANES_TOGGLED: {
			bool modified = fShowOutputPanes->Value() != fWindowSettingsFile->FindInt32("show_output");
				_ManageModifications(fShowOutputPanes, modified);
		}
		case MSG_SHOW_PROJECTS_PANES_TOGGLED: {
			bool modified = fShowProjectsPanes->Value() !=
								fWindowSettingsFile->FindInt32("show_projects");
				_ManageModifications(fShowProjectsPanes, modified);
			break;
		}
		case MSG_SHOW_TOOLBAR_TOGGLED: {
			bool modified = fShowToolBar->Value() != fWindowSettingsFile->FindInt32("show_toolbar");
				_ManageModifications(fShowToolBar, modified);
			break;
		}
		case MSG_SYNTAX_HIGHLIGHT_TOGGLED: {
			bool modified = fSyntaxHighlight->Value() !=
								fWindowSettingsFile->FindInt32("syntax_highlight");
				_ManageModifications(fSyntaxHighlight, modified);
			break;
		}
		case MSG_TAB_WIDTH_CHANGED: {
			// TODO SetViewColor not working
			bool modified = fTabWidthSpinner->Value() !=
								fWindowSettingsFile->FindInt32("tab_width");
				_ManageModifications(fTabWidthSpinner, modified);
			break;
		}
		case MSG_WRAP_CONSOLE_ENABLED: {
			bool modified = fWrapConsoleEnabled->Value() !=
								fWindowSettingsFile->FindInt32("wrap_console");
				_ManageModifications(fWrapConsoleEnabled, modified);
			break;
		}
		case MSG_CONSOLE_BANNER_ENABLED: {
			bool modified = fConsoleBannerEnabled->Value() !=
								fWindowSettingsFile->FindInt32("console_banner");
				_ManageModifications(fConsoleBannerEnabled, modified);
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
	delete fWindowSettingsFile;
	// Reload vars from file
	GenioNames::LoadSettingsVars();

	// Do some cleaning
	// If full path window title has been set off, clean title
	if (GenioNames::Settings.fullpath_title == false)
		be_app->WindowAt(0)->SetTitle(GenioNames::kApplicationName);

	// TODO Send a message to reload file settings

	return true;
}

void
SettingsWindow::_ApplyModifications()
{
	int32 modifications = fModifiedList->CountItems();

	for (int32 item = fModifiedList->CountItems() -1; item >= 0 ; item--) {

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

	// If not all modifications were applied, return
	if (fAppliedModifications < modifications)
		return;

	fApplyButton->SetEnabled(false);
	fRevertButton->SetEnabled(false);
	
	// If applied, update markers too
	// TODO Check B_OK
	int32 count = 0;
	fWindowSettingsFile->FindInt32("use_count", &count);
	fWindowSettingsFile->SetInt64("last_used", real_time_clock());
	fWindowSettingsFile->SetInt32("use_count", ++count);
	fWindowSettingsFile->FindInt32("use_count", &fUseCount);

	_UpdateText();
	_UpdateTrailing();
}

void
SettingsWindow::_ApplyOrphans()
{
	int32 orphans = fOrphansList->CountItems();
	int32 appliedOrphans = 0;

	if (orphans > 0) {
		for (int32 item = fOrphansList->CountItems() -1; item >= 0 ; item--) {

			if (_StoreToFile(fOrphansList->ItemAt(item)) != B_OK) {
				// TODO notify
				continue;
			}	

			fOrphansList->ItemAt(item)->SetViewColor(kSettingsAzure);
			fOrphansList->ItemAt(item)->Invalidate();
			fAppliedList->AddItem(fOrphansList->ItemAt(item));
			fOrphansList->RemoveItem(fOrphansList->ItemAt(item));
			appliedOrphans++;
			fControlsDone++;
		}
		// If not all orphans were applied, 
		if (appliedOrphans < orphans)
			return;

		// Reset counter
		fWindowSettingsFile->SetInt32("use_count", 0);
		fWindowSettingsFile->SetInt64("last_used", real_time_clock());
		// Reset app version
		fWindowSettingsFile->SetBString("app_version", GenioNames::GetVersionInfo());
	}
	_UpdateText();
	_UpdateTrailing();
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
	fExitButton = new BButton(B_TRANSLATE("Exit"), new BMessage(MSG_BUTTON_EXIT_CLICKED));
	fApplyButton = new BButton(B_TRANSLATE("Apply"), new BMessage(MSG_BUTTON_APPLY_CLICKED));

	fRevertButton->SetEnabled(false);
	fDefaultButton->SetEnabled(false);
	fApplyButton->SetEnabled(false);

	// Window layout
	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
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
	status_t status;
	BString stringVal;
	int32	intVal;

	if (loadAll == true) {
		fControlsCount = 0;
		fControlsDone = 0;
	}

	// TODO manage/notify errors
	if ((status = fWindowSettingsFile->InitCheck()) != B_OK)
		return status;

	if (fWindowSettingsFile->FindInt32("use_count", &fUseCount) != B_OK)
		fUseCount = -1;

	// General Page
	if (control == fProjectsDirectory || loadAll == true) {
		status = fWindowSettingsFile->FindString("projects_directory", &stringVal);
		fControlsCount += loadAll == true;
		if (status == B_OK) {
			fProjectsDirectory->SetText(stringVal);
			fControlsDone += loadAll == true;
		} else
			fOrphansList->AddItem(fProjectsDirectory);
	}
	if (control == fFullPathWindowTitle || loadAll == true) {
		status = fWindowSettingsFile->FindInt32("fullpath_title", &intVal);
		fControlsCount += loadAll == true;
		if (status == B_OK) {
			fFullPathWindowTitle->SetValue(intVal);
			fControlsDone += loadAll == true;
		} else
			fOrphansList->AddItem(fFullPathWindowTitle);
	}
	// General Startup Page
	if (control == fReopenProjects || loadAll == true) {
		status = fWindowSettingsFile->FindInt32("reopen_projects", &intVal);
		fControlsCount += loadAll == true;
		if (status == B_OK) {
			fReopenProjects->SetValue(intVal);
			fControlsDone += loadAll == true;
		} else
			fOrphansList->AddItem(fReopenProjects);
	}
	if (control == fReopenFiles || loadAll == true) {
		status = fWindowSettingsFile->FindInt32("reopen_files", &intVal);
		fControlsCount += loadAll == true;
		if (status == B_OK) {
			fReopenFiles->SetValue(intVal);
			fControlsDone += loadAll == true;
		} else
			fOrphansList->AddItem(fReopenFiles);
	}
	if (control == fShowProjectsPanes || loadAll == true) {
		status = fWindowSettingsFile->FindInt32("show_projects", &intVal);
		fControlsCount += loadAll == true;
		if (status == B_OK) {
			fShowProjectsPanes->SetValue(intVal);
			fControlsDone += loadAll == true;
		} else
			fOrphansList->AddItem(fShowProjectsPanes);
	}
	if (control == fShowOutputPanes || loadAll == true) {
		status = fWindowSettingsFile->FindInt32("show_output", &intVal);
		fControlsCount += loadAll == true;
		if (status == B_OK) {
			fShowOutputPanes->SetValue(intVal);
			fControlsDone += loadAll == true;
		} else
			fOrphansList->AddItem(fShowOutputPanes);
	}
	if (control == fShowToolBar || loadAll == true) {
		status = fWindowSettingsFile->FindInt32("show_toolbar", &intVal);
		fControlsCount += loadAll == true;
		if (status == B_OK) {
			fShowToolBar->SetValue(intVal);
			fControlsDone += loadAll == true;
		} else
			fOrphansList->AddItem(fShowToolBar);
	}

	// Editor Page
	if (control == fEditorFontSizeOP || loadAll == true) {
		status = fWindowSettingsFile->FindInt32("edit_fontsize", &intVal);
		fControlsCount += loadAll == true;
		if (status == B_OK) {
			fEditorFontSizeOP->SetValue(intVal);
			fControlsDone += loadAll == true;
		} else
			fOrphansList->AddItem(fEditorFontSizeOP);
	}
	if (control == fSyntaxHighlight || loadAll == true) {
		status = fWindowSettingsFile->FindInt32("syntax_highlight", &intVal);
		fControlsCount += loadAll == true;
		if (status == B_OK) {
			fSyntaxHighlight->SetValue(intVal);
			fControlsDone += loadAll == true;
		} else
			fOrphansList->AddItem(fSyntaxHighlight);
	}
	if (control == fTabWidthSpinner || loadAll == true) {
		status = fWindowSettingsFile->FindInt32("tab_width", &intVal);
		fControlsCount += loadAll == true;
		if (status == B_OK) {
			fTabWidthSpinner->SetValue(intVal);
			fControlsDone += loadAll == true;
		} else
			fOrphansList->AddItem(fTabWidthSpinner);
	}
	if (control == fBraceMatch || loadAll == true) {
		status = fWindowSettingsFile->FindInt32("brace_match", &intVal);
		fControlsCount += loadAll == true;
		if (status == B_OK) {
			fBraceMatch->SetValue(intVal);
			fControlsDone += loadAll == true;
		} else
			fOrphansList->AddItem(fBraceMatch);
	}
	if (control == fSaveCaret || loadAll == true) {
		status = fWindowSettingsFile->FindInt32("save_caret", &intVal);
		fControlsCount += loadAll == true;
		if (status == B_OK) {
			fSaveCaret->SetValue(intVal);
			fControlsDone += loadAll == true;
		} else
			fOrphansList->AddItem(fSaveCaret);
	}
	// Editor Visual Page
	if (control == fShowLineNumber || loadAll == true) {
		status = fWindowSettingsFile->FindInt32("show_linenumber", &intVal);
		fControlsCount += loadAll == true;
		if (status == B_OK) {
			fShowLineNumber->SetValue(intVal);
			fControlsDone += loadAll == true;
		} else
			fOrphansList->AddItem(fShowLineNumber);
	}
	if (control == fShowCommentMargin || loadAll == true) {
		status = fWindowSettingsFile->FindInt32("show_commentmargin", &intVal);
		fControlsCount += loadAll == true;
		if (status == B_OK) {
			fShowCommentMargin->SetValue(intVal);
			fControlsDone += loadAll == true;
		} else
			fOrphansList->AddItem(fShowCommentMargin);
	}
	if (control == fMarkCaretLine || loadAll == true) {
		status = fWindowSettingsFile->FindInt32("mark_caretline", &intVal);
		fControlsCount += loadAll == true;
		if (status == B_OK) {
			fMarkCaretLine->SetValue(intVal);
			fControlsDone += loadAll == true;
		} else
			fOrphansList->AddItem(fMarkCaretLine);
	}
	if (control == fShowEdgeLine || loadAll == true) {
		status = fWindowSettingsFile->FindInt32("show_edgeline", &intVal);
		fControlsCount += loadAll == true;
		if (status == B_OK) {
			fShowEdgeLine->SetValue(intVal);
			fControlsDone += loadAll == true;
		} else
			fOrphansList->AddItem(fShowEdgeLine);
	}
	if (control == fEdgeLineColumn || loadAll == true) {
		status = fWindowSettingsFile->FindString("edgeline_column", &stringVal);
		fControlsCount += loadAll == true;
		if (status == B_OK) {
			fEdgeLineColumn->SetText(stringVal);
			fControlsDone += loadAll == true;
		} else
			fOrphansList->AddItem(fEdgeLineColumn);
	}
	if (control == fEnableFolding || loadAll == true) {
		status = fWindowSettingsFile->FindInt32("enable_folding", &intVal);
		fControlsCount += loadAll == true;
		if (status == B_OK) {
			fEnableFolding->SetValue(intVal);
			fControlsDone += loadAll == true;
		} else
			fOrphansList->AddItem(fEnableFolding);
	}
	//  Notifications Page
	if (control == fEnableNotifications || loadAll == true) {
		status = fWindowSettingsFile->FindInt32("enable_notifications", &intVal);
		fControlsCount += loadAll == true;
		if (status == B_OK) {
			fEnableNotifications->SetValue(intVal);
			fControlsDone += loadAll == true;
		} else
			fOrphansList->AddItem(fEnableNotifications);
	}
	//  Build Page
	if (control == fWrapConsoleEnabled || loadAll == true) {
		status = fWindowSettingsFile->FindInt32("wrap_console", &intVal);
		fControlsCount += loadAll == true;
		if (status == B_OK) {
			fWrapConsoleEnabled->SetValue(intVal);
			fControlsDone += loadAll == true;
		} else
			fOrphansList->AddItem(fWrapConsoleEnabled);
	}
	if (control == fConsoleBannerEnabled || loadAll == true) {
		status = fWindowSettingsFile->FindInt32("console_banner", &intVal);
		fControlsCount += loadAll == true;
		if (status == B_OK) {
			fConsoleBannerEnabled->SetValue(intVal);
			fControlsDone += loadAll == true;
		} else
			fOrphansList->AddItem(fConsoleBannerEnabled);
	}

	_UpdateText();
	// Note: gets rewritten on reload defaults
	_UpdateTrailing();

	return status;
}

void
SettingsWindow::_ManageModifications(BControl* control, bool isModified)
{
	// Item was modified
	if (isModified == true) {

		// Manage multiple modifications for the same control 
		if (!fModifiedList->HasItem(control))
			fModifiedList->AddItem(control);
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
			if (fModifiedList->IsEmpty()){
					// Probably reviewing a new version, let apply even if unmodified
					if (fOrphansList->CountItems() > 0)
						fApplyButton->SetEnabled(true);
					else
						fApplyButton->SetEnabled(false);
				fRevertButton->SetEnabled(false);
			}
		}
	}

	_UpdateText();
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

	fBuildBox = new BBox("BuildBox");
	fBuildBox->SetLabel(B_TRANSLATE("Build"));

	fWrapConsoleEnabled = new BCheckBox("WrapConsoleEnabled",
		B_TRANSLATE("Wrap console"), new BMessage(MSG_WRAP_CONSOLE_ENABLED));

	fConsoleBannerEnabled = new BCheckBox("ConsoleBannerEnabled",
		B_TRANSLATE("Console banner"), new BMessage(MSG_CONSOLE_BANNER_ENABLED));

	BView* view = BGroupLayoutBuilder(B_VERTICAL, 0)
		.Add(BLayoutBuilder::Grid<>(fBuildBox)
		.Add(fWrapConsoleEnabled, 0, 1)
		.Add(fConsoleBannerEnabled, 1, 1)
		.Add(new BSeparatorView(B_HORIZONTAL, B_PLAIN_BORDER), 0, 3, 4)
		.AddGlue(0, 4)
		.SetInsets(10, 20, 10, 10))
	.TopView();

	view->SetName("BuildPage");

	return view;

}

BView*
SettingsWindow::_PageEditorView()
{
	// "EditorPage" Box
	fEditorBox = new BBox("EditorBox");
	fEditorBox->SetLabel(B_TRANSLATE("Editor"));

	fFontMenuOP = new BOptionPopUp("FontMenu",
		B_TRANSLATE("Font:"), new BMessage(MSG_EDITOR_FONT_CHANGED));

//	fFontMenuOP->SetExplicitMaxSize(BSize(400, B_SIZE_UNSET));

	BFont font(be_fixed_font);
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

	for (int32 i = 10; i <= 18; i++) {
		BString text;
		text << i;
		fEditorFontSizeOP->AddOption(text.String(), i);
	}

	fSyntaxHighlight = new BCheckBox("SyntaxHighlight",
		B_TRANSLATE("Enable Syntax highlighting"), new BMessage(MSG_SYNTAX_HIGHLIGHT_TOGGLED));
	fBraceMatch = new BCheckBox("BraceMatch",
		B_TRANSLATE("Enable Brace matching"), new BMessage(MSG_BRACE_MATCHING_TOGGLED));
	fSaveCaret = new BCheckBox("SaveCaret",
		B_TRANSLATE("Save caret position"), new BMessage(MSG_SAVE_CARET_TOGGLED));
	fTabWidthSpinner = new BSpinner("TabWidth",
		B_TRANSLATE("Tab width:  "), new BMessage(MSG_TAB_WIDTH_CHANGED));
	fTabWidthSpinner->SetRange(1, 8);
	fTabWidthSpinner->SetAlignment(B_ALIGN_RIGHT);

	BView* view = BGroupLayoutBuilder(B_VERTICAL, 0)
		.Add(BLayoutBuilder::Grid<>(fEditorBox)
		.Add(fFontMenuOP, 0, 0, 2)
		.Add(fEditorFontSizeOP, 3, 0)
		.Add(fPreviewBox, 0, 1, 3)
		.Add(fSyntaxHighlight, 0, 2)
		.Add(fBraceMatch, 0, 3)
		.Add(fSaveCaret, 0, 4)
		.Add(fTabWidthSpinner, 0, 5, 1)
		.Add(new BSeparatorView(B_HORIZONTAL, B_PLAIN_BORDER), 0, 6, 6)
		.AddGlue(0, 7)
		.SetInsets(10, 20, 10, 10))
	.TopView();

	view->SetName("EditorPage");

	return view;
}

BView*
SettingsWindow::_PageEditorViewVisual()
{
	// "EditorVisualPage" Box
	fEditorVisualBox = new BBox("EditorVisualBox");
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

	BView* view = BGroupLayoutBuilder(B_VERTICAL, 0)
		.Add(BLayoutBuilder::Grid<>(fEditorVisualBox)
		.Add(fShowLineNumber, 0, 0)
		.Add(fEnableFolding, 1, 0)
		.Add(fShowCommentMargin, 2, 0)
		.Add(fMarkCaretLine, 0, 1)
		.Add(fShowEdgeLine, 0, 2)
		.Add(fEdgeLineColumn->CreateLabelLayoutItem(), 1, 2)
		.Add(fEdgeLineColumn->CreateTextViewLayoutItem(), 2, 2)
		.AddGlue(3, 2)
		.Add(new BSeparatorView(B_HORIZONTAL, B_PLAIN_BORDER), 0, 3, 4)
		.AddGlue(0, 4)
		.SetInsets(10, 20, 10, 10))
	.TopView();

	view->SetName("EditorVisualPage");

	return view;
}

BView*
SettingsWindow::_PageGeneralView()
{
	// "General" Box
	fGeneralBox = new BBox("GeneralBox");
	fGeneralBox->SetLabel(B_TRANSLATE("General"));

	// Text Control
	fProjectsDirectory = new BTextControl("ProjectsDirectory",
		B_TRANSLATE("Projects directory:"), "", nullptr);
	fProjectsDirectory->SetModificationMessage(new BMessage(MSG_PROJECT_DIRECTORY_EDITED));

	// Button disabled now
	fBrowseProjectsButton = new BButton(B_TRANSLATE("Browse" B_UTF8_ELLIPSIS), nullptr);
//		new BMessage(MSG_BUTTON_BROWSE_CLICKED));
	fBrowseProjectsButton->SetEnabled(false);

	// Check Box disabled TODO This comes from ui.settings, just show
	BCheckBox* saveWindowPosition = new BCheckBox("saveWindowPosition",
		B_TRANSLATE("Save position"), nullptr); //TODO var name
	saveWindowPosition->SetValue(B_CONTROL_ON);
	saveWindowPosition->SetEnabled(false);

	// Check Box
	fFullPathWindowTitle = new BCheckBox("FullPathWindowTitle",
		B_TRANSLATE("File full path in window title"), new BMessage(MSG_FULL_PATH_TOGGLED));

	BView* view = BGroupLayoutBuilder(B_VERTICAL, 0)
		.Add(BLayoutBuilder::Grid<>(fGeneralBox)
		.Add(fProjectsDirectory->CreateLabelLayoutItem(), 0, 0)
		.Add(fProjectsDirectory->CreateTextViewLayoutItem(), 1, 0, 2)
		.Add(fBrowseProjectsButton, 3, 0)
		.Add(saveWindowPosition, 0, 1)
		.Add(fFullPathWindowTitle, 1, 1)
		.Add(new BSeparatorView(B_HORIZONTAL, B_PLAIN_BORDER), 0, 3, 4)
		.AddGlue(0, 4)
		.SetInsets(10, 20, 10, 10))
	.TopView();

	view->SetName("GeneralPage");

	return view;
}

BView*
SettingsWindow::_PageGeneralViewStartup()
{
	// "Startup" Box
	fGeneralStartupBox = new BBox("GeneralStartupBox");
	fGeneralStartupBox->SetLabel(B_TRANSLATE("Startup"));

	fReopenProjects = new BCheckBox("ReopenProjects",
		B_TRANSLATE("Reload projects"), new BMessage(MSG_REOPEN_PROJECTS_TOGGLED));

	fReopenFiles = new BCheckBox("ReopenFiles",
		B_TRANSLATE("Reload files"), new BMessage(MSG_REOPEN_FILES_TOGGLED));

	fShowProjectsPanes = new BCheckBox("ShowProjectsPanes",
		B_TRANSLATE("Show Projects panes"), new BMessage(MSG_SHOW_PROJECTS_PANES_TOGGLED));

	fShowOutputPanes = new BCheckBox("ShowOutputPanes",
		B_TRANSLATE("Show Output panes"), new BMessage(MSG_SHOW_OUTPUT_PANES_TOGGLED));

	fShowToolBar = new BCheckBox("ShowToolBar",
		B_TRANSLATE("Show ToolBar"), new BMessage(MSG_SHOW_TOOLBAR_TOGGLED));

	BView* view = BGroupLayoutBuilder(B_VERTICAL, 0)
		.Add(BLayoutBuilder::Grid<>(fGeneralStartupBox)
		.Add(fReopenProjects, 0, 1)
		.Add(fReopenFiles, 1, 1)
		.Add(fShowProjectsPanes, 0, 2)		
		.Add(fShowOutputPanes, 1, 2)
		.Add(fShowToolBar, 2, 2)
		.Add(new BSeparatorView(B_HORIZONTAL, B_PLAIN_BORDER), 0, 3, 4)
		.AddGlue(0, 4)
		.SetInsets(10, 20, 10, 10))
	.TopView();

	view->SetName("GeneralStartupPage");

	return view;
}

BView*
SettingsWindow::_PageNotificationsView()
{
	// "" Box
	fNotificationsBox = new BBox("NotificationsBox");
	fNotificationsBox->SetLabel(B_TRANSLATE("Notifications"));

	fEnableNotifications = new BCheckBox("EnableNotifications",
		B_TRANSLATE("Enable Notifications"), new BMessage(MSG_ENABLE_NOTIFICATIONS_TOGGLED));

	BView* view = BGroupLayoutBuilder(B_VERTICAL, 0)
		.Add(BLayoutBuilder::Grid<>(fNotificationsBox)
		.Add(fEnableNotifications, 0, 1)
		.Add(new BSeparatorView(B_HORIZONTAL, B_PLAIN_BORDER), 0, 3, 4)
		.AddGlue(0, 4)
		.SetInsets(10, 20, 10, 10))
	.TopView();

	view->SetName("NotificationsPage");

	return view;
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

	// Probably reviewing a new version, let apply even if unmodified
	if (fOrphansList->CountItems() > 0)
		fApplyButton->SetEnabled(true);
	else
		fApplyButton->SetEnabled(false);

	fRevertButton->SetEnabled(false);

	_UpdateText();
}

void
SettingsWindow::_ShowOrphans()
{
	BString appVersion(GenioNames::GetVersionInfo());
	BString fileVersion(fWindowSettingsFile->FindString("app_version"));

	// If there is a new app version with no settings modifications
	// ReadyToRun will keep asking to review forever so update version
	if (GenioNames::CompareVersion(appVersion, fileVersion) > 0)
		fWindowSettingsFile->SetBString("app_version", appVersion);

	if (fOrphansList->CountItems() == 0)
		return;

	// New controls were detected, load default values and reload file
	delete fWindowSettingsFile;

	GenioNames::UpdateSettingsFile();

	fWindowSettingsFile = new TPreferences(GenioNames::kSettingsFileName,
											GenioNames::kApplicationName, 'IDSE');

	// Mark new controls with green
	for (int32 item = fOrphansList->CountItems() -1; item >= 0 ; item--) {
		_LoadFromFile(fOrphansList->ItemAt(item), false);		
		fOrphansList->ItemAt(item)->SetViewColor(kSettingsGreen);
		fOrphansList->ItemAt(item)->Invalidate();
	}

	// Let user apply
	fApplyButton->SetEnabled(true);
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
	status_t status = B_OK;

	// General Page
	if (control == fProjectsDirectory)
		status = fWindowSettingsFile->SetBString("projects_directory", fProjectsDirectory->Text());
	else if (control == fFullPathWindowTitle)
		status = fWindowSettingsFile->SetInt32("fullpath_title", fFullPathWindowTitle->Value());
	// General Startup Page
	else if (control == fReopenProjects)
		status = fWindowSettingsFile->SetInt32("reopen_projects", fReopenProjects->Value());
	else if (control == fReopenFiles)
		status = fWindowSettingsFile->SetInt32("reopen_files", fReopenFiles->Value());
	else if (control == fShowProjectsPanes)
		status = fWindowSettingsFile->SetInt32("show_projects", fShowProjectsPanes->Value());
	else if (control == fShowOutputPanes)
		status = fWindowSettingsFile->SetInt32("show_output", fShowOutputPanes->Value());
	else if (control == fShowToolBar)
		status = fWindowSettingsFile->SetInt32("show_toolbar", fShowToolBar->Value());
	// Editor Page
	else if (control == fEditorFontSizeOP)
		status = fWindowSettingsFile->SetInt32("edit_fontsize", fEditorFontSizeOP->Value());
	else if (control == fSyntaxHighlight)
		status = fWindowSettingsFile->SetInt32("syntax_highlight", fSyntaxHighlight->Value());
	else if (control == fTabWidthSpinner)
		status = fWindowSettingsFile->SetInt32("tab_width", fTabWidthSpinner->Value());
	else if (control == fBraceMatch)
		status = fWindowSettingsFile->SetInt32("brace_match", fBraceMatch->Value());
	else if (control == fSaveCaret)
		status = fWindowSettingsFile->SetInt32("save_caret", fSaveCaret->Value());
	// Editor Visual Page
	else if (control == fShowLineNumber)
		status = fWindowSettingsFile->SetInt32("show_linenumber", fShowLineNumber->Value());
	else if (control == fShowCommentMargin)
		status = fWindowSettingsFile->SetInt32("show_commentmargin", fShowCommentMargin->Value());
	else if (control == fMarkCaretLine)
		status = fWindowSettingsFile->SetInt32("mark_caretline", fMarkCaretLine->Value());
	else if (control == fShowEdgeLine)
		status = fWindowSettingsFile->SetInt32("show_edgeline", fShowEdgeLine->Value());
	else if (control == fEdgeLineColumn)
		status = fWindowSettingsFile->SetBString("edgeline_column", fEdgeLineColumn->Text());
	else if (control == fEnableFolding)
		status = fWindowSettingsFile->SetInt32("enable_folding", fEnableFolding->Value());
	// Notifications Page
	else if (control == fEnableNotifications)
		status = fWindowSettingsFile->SetInt32("enable_notifications", fEnableNotifications->Value());
	// Build Page
	else if (control == fWrapConsoleEnabled)
		status = fWindowSettingsFile->SetInt32("wrap_console", fWrapConsoleEnabled->Value());
	else if (control == fConsoleBannerEnabled)
		status = fWindowSettingsFile->SetInt32("console_banner", fConsoleBannerEnabled->Value());

	return status;
}

status_t
SettingsWindow::_StoreToFileDefaults()
{
	if (fWindowSettingsFile->InitCheck() != B_OK)
		return B_ERROR; // TODO notify

	// TODO check individual Setting
	// Reset counter
	fWindowSettingsFile->SetInt32("use_count", 0);

	fWindowSettingsFile->SetInt64("last_used", real_time_clock());
	// Reset app version
	fWindowSettingsFile->SetBString("app_version", GenioNames::GetVersionInfo());

	// General Page
	fWindowSettingsFile->SetBString("projects_directory", kSKProjectsDirectory);
	fWindowSettingsFile->SetInt32("fullpath_title", kSKFullPathTitle);

	// General Startup Page
	fWindowSettingsFile->SetInt32("reopen_projects", kSKReopenProjects);
	fWindowSettingsFile->SetInt32("reopen_files", kSKReopenFiles);
	fWindowSettingsFile->SetInt32("show_projects", kSKShowProjects);
	fWindowSettingsFile->SetInt32("show_output", kSKShowOutput);
	fWindowSettingsFile->SetInt32("show_toolbar", kSKShowToolBar);

	// Editor Page
	fWindowSettingsFile->SetInt32("edit_fontsize", kSKEditorFontSize);
	fWindowSettingsFile->SetInt32("syntax_highlight", kSKSyntaxHighlight);
	fWindowSettingsFile->SetInt32("tab_width", kSKTabWidth);
	fWindowSettingsFile->SetInt32("brace_match", kSKBraceMatch);
	fWindowSettingsFile->SetInt32("save_caret", kSKSaveCaret);

	// Editor Visual Page
	fWindowSettingsFile->SetInt32("show_linenumber", kSKShowLineNumber);
	fWindowSettingsFile->SetInt32("show_commentmargin", kSKShowCommentMargin);
	fWindowSettingsFile->SetInt32("mark_caretline", kSKMarkCaretLine);
	fWindowSettingsFile->SetInt32("show_edgeline", kSKShowEdgeLine);
	fWindowSettingsFile->SetBString("edgeline_column", kSKEdgeLineColumn);
	fWindowSettingsFile->SetInt32("enable_folding", kSKEnableFolding);

	// Notifications Page
	fWindowSettingsFile->SetInt32("enable_notifications", kSKEnableNotifications);

	// Build Page
	fWindowSettingsFile->SetInt32("wrap_console", kSKWrapConsole);
	fWindowSettingsFile->SetInt32("console_banner", kSKConsoleBanner);
	
	return B_OK;
}

void
SettingsWindow::_UpdateText()
{
	BString text;

	text << fControlsDone << "/" << fControlsCount;
	text <<  " " << B_TRANSLATE("Controls loaded.");
	text << " " << B_TRANSLATE("Modifications") << " " << fModifiedList->CountItems();
	text <<  ",\t" << B_TRANSLATE("applied") << " " << fAppliedModifications;
	fStatusBar->SetText(text.String());
}

void
SettingsWindow::_UpdateTrailing()
{
	BString text;

	text << B_TRANSLATE("Set for app ver: ")
		 << fWindowSettingsFile->FindString("app_version") << ",\t";

	BString option = fUseCount == 1	? B_TRANSLATE("time")
										: B_TRANSLATE("times");
	text << B_TRANSLATE("tuned ") << fUseCount << " " << option;

	fStatusBar->SetTrailingText(text.String());
}
