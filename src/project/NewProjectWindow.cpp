/*
 * Copyright 2017..2018 A. Mosca <amoscaster@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "NewProjectWindow.h"

#include <Alignment.h>
#include <AppFileInfo.h>
#include <Application.h>
#include <Architecture.h>
#include <Bitmap.h>
#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <DateTime.h>
#include <Directory.h>
#include <LayoutBuilder.h>
#include <ListView.h>
#include <Locale.h>
#include <MenuBar.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Mime.h>
#include <NodeInfo.h>
#include <OutlineListView.h>
#include <PopUpMenu.h>
#include <ScrollView.h>
#include <SeparatorView.h>
#include <SpaceLayoutItem.h>
#include <SplitView.h>
#include <TextControl.h>
#include <OptionPopUp.h>

#include "GenioNamespace.h"
#include "GenioCommon.h"
#include "TPreferences.h"

#include <iostream>
#include <fstream>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "NewProject Window"

ProjectTypeMap projectTypeMap;

enum
{
	MSG_ADD_FILE_NAME				= 'afna',
	MSG_ADD_HEADER_TOGGLED			= 'ahto',
	MSG_ADD_SECOND_FILE_NAME		= 'asfn',
	MSG_ADD_SECOND_HEADER_TOGGLED	= 'asht',
	MSG_BROWSE_HAIKU_APP_CLICKED	= 'bhac',
	MSG_BROWSE_LOCAL_APP_CLICKED	= 'blac',
	MSG_GIT_ENABLED					= 'gien',
	MSG_HAIKU_APP_EDITED			= 'haed',
	MSG_HAIKU_APP_REFS_RECEIVED		= 'harr',
	MSG_LOCAL_APP_EDITED			= 'laed',
	MSG_LOCAL_APP_REFS_RECEIVED		= 'larr',
	MSG_PROJECT_CANCEL				= 'prca',
	MSG_PROJECT_CREATE				= 'prcr',
	MSG_PROJECT_CHOSEN				= 'prch',
	MSG_PROJECT_NAME_EDITED			= 'pned',
	MSG_PROJECT_TARGET				= 'prta',
	MSG_PROJECT_TYPE_CHANGED		= 'ptch',
	MSG_PROJECT_DIRECTORY			= 'prdi',
	MSG_RUN_IN_TERMINAL				= 'rite'
};


NewProjectWindow::NewProjectWindow()
	:
	BWindow(BRect(0, 0, 799, 599), "New Project", B_TITLED_WINDOW, //B_MODAL_WINDOW,
													B_ASYNCHRONOUS_CONTROLS | 
													B_NOT_ZOOMABLE |
//													B_NOT_RESIZABLE |
													B_AVOID_FRONT |
													B_AUTO_UPDATE_SIZE_LIMITS |
													B_CLOSE_ON_ESCAPE)
{
	_InitWindow();
	// Map Items description
	_MapItems();

	CenterOnScreen();			
}

NewProjectWindow::~NewProjectWindow()
{

}

bool
NewProjectWindow::QuitRequested()
{
	delete fOpenPanel;

	delete haikuItem;
	delete appItem;
	delete appMenuItem;
	delete genericItem;
	delete helloCplusItem;
	delete helloCItem;
	delete principlesItem;
	delete emptyItem;
	delete importItem;
	delete sourcesItem;
	delete existingItem;
	delete rustItem;
	delete cargoItem;

	BWindow::Quit();
		
	return true;
}

void
NewProjectWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what) {

		case MSG_PROJECT_CANCEL:
			PostMessage(B_QUIT_REQUESTED);
			break;
		case MSG_PROJECT_CREATE:
		{
			_CreateProject();
			break;
		}
		case MSG_PROJECT_CHOSEN:
		{
			_UpdateControlsState(fTypeListView->CurrentSelection());
			break;	
		}
		case MSG_PROJECT_NAME_EDITED:
		{
			_UpdateControlsData(fTypeListView->CurrentSelection());
			break;	
		}
		case MSG_HAIKU_APP_EDITED:
		{
			_OnEditingHaikuAppText();
			break;
		}
		case MSG_BROWSE_HAIKU_APP_CLICKED:
		{
			fOpenPanel->SetMessage(new BMessage(MSG_HAIKU_APP_REFS_RECEIVED));
			fOpenPanel->Show();
			break;
		}
		case MSG_HAIKU_APP_REFS_RECEIVED:
		{
			entry_ref ref;
			if ((msg->FindRef("refs", &ref)) == B_OK) {
				BPath path(&ref);
				fHaikuAppDirText->SetText(path.Path());
			}
			break;
		}
		case MSG_LOCAL_APP_EDITED:
		{
			_OnEditingLocalAppText();
			break;
		}
		case MSG_BROWSE_LOCAL_APP_CLICKED:
		{
			fOpenPanel->SetMessage(new BMessage(MSG_LOCAL_APP_REFS_RECEIVED));
			fOpenPanel->Show();
			break;
		}
		case MSG_LOCAL_APP_REFS_RECEIVED:
		{
			entry_ref ref;
			if ((msg->FindRef("refs", &ref)) == B_OK) {
				BPath path(&ref);
				fLocalAppDirText->SetText(path.Path());
			}
			break;
		}

		default:
		{
			BWindow::MessageReceived(msg);
			break;
		}
	}	
}

/**
 * When you click 'Create' button you are driven here.
 * After some validation the _Create... function for the chosen project is called.
 * Some projects may use common functions (e.g. _CreateSkeleton) 
 * while others are "stand alone" (e.g. _CreateCargoProject)
 */
status_t
NewProjectWindow::_CreateProject()
{
	status_t status;
	int32 selection = fTypeListView->CurrentSelection();

	fCurrentItem = dynamic_cast<BStringItem*>(fTypeListView->ItemAt(selection));

	// Little validation for controls

	// Warn if project name is empty
	if ((strcmp(fProjectNameText->Text(), "") == 0)) {
		fProjectDescription->SetText(B_TRANSLATE("Please fill \"Project name\" field"));
		return B_ERROR;
	}

	// Trim spaces (to avoid messing class names)
	// Thanx to Sofia for bug discovering
	BString projectName = fProjectNameText->Text();
	fProjectNameText->SetText(projectName.Trim());
	
	// Warn if project directory exists
	BPath dirPath(fProjectsDirectoryText->Text());
	dirPath.Append(fProjectNameText->Text());
	BEntry entry(dirPath.Path());
	if (entry.Exists() && fCurrentItem != existingItem) {
		fProjectDescription->SetText(B_TRANSLATE("ERROR: Project directory exists!"));
		return B_ERROR;
	}

	// Warn if project exists
	BPath projectPath;
	BString projectFile(fProjectNameText->Text());
	projectFile.Append(GenioNames::kProjectExtension); // ".idmpro"

	status = find_directory(B_USER_SETTINGS_DIRECTORY, &projectPath);
	projectPath.Append(GenioNames::kApplicationName);
	projectPath.Append(projectFile);
	entry.SetTo(projectPath.Path());
	if (entry.Exists()) {
		BString warn;
		warn << B_TRANSLATE("ERROR: Project ") << projectFile
			<< " " << B_TRANSLATE("exists!");
		fProjectDescription->SetText(warn.String());
		return B_ERROR;
	}

	// Warn if project target is empty if enabled
	if ((!strcmp(fProjectTargetText->Text(), "") && fProjectTargetText->IsEnabled())) {
		fProjectDescription->SetText(B_TRANSLATE("Please fill \"Project target\" field"));
		return B_ERROR;
	}

	// Warn if add file is empty if enabled
	if ((!strcmp(fAddFileText->Text(), "") && fAddFileText->IsEnabled())) {
		fProjectDescription->SetText(B_TRANSLATE("Please fill \"Add file\" field"));
		return B_ERROR;
	}

	// Create projects directory if not existing
	BPath projectsPath(fProjectsDirectoryText->Text());
	BEntry projectsEntry(projectsPath.Path());
	if (!projectsEntry.Exists()) {
		// TODO: Set mask somewhere
		status = create_directory(projectsPath.Path(), 0755);
		if (status != B_OK)
			return status;
	}

	if (fCurrentItem == appItem)
		status = _CreateAppProject();
	else if (fCurrentItem == appMenuItem) {
		// App with menu item, warn on empty add second file
		if (!strcmp(fAddSecondFileText->Text(), "")) {
			fProjectDescription->SetText(B_TRANSLATE("Please fill \"Add file\" field"));
			return B_ERROR;
		}
		status = _CreateAppMenuProject();
	}
	else if (fCurrentItem == helloCplusItem)
		status = _CreateHelloCplusProject();
	else if (fCurrentItem == helloCItem)
		status = _CreateHelloCProject();
	else if (fCurrentItem == principlesItem)
		status = _CreatePrinciplesProject();
	else if (fCurrentItem == emptyItem)
		status = _CreateEmptyProject();
	else if (fCurrentItem == sourcesItem) {
		// Haiku sources item, warn on empty app dir
		if (strcmp(fHaikuAppDirText->Text(), "") == 0) {
			fProjectDescription->SetText(B_TRANSLATE("Please fill \"Haiku app dir\" field"));
			return B_ERROR;
		}
		status = _CreateHaikuSourcesProject();
	}
	else if (fCurrentItem == existingItem) {
		// Local sources item, warn on empty app dir
		if (strcmp(fLocalAppDirText->Text(), "") == 0) {
			fProjectDescription->SetText(B_TRANSLATE("Please fill \"Local app dir\" field"));
			return B_ERROR;
		}
		status = _CreateLocalSourcesProject();
	}
	else if (fCurrentItem == cargoItem)
		status = _CreateCargoProject();

	if (status == B_OK) {
		// If git enabled, init
		if (fGitEnabled->Value() == true) {
			chdir(f_project_directory);
			system("git init");
		}
		// Post a message
		BMessage message(GenioNames::NEWPROJECTWINDOW_PROJECT_OPEN_NEW);
		message.AddString("project_extensioned_name", fProjectExtensionedName);
		be_app->WindowAt(0)->PostMessage(&message);
		// Quit
		PostMessage(B_QUIT_REQUESTED);
	} else {
		// Delete stale project files if any
		BString description;
		description << "\n\n"
			<< B_TRANSLATE("ERROR: Project creation failed")
			<< "\n\n";
		fProjectDescription->Insert(description);

		_RemoveStaleEntries(dirPath.Path());

		entry.SetTo(projectPath.Path());
		if (entry.Exists()) {
			entry.Remove();
			BString text;
			text << "\n" << B_TRANSLATE("Removing stale project file:")
				<< " " << entry.Name() << "\n\n";
			fProjectDescription->Insert(text);
		}
	}

	return status;
}

status_t
NewProjectWindow::_CreateSkeleton()
{
	status_t status;

	BPath path(fProjectsDirectoryText->Text());
	path.Append(fProjectNameText->Text());
	BDirectory projectDirectory;

	status = projectDirectory.CreateDirectory(path.Path(), nullptr);
	f_project_directory = path.Path();
	if (status != B_OK)
		return status;

	// Create "src" and "app" directories
	BPath srcPath(path.Path());
	srcPath.Append("src");
	BDirectory srcDirectory;
	status = srcDirectory.CreateDirectory(srcPath.Path(), nullptr);
	if (status != B_OK)
		return status;

	BPath appPath(path.Path());
	appPath.Append("app");
	BDirectory appDirectory;
	status = appDirectory.CreateDirectory(appPath.Path(), nullptr);
	if (status != B_OK)
		return status;

	// Project file
	fProjectExtensionedName = fProjectNameText->Text();
	fProjectExtensionedName.Append(GenioNames::kProjectExtension); // ".idmpro"
	fProjectFile =  new TPreferences(fProjectExtensionedName, GenioNames::kApplicationName, 'LOPR');

	fProjectFile->SetBString("project_extensioned_name", fProjectExtensionedName);
	fProjectFile->SetBString("project_name", fProjectNameText->Text());
	fProjectFile->SetBString("project_directory", path.Path());
	fProjectFile->SetBString("project_build_command", "make");
	fProjectFile->SetBString("project_clean_command", "make clean rmapp");
	fProjectFile->SetBool("run_in_terminal", fRunInTeminal->Value());
	fProjectFile->SetBool("release_mode", false);

	if (fGitEnabled->Value() == true)
		fProjectFile->SetBString("project_scm", "git");

	// Set project target
	BString target(appPath.Path());
	target << "/" << fProjectTargetText->Text();
	fProjectFile->SetBString("project_target", target);

	return B_OK;
}

status_t
NewProjectWindow::_CreateAppProject()
{
	status_t status;

	if ((status = _CreateSkeleton()) != B_OK)
		return status;

	fProjectFile->SetBString("project_type", "c++");

	if ((status = _WriteMakefile()) != B_OK)
		return status;

	if ((status = _WriteAppfiles()) != B_OK)
		return status;

	delete fProjectFile;

	return B_OK;
}

status_t
NewProjectWindow::_CreateAppMenuProject()
{
	status_t status;

	if ((status = _CreateSkeleton()) != B_OK)
		return status;

	fProjectFile->SetBString("project_build_command",
		"make && make bindcatalogs");
	fProjectFile->SetBString("project_type", "c++");

	if ((status = _WriteMakefile()) != B_OK)
		return status;

	if ((status = _WriteAppMenufiles()) != B_OK)
		return status;

	delete fProjectFile;

	return B_OK;
}

status_t
NewProjectWindow::_CreateCargoProject()
{
	BPath path(fProjectsDirectoryText->Text());

	// Test cargo presence
	BEntry entry(fCargoPathText->Text(), true);
	if (!entry.Exists()) {
		fProjectDescription->SetText(B_TRANSLATE("cargo binary not found!"));
		return B_ERROR;
	}

	BString args, srcFilename("src/lib.rs");
	args << fProjectNameText->Text();
	if (fCargoBinEnabled->Value() == B_CONTROL_ON) {
		args << " --bin";
		srcFilename = "src/main.rs";
	}
	if (fCargoVcsEnabled->Value() == B_CONTROL_ON)
		args << " --vcs none";

	// Post a message
	BMessage message(GenioNames::NEWPROJECTWINDOW_PROJECT_CARGO_NEW);
	message.AddString("cargo_new_string", args);
	be_app->WindowAt(0)->PostMessage(&message);

	// Project file
	fProjectExtensionedName = fProjectNameText->Text();
	fProjectExtensionedName.Append(GenioNames::kProjectExtension); // ".idmpro"
	TPreferences projectFile(fProjectExtensionedName, GenioNames::kApplicationName, 'LOPR');

	projectFile.SetBString("project_extensioned_name", fProjectExtensionedName);
	projectFile.SetBString("project_name", fProjectNameText->Text());
	path.Append(fProjectNameText->Text());
	projectFile.SetBString("project_directory", path.Path());
	// Set project target: cargo is able to build & run in one pass, setting
	// target to project directory will enable doing the same in
	// GenioWindow::_RunTarget()
	projectFile.SetBString("project_target", path.Path());
	// Mind gcc2 appendind
	BString build_command;
	build_command << f_cargo_binary << " build";
	BString clean_command;
	clean_command << f_cargo_binary << " clean";

	projectFile.SetBString("project_build_command", build_command);
	projectFile.SetBString("project_clean_command", clean_command);
	projectFile.SetBool("run_in_terminal", fRunInTeminal->Value());
	projectFile.SetBool("release_mode", false);

	// Cargo specific
	projectFile.SetBString("project_type", "cargo");

	BPath srcPath(path);
	path.Append("Cargo.toml");
	projectFile.AddString("project_file", path.Path());
	srcPath.Append(srcFilename);
	projectFile.AddString("project_source", srcPath.Path());

	return B_OK;
}

status_t
NewProjectWindow::_CreateHelloCplusProject()
{
	status_t status;

	if ((status = _CreateSkeleton()) != B_OK)
		return status;

	fProjectFile->SetBString("project_type", "c++");

	if ((status = _WriteMakefile()) != B_OK)
		return status;

	if ((status = _WriteHelloCplusfile()) != B_OK)
		return status;

	delete fProjectFile;

	return B_OK;
}

status_t
NewProjectWindow::_CreateHelloCProject()
{
	status_t status;

	if ((status = _CreateSkeleton()) != B_OK)
		return status;

	if ((status = _WriteHelloCMakefile()) != B_OK)
		return status;

	if ((status = _WriteHelloCfile()) != B_OK)
		return status;

	delete fProjectFile;

	return B_OK;
}

status_t
NewProjectWindow::_CreatePrinciplesProject()
{
	status_t status;

	if ((status = _CreateSkeleton()) != B_OK)
		return status;

	fProjectFile->SetBString("project_type", "c++");

	if ((status = _WriteMakefile()) != B_OK)
		return status;

	if ((status = _WritePrinciplesfile()) != B_OK)
		return status;

	delete fProjectFile;

		return B_OK;
}

status_t
NewProjectWindow::_CreateEmptyProject()
{
	status_t status;

	BPath path(fProjectsDirectoryText->Text());
	path.Append(fProjectNameText->Text());
	BDirectory projectDirectory;

	// TODO manage existing
	status = projectDirectory.CreateDirectory(path.Path(), NULL);
	f_project_directory = path.Path();
	if (status != B_OK)
		return status;

	// Project file
	fProjectExtensionedName = fProjectNameText->Text();
	fProjectExtensionedName.Append(GenioNames::kProjectExtension); // ".idmpro"
	fProjectFile =  new TPreferences(fProjectExtensionedName, GenioNames::kApplicationName, 'LOPR');

	fProjectFile->SetBString("project_extensioned_name", fProjectExtensionedName);
	fProjectFile->SetBString("project_name", fProjectNameText->Text());
	fProjectFile->SetBString("project_directory", path.Path());
	fProjectFile->SetBool("run_in_terminal", fRunInTeminal->Value());
	fProjectFile->SetBool("release_mode", false);

	if (fGitEnabled->Value() == true)
		fProjectFile->SetBString("project_scm", "git");

	delete fProjectFile;

	return B_OK;
}

status_t
NewProjectWindow::_CreateHaikuSourcesProject()
{
	status_t status    = B_OK;

	// Project file
	fProjectExtensionedName = fProjectNameText->Text();
	fProjectExtensionedName.Append(GenioNames::kProjectExtension); // ".idmpro"
	fProjectFile =  new TPreferences(fProjectExtensionedName, GenioNames::kApplicationName, 'LOPR');

	fProjectFile->SetBString("project_extensioned_name", fProjectExtensionedName);
	fProjectFile->SetBString("project_name", fProjectNameText->Text());
	fProjectFile->SetBString("project_target", fProjectTargetText->Text());
	fProjectFile->SetBString("project_directory", fHaikuAppDirText->Text());
	fProjectFile->SetBString("project_build_command", "jam -q");
	fProjectFile->SetBString("project_clean_command", "jam clean");
	fProjectFile->SetBool("run_in_terminal", fRunInTeminal->Value());
	fProjectFile->SetBool("release_mode", false);
	fProjectFile->SetBString("project_type", "haiku_source");

	// Scan dir for files
	ProjectParser parser(fProjectFile);
	parser.ParseProjectFiles(fHaikuAppDirText->Text());

	delete fProjectFile;

	return status;
}

status_t
NewProjectWindow::_CreateLocalSourcesProject()
{
	status_t status    = B_OK;

	// Project file
	fProjectExtensionedName = fProjectNameText->Text();
	fProjectExtensionedName.Append(GenioNames::kProjectExtension); // ".idmpro"
	fProjectFile = new TPreferences(fProjectExtensionedName, GenioNames::kApplicationName, 'LOPR');

	fProjectFile->SetBString("project_extensioned_name", fProjectExtensionedName);
	fProjectFile->SetBString("project_name", fProjectNameText->Text());
	fProjectFile->SetBString("project_target", fProjectTargetText->Text());
	fProjectFile->SetBString("project_directory", fLocalAppDirText->Text());
	fProjectFile->SetBString("project_build_command", "make");
	fProjectFile->SetBString("project_clean_command", "make clean");
	fProjectFile->SetBool("run_in_terminal", fRunInTeminal->Value());
	fProjectFile->SetBool("release_mode", false);

	// Scan dir for files
	ProjectParser parser(fProjectFile);
	parser.ParseProjectFiles(fLocalAppDirText->Text());

	delete fProjectFile;

	return status;
}

void
NewProjectWindow::_InitWindow()
{
	// Project types OutlineListView
	fTypeListView = new BOutlineListView("typeview", B_SINGLE_SELECTION_LIST);
	fTypeListView->SetSelectionMessage(new BMessage(MSG_PROJECT_CHOSEN));

	typeListScrollView = new BScrollView("typescrollview",
		fTypeListView, B_FRAME_EVENTS | B_WILL_DRAW, false, true, B_FANCY_BORDER);

	// Items
	haikuItem = new TitleItem("Haiku");
	haikuItem->SetEnabled(false);
	haikuItem->SetExpanded(false);
	fTypeListView->AddItem(haikuItem);

	appItem = new BStringItem(B_TRANSLATE("Application"), 1, true);
	appMenuItem = new BStringItem(B_TRANSLATE("Application with menu"), 1, true);
/*
	appLayoutItem = new BStringItem(B_TRANSLATE("Application with layout"), 1, true);
	sharedItem = new BStringItem(B_TRANSLATE("Shared Library"), 1, true);
	staticItem = new BStringItem(B_TRANSLATE("Static Library"), 1, true);
	driverItem = new BStringItem(B_TRANSLATE("Driver"), 1, true);
	trackerItem = new BStringItem(B_TRANSLATE("Tracker add-on"), 1, true);
*/
	fTypeListView->AddItem(appItem);
	fTypeListView->AddItem(appMenuItem);
/*
	fTypeListView->AddItem(appLayoutItem);
	fTypeListView->AddItem(sharedItem);
	fTypeListView->AddItem(staticItem);
	fTypeListView->AddItem(driverItem);
	fTypeListView->AddItem(trackerItem);
*/
	genericItem = new TitleItem("Generic");
	genericItem->SetEnabled(false);
	genericItem->SetExpanded(false);
	fTypeListView->AddItem(genericItem);
	helloCplusItem = new BStringItem(B_TRANSLATE("C++ Hello World!"), 1, true);
	helloCItem = new BStringItem(B_TRANSLATE("C Hello World!"), 1, true);
	principlesItem = new BStringItem(B_TRANSLATE("Principles and Practice (2nd)"), 1, true);
	emptyItem = new BStringItem(B_TRANSLATE("Empty Project"), 1, true);
	fTypeListView->AddItem(helloCplusItem);
	fTypeListView->AddItem(helloCItem);
	fTypeListView->AddItem(principlesItem);
	fTypeListView->AddItem(emptyItem);

	importItem = new TitleItem(B_TRANSLATE("Import"));
	importItem->SetEnabled(false);
	importItem->SetExpanded(false);
	fTypeListView->AddItem(importItem);
	sourcesItem = new BStringItem(B_TRANSLATE("App from Haiku sources"), 1, true);
	existingItem = new BStringItem(B_TRANSLATE("C/C++ Project with Makefile"), 1, true);
	fTypeListView->AddItem(sourcesItem);
	fTypeListView->AddItem(existingItem);

	rustItem = new TitleItem(B_TRANSLATE("Rust"));
	rustItem->SetEnabled(false);
	rustItem->SetExpanded(false);
	cargoItem = new BStringItem(B_TRANSLATE("Cargo project"), 1, true);
	fTypeListView->AddItem(rustItem);
	fTypeListView->AddItem(cargoItem);

	// Project Description TextView
	fProjectDescription = new BTextView("projecttext");
	fProjectDescription->SetInsets(4.0f, 4.0f, 4.0f, 4.0f);
	fProjectDescription->MakeEditable(false);

	fScrollText = new BScrollView("scrolltext",
		fProjectDescription, B_WILL_DRAW | B_FRAME_EVENTS, false,
		true, B_FANCY_BORDER);

	// "Project" Box
	f_primary_architecture << get_primary_architecture();

	// cargo hack
	f_cargo_binary.SetTo("cargo");
	if (f_primary_architecture == "x86_gcc2")
		f_cargo_binary.SetTo("cargo-x86");


	fProjectBox = new BBox("projectBox");
	BString boxLabel = B_TRANSLATE("Project [host architecture: ");
	boxLabel << f_primary_architecture << "]";
	fProjectBox->SetLabel(boxLabel);

	fProjectNameText = new BTextControl("ProjectNameText",
		B_TRANSLATE("Project name:"), "", nullptr);
	fProjectNameText->SetModificationMessage(new BMessage(MSG_PROJECT_NAME_EDITED));
	fProjectNameText->SetEnabled(false);

	fGitEnabled = new BCheckBox("GitEnabled",
		B_TRANSLATE("Enable Git"), new BMessage(MSG_GIT_ENABLED));
	fGitEnabled->SetEnabled(false);
	fGitEnabled->SetValue(B_CONTROL_OFF);

	fProjectTargetText = new BTextControl("ProjectTargetText",
		B_TRANSLATE("Project target:"), "", new BMessage(MSG_PROJECT_TARGET));
	fProjectTargetText->SetEnabled(false);

	fRunInTeminal = new BCheckBox("RunInTeminal",
		B_TRANSLATE("Run in Terminal"), new BMessage(MSG_RUN_IN_TERMINAL));
	fRunInTeminal->SetEnabled(false);
	fRunInTeminal->SetValue(B_CONTROL_OFF);

	fProjectsDirectoryText = new BTextControl("ProjectsDirectoryText",
		B_TRANSLATE("Projects dir:"), "", new BMessage(MSG_PROJECT_DIRECTORY));
	fProjectsDirectoryText->SetEnabled(false);

	// Peep settings
	TPreferences* prefs = new TPreferences(GenioNames::kSettingsFileName,
								GenioNames::kApplicationName, 'IDSE');
	fProjectsDirectoryText->SetText(prefs->GetString("projects_directory"));
	delete prefs;

	fAddFileText = new BTextControl("AddFileText", B_TRANSLATE("Add file:"), "",
			new BMessage(MSG_ADD_FILE_NAME));
	fAddFileText->SetEnabled(false);

	fAddHeader = new BCheckBox("AddHeader",
		B_TRANSLATE("Add header"), new BMessage(MSG_ADD_HEADER_TOGGLED));
	fAddHeader->SetEnabled(false);
	fAddHeader->SetValue(B_CONTROL_OFF);

	fAddSecondFileText = new BTextControl("AddSecondFileText",
		B_TRANSLATE("Add file:"), "", new BMessage(MSG_ADD_SECOND_FILE_NAME));
	fAddSecondFileText->SetEnabled(false);


	fAddSecondHeader = new BCheckBox("AddSecondHeader",
		B_TRANSLATE("Add header"), new BMessage(MSG_ADD_SECOND_HEADER_TOGGLED));
	fAddSecondHeader->SetEnabled(false);
	fAddSecondHeader->SetValue(B_CONTROL_OFF);

//	BOptionPopUp* fProjectTypeOPU = new BOptionPopUp("ProjectTypeOPU",
//		B_TRANSLATE("Project type:"), new BMessage(MSG_PROJECT_TYPE_CHANGED));

	// Haiku sources app
	fHaikuAppDirText = new BTextControl("HaikuAppDirText",
		B_TRANSLATE("Haiku app dir:"), "", nullptr);
	fHaikuAppDirText->SetModificationMessage(new BMessage(MSG_HAIKU_APP_EDITED));
	fHaikuAppDirText->SetEnabled(false);

	fBrowseHaikuAppButton = new BButton(B_TRANSLATE("Browse" B_UTF8_ELLIPSIS),
								new BMessage(MSG_BROWSE_HAIKU_APP_CLICKED));
	fBrowseHaikuAppButton->SetExplicitMaxSize(fHaikuAppDirText->MinSize());
	fBrowseHaikuAppButton->SetEnabled(false);


	// Local sources app
	fLocalAppDirText = new BTextControl("LocalAppDirText",
		B_TRANSLATE("Local app dir:"), "", nullptr);
	fLocalAppDirText->SetModificationMessage(new BMessage(MSG_LOCAL_APP_EDITED));
	fLocalAppDirText->SetEnabled(false);

	fBrowseLocalAppButton = new BButton(B_TRANSLATE("Browse" B_UTF8_ELLIPSIS),
								new BMessage(MSG_BROWSE_LOCAL_APP_CLICKED));
	fBrowseLocalAppButton->SetExplicitMaxSize(fLocalAppDirText->MinSize());
	fBrowseLocalAppButton->SetEnabled(false);

	// Cargo app
	fCargoPathText = new BTextControl("CargoPathText",
		B_TRANSLATE("cargo path:"), "", nullptr);
	BString cargoPath;
	cargoPath << "/bin/" << f_cargo_binary;
	fCargoPathText->SetText(cargoPath);
	fCargoPathText->SetEnabled(false);

	fCargoBinEnabled = new BCheckBox("CargoBin", "--bin", nullptr);
	fCargoBinEnabled->SetEnabled(false);
	fCargoBinEnabled->SetValue(B_CONTROL_ON);
	fCargoBinEnabled->SetToolTip(B_TRANSLATE("Library target when unchecked"));

	fCargoVcsEnabled = new BCheckBox("CargoVcs", "--vcs none", nullptr);
	fCargoVcsEnabled->SetEnabled(false);
	fCargoVcsEnabled->SetValue(B_CONTROL_OFF);
	fCargoVcsEnabled->SetToolTip(B_TRANSLATE("Disables vcs management when checked"));

	// Buttons
	fExitButton = new BButton("ExitButton",
		B_TRANSLATE("Exit"), new BMessage(MSG_PROJECT_CANCEL));

	fCreateButton = new BButton("create", B_TRANSLATE("Create"),
		new BMessage(MSG_PROJECT_CREATE) );
	fCreateButton->SetEnabled(false);

	// Open panel
	// TODO read settings file for sources dir
	fOpenPanel = new BFilePanel(B_OPEN_PANEL, new BMessenger(this), NULL,
							B_DIRECTORY_NODE, false, NULL);

	BLayoutBuilder::Grid<>(fProjectBox)
.SetInsets(10.0f, 24.0f, 10.0f, 10.0f)
		.Add(fProjectNameText->CreateLabelLayoutItem(), 0, 1)
		.Add(fProjectNameText->CreateTextViewLayoutItem(), 1, 1, 2)
		.Add(fGitEnabled, 3, 1)
		.Add(fProjectTargetText->CreateLabelLayoutItem(), 0, 2)
		.Add(fProjectTargetText->CreateTextViewLayoutItem(), 1, 2, 2)
		.Add(fRunInTeminal, 3, 2)
		.Add(fProjectsDirectoryText->CreateLabelLayoutItem(), 0, 3)
		.Add(fProjectsDirectoryText->CreateTextViewLayoutItem(), 1, 3, 2)
		.Add(fAddFileText->CreateLabelLayoutItem(), 0, 4)
		.Add(fAddFileText->CreateTextViewLayoutItem(), 1, 4, 2)
		.Add(fAddHeader, 3, 4)
		.Add(fAddSecondFileText->CreateLabelLayoutItem(), 0, 5)
		.Add(fAddSecondFileText->CreateTextViewLayoutItem(), 1, 5, 2)
		.Add(fAddSecondHeader, 3, 5)
//		.Add(fProjectTypeOPU, 0, 6, 2)
		.Add(new BSeparatorView(B_HORIZONTAL), 0, 6, 4)
		.Add(fHaikuAppDirText->CreateLabelLayoutItem(), 0, 7)
		.Add(fHaikuAppDirText->CreateTextViewLayoutItem(), 1, 7, 2)
		.Add(fBrowseHaikuAppButton, 3, 7)
		.Add(fLocalAppDirText->CreateLabelLayoutItem(), 0, 8)
		.Add(fLocalAppDirText->CreateTextViewLayoutItem(), 1, 8, 2)
		.Add(fBrowseLocalAppButton, 3, 8)
		.Add(new BSeparatorView(B_HORIZONTAL), 0, 9, 4)
		.Add(fCargoPathText->CreateLabelLayoutItem(), 0, 10)
		.Add(fCargoPathText->CreateTextViewLayoutItem(), 1, 10)
		.Add(fCargoBinEnabled, 2, 10)
		.Add(fCargoVcsEnabled, 3, 10)
		.Add(new BSeparatorView(B_HORIZONTAL), 0, 11, 4)
		.AddGlue(0, 12)
		;

	// Window layout
	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
//		.SetInsets(2.0f)
		.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING, 5.0f)
			.Add(typeListScrollView, 2.0f)
			.Add(fProjectBox, 5.0f)
		.End()
		.AddGroup(B_VERTICAL, B_USE_DEFAULT_SPACING, 2.0f)
			.Add(fScrollText)
			.AddGroup(B_HORIZONTAL)
				.AddGlue()
				.Add(fExitButton)
				.AddGlue(4.0f)
				.Add(fCreateButton)
				.AddGlue()
			.End()
		.End()
	;
}

void
NewProjectWindow::_MapItems()
{
	BString appItemString;
	BString appMenuItemString;
	BString helloCplusItemString;
	BString helloCItemString;
	BString principlesItemString;
	BString emptyItemString;
	BString sourcesItemString;
	BString existingItemString;
	BString cargoItemString;

	appItemString
		<< B_TRANSLATE("A simple GUI \"Hello World!\" application");

	appMenuItemString
		<< B_TRANSLATE("A GUI application with menu bar and localization");

	helloCplusItemString
		<< B_TRANSLATE("A C++ command line \"Hello World!\" application");

	helloCItemString
		<< B_TRANSLATE("A C command line \"Hello World!\" application");

	principlesItemString
		<< B_TRANSLATE("An application template for programs")
		<< " "
		<< B_TRANSLATE("contained in Bjarne Stroustrup' s book:")
		<< "\n\n"
		<< B_TRANSLATE("Programming: Principles and Practice using C++ (2nd edition)")
		<< "\n\n"
		<< B_TRANSLATE("Put \"std_lib_facilities.h\" in:")
		<< "\n"
		<< "\t/boot/home/config/non-packaged/develop/headers"
		<< "\n"
		<< B_TRANSLATE("or edit Makefile variable (SYSTEM_INCLUDE_PATHS) accordingly");

	emptyItemString
		<< B_TRANSLATE("A project file with an empty project directory.")
		<< "\n\n"
		<< B_TRANSLATE("It may be filled in Project->Settings menu");

	sourcesItemString
		<< B_TRANSLATE("An application from Haiku sources repo.")
		<< "\n"
		<< B_TRANSLATE("Steps (read Haiku guides if unsure):")
		<< "\n\n"
		<< B_TRANSLATE("* Clone Haiku repo and configure it.")
		<< "\n"
		<< "\t"
		<< B_TRANSLATE("repodir>  git clone https://git.haiku-os.org/haiku")
		<< "\n"
		<< "\t"
		<< B_TRANSLATE("repodir/haiku> ./configure --use-gcc-pipe")
		<< "\n\n"
//		") Issue a build to save time.\n"
//		" e.g. repodir/haiku>  jam -q -j2 haiku-image\n\n" // @nightly-raw
		<< B_TRANSLATE("Once done get back here and:")
		<< "\n\n"
		<< B_TRANSLATE("* Select your Haiku app dir (from src/apps) or write full path in text control")
		<< "\n"
		<< "\t"
		<< B_TRANSLATE("e.g. /boot/home/repodir/haiku/src/apps/clock")
		<< "\n\n"
		<< B_TRANSLATE("* Accord \"generated\" target dir (in \"Project target:\") if needed")
		<< "\n\n"
		<< B_TRANSLATE("* Click \"Create\" button")
		<< "\n\n"
		<< B_TRANSLATE("NOTE: first compilation may take a long time to complete")
		<< "\n"
		<< B_TRANSLATE("NOTE: sources not copied or moved");

	existingItemString
		<< B_TRANSLATE("Existing project with Makefile.")
		<< "\n\n"
		<< B_TRANSLATE("Select project directory and accord \"Project target:\" if needed")
		<< "\n\n"
//		<< B_TRANSLATE("NOTE: clean sources before importing")
//		<< "\n"
		<< B_TRANSLATE("NOTE: sources not copied or moved");

	cargoItemString
		<< B_TRANSLATE("A rust cargo project.")
		<< "\n\n"
		<< B_TRANSLATE("You should make sure that the rust package (read cargo binary) is installed and working!");

	projectTypeMap.insert(ProjectTypePair(appItem, appItemString));
	projectTypeMap.insert(ProjectTypePair(appMenuItem, appMenuItemString));
	projectTypeMap.insert(ProjectTypePair(helloCplusItem, helloCplusItemString));
	projectTypeMap.insert(ProjectTypePair(helloCItem, helloCItemString));
	projectTypeMap.insert(ProjectTypePair(principlesItem, principlesItemString));
	projectTypeMap.insert(ProjectTypePair(emptyItem, emptyItemString));
	projectTypeMap.insert(ProjectTypePair(sourcesItem, sourcesItemString));
	projectTypeMap.insert(ProjectTypePair(existingItem, existingItemString));
	projectTypeMap.insert(ProjectTypePair(cargoItem, cargoItemString));
};

void
NewProjectWindow::_OnEditingHaikuAppText()
{
	BString appdir(fHaikuAppDirText->Text());

	if (appdir == "")
		return;

	BString generated;
	generated << "/generated/objects/haiku/"
		<< f_primary_architecture
		<< "/release/";
	appdir.Replace("/src/", generated, 1, appdir.FindLast("/src/"));

	BString appname;
	appdir.CopyInto(appname, appdir.FindLast('/') + 1, appdir.Length());
	appname.Capitalize();

	BString target;
	target << appdir << "/" << appname;

	fProjectNameText->SetText(appname);
	fProjectTargetText->SetText(target);
}

void
NewProjectWindow::_OnEditingLocalAppText()
{
	BString appdir(fLocalAppDirText->Text());
	if (appdir == "")
		return;

	BString appname;
	appdir.CopyInto(appname, appdir.FindLast('/') + 1, appdir.Length());
	appname.Capitalize();

	BString target, targetPath;
	bool foundTarget = _FindMakefile(target);

	if (foundTarget == true)
		// Target found, set that
		targetPath << appdir << "/" << target;
	else
		// Target not found, make assumptions
		targetPath << appdir << "/" << appname;

	fProjectNameText->SetText(appname);
	fProjectTargetText->SetText(targetPath);
}

void
NewProjectWindow::_RemoveStaleEntries(const char* dirpath)
{
	BDirectory dir(dirpath);
	BEntry startEntry(dirpath);
	BEntry entry;

	while (dir.GetNextEntry(&entry) == B_OK) {
		BString text;
		if (entry.IsDirectory()) {
			BDirectory newdir(&entry);

			if (newdir.CountEntries() > 0) {
				BPath newPath(dirpath);
				newPath.Append(entry.Name());

				_RemoveStaleEntries(newPath.Path());
			}
			else {
				entry.Remove();
				text << "\n" << B_TRANSLATE("Removing stale empty dir:")
					<< " " << entry.Name();
				fProjectDescription->Insert(text);
			}
		}
		else {
			entry.Remove();
				text << "\n" << B_TRANSLATE("Removing stale file:")
					<< " " << entry.Name();
			fProjectDescription->Insert(text);
		}
	}

	BString str;
	startEntry.Remove();
	str << "\n" << B_TRANSLATE("Removing stale dir:") << " " << startEntry.Name();
	fProjectDescription->Insert(str);
}

bool
NewProjectWindow::_FindMakefile(BString& target)
{
	BDirectory projectDir(fLocalAppDirText->Text());
	BEntry entry;
	entry_ref ref;
	char sname[B_FILE_NAME_LENGTH];

	while (projectDir.GetNextEntry(&entry) == B_OK) {
		if (entry.IsFile()) {
				entry.GetName(sname);
				BString name(sname);
				// Standard name first in case there are many
				if (name == "Makefile")
					return _ParseMakefile(target, &entry);
				else if (name == "makefile")
					return _ParseMakefile(target, &entry);
				else if (name.IFindFirst("makefile") != B_ERROR)
					return _ParseMakefile(target, &entry);
		}
	}
	// No target found
	return false;
}

bool
NewProjectWindow::_ParseMakefile(BString& target, const BEntry* entry)
{
	BPath path;
	entry->GetPath(&path);
	std::ifstream file(path.Path());
	std::string str;
	BString targetName, targetString;
	bool nameFound = false;

	if (file.is_open()) {
		while (getline(file, str)) {
			BString line(str.c_str());
			line.Trim();

			// Avoid comments
			if (line.StartsWith("#") == false) {
				// Haiku Makefile (hopefully)
				if (line.StartsWith("NAME")) {
					// Target found
					nameFound = true;
					line.CopyInto(targetName, line.FindLast('=') + 1, line.Length());

					// Empty NAME, trouble
					if (targetName.Trim() == "")
						fProjectDescription->SetText(B_TRANSLATE("ERROR: empty \"NAME\" in Makefile"));

					// Do not break right now, it could be a standard makefile
					// with a NAME variable unset
					continue;
				}
				if (line.StartsWith("TARGET_DIR")) {
					// Target dir found
					line.CopyInto(targetString, line.FindLast('=') + 1, line.Length());

					targetString.Trim();
					// Assuming Haiku makefile: should have NAME set, then TARGET_DIR
					// maybe set. So break and assign target string if:
					// both are found in correct order and NAME not empty
					if (nameFound == true && targetName != "") {
						if (targetString != "" || targetString != ".")
							target << targetString << "/" << targetName;
						else
							target << targetName;

						break;
					}
					continue;

				} else if (line.IFindFirst("TARGET") != B_ERROR) {
					// Traditional Makefile
					line.CopyInto(targetString, line.FindLast('=') + 1, line.Length());

					if (targetString.Trim() != "") {
						target << targetString;
						nameFound = true;
					} else
						fProjectDescription->SetText(B_TRANSLATE("ERROR: empty \"TARGET\" in Makefile"));

					break;
				}
			}
		}
		file.close();
	}

	return nameFound;
}


/**
 * Upon changing project type only relevant controls are enabled.
 *
 */
void
NewProjectWindow::_UpdateControlsState(int32 selection)
{
	if (selection < 0 ) {
		fCreateButton->SetEnabled(false);
		fProjectDescription->SetText(B_TRANSLATE("Invalid selection"));
		return;
	}

	BStringItem *item = dynamic_cast<BStringItem*>(fTypeListView->ItemAt(selection));

	// Find item description in map and display it
	ProjectTypeIterator iter = projectTypeMap.find(item);
	fProjectDescription->SetText(iter->second);

	// Clean controls
	fProjectNameText->SetText("");
	fGitEnabled->SetValue(B_CONTROL_OFF);
	fProjectTargetText->SetText("");
	fRunInTeminal->SetValue(B_CONTROL_OFF);
	fAddFileText->SetText("");
	fAddHeader->SetValue(B_CONTROL_OFF);
	fAddSecondFileText->SetText("");
	fAddSecondHeader->SetValue(B_CONTROL_OFF);
	fHaikuAppDirText->SetText("");
	fLocalAppDirText->SetText("");

	// Set controls
	fProjectNameText->SetEnabled(true);
	fGitEnabled->SetEnabled(true);
	fProjectTargetText->SetEnabled(true);
	fRunInTeminal->SetEnabled(false);
	fAddFileText->SetEnabled(true);
	fAddHeader->SetEnabled(false);
	fAddSecondFileText->SetEnabled(false);
	fAddSecondHeader->SetEnabled(false);
	fHaikuAppDirText->SetEnabled(false);
	fBrowseHaikuAppButton->SetEnabled(false);
	fLocalAppDirText->SetEnabled(false);
	fBrowseLocalAppButton->SetEnabled(false);
	fCargoPathText->SetEnabled(false);
	fCargoBinEnabled->SetEnabled(false);
	fCargoVcsEnabled->SetEnabled(false);

	if (item == appItem) {

//	fAddHeader->SetEnabled(true);
	fAddHeader->SetValue(B_CONTROL_ON);

	} else if (item == appMenuItem) {

	fAddSecondFileText->SetEnabled(true);
//	fAddHeader->SetEnabled(true);
//	fAddSecondHeader->SetEnabled(true);
	fAddHeader->SetValue(B_CONTROL_ON);
	fAddSecondHeader->SetValue(B_CONTROL_ON);

	} else if (item == helloCplusItem) {
		//fRunInTeminal->SetEnabled(true);
		fRunInTeminal->SetValue(B_CONTROL_ON);

	} else if (item == helloCItem) {
		//fRunInTeminal->SetEnabled(true);
		fRunInTeminal->SetValue(B_CONTROL_ON);

	} else if (item == principlesItem) {
		fRunInTeminal->SetEnabled(true);
		fRunInTeminal->SetValue(B_CONTROL_ON);

	} else if (item == emptyItem) {

		fRunInTeminal->SetEnabled(true);
		fAddFileText->SetEnabled(false);

	} else if (item == sourcesItem) {
		fGitEnabled->SetEnabled(false);
		fRunInTeminal->SetEnabled(true);
		fAddFileText->SetEnabled(false);
		fHaikuAppDirText->SetEnabled(true);
		fBrowseHaikuAppButton->SetEnabled(true);

	} else if (item == existingItem) {
		fGitEnabled->SetEnabled(false);
//		fProjectTargetText->SetEnabled(false);
		fRunInTeminal->SetEnabled(true);
		fAddFileText->SetEnabled(false);
		fLocalAppDirText->SetEnabled(true);
		fBrowseLocalAppButton->SetEnabled(true);

	} else if (item == cargoItem) {
		fGitEnabled->SetEnabled(false);
		fProjectTargetText->SetEnabled(false);
		fRunInTeminal->SetEnabled(true);
		fRunInTeminal->SetValue(B_CONTROL_ON);
		fAddFileText->SetEnabled(false);
		fCargoPathText->SetEnabled(true);
		fCargoBinEnabled->SetEnabled(true);
		fCargoVcsEnabled->SetEnabled(true);
//	} else if (item == gitItem) {
	}

	fCreateButton->SetEnabled(true);
}

void
NewProjectWindow::_UpdateControlsData(int32 selection)
{
	// No project selected, exit
	if (selection < 0 ) {
		fProjectDescription->SetText(B_TRANSLATE("Please choose a project first"));
		return;
	}

	BStringItem *item = dynamic_cast<BStringItem*>(fTypeListView->ItemAt(selection));

	// Import and Cargo items are managed elsewhere
	if (item == sourcesItem || item == existingItem || item == cargoItem)
		return;

	BString name = fProjectNameText->Text();
	fProjectTargetText->SetText(name);

	// Get rid of garbage on changing project
	if (name == "")
		return;

	if (item == appItem) {

		name.Append(".cpp");
		fAddFileText->SetText(name);

	} else if (item == appMenuItem) {

		fProjectTargetText->SetText(name);
		BString name2(name);
		name.Append("App.cpp");
		fAddFileText->SetText(name);
		name2.Append("Window.cpp");
		fAddSecondFileText->SetText(name2);

	} else if (item == helloCplusItem ) {

		name.Append(".cpp");
		fAddFileText->SetText(name);

	} else if (item == helloCItem ) {

		name.Append(".c");
		fAddFileText->SetText(name);

	} else if (item == principlesItem ) {

		name.Append(".cpp");
		fAddFileText->SetText(name);
	}
}

status_t
NewProjectWindow::_WriteMakefile()
{
	status_t status;
	BFile file;
	BPath path(fProjectsDirectoryText->Text());
	path.Append(fProjectNameText->Text());

	path.Append("Makefile");
	status = file.SetTo(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_FAIL_IF_EXISTS);
	if (status != B_OK)
		return status;

	BString makefile;
	makefile <<
"## Genio haiku Makefile ########################################################\n\n";
	if (f_primary_architecture == "x86_gcc2") {
	makefile	<< "CC  := gcc-x86\n"
				<< "CXX := g++-x86\n\n";
	}
	makefile
		<< "NAME := " << fProjectTargetText->Text() << "\n\n"
		<< "TARGET_DIR := app\n\n"
		<< "TYPE := APP\n\n"// TODO get
		<< "APP_MIME_SIG := \"application/x-vnd.Genio-"
		<< fProjectTargetText->Text() <<"\"\n\n";

	if (fCurrentItem == appMenuItem)
		makefile << "SRCS :=  src/" << fAddFileText->Text() << "\n"
					"SRCS +=  src/" << fAddSecondFileText->Text() << "\n\n";
	else
		makefile << "SRCS :=  src/" << fAddFileText->Text() << "\n\n";

	makefile << "RDEFS :=  \n\n";

	if (fCurrentItem == appItem)
		makefile << "LIBS = be $(STDCPPLIBS)\n\n";
	else if (fCurrentItem == appMenuItem)
		makefile << "LIBS = be localestub $(STDCPPLIBS)\n\n";
	else
		makefile << "LIBS = $(STDCPPLIBS)\n\n";
		

	makefile << "LIBPATHS =\n\n";

	if (fCurrentItem == principlesItem) {
		makefile << "SYSTEM_INCLUDE_PATHS += /boot/home/config/non-packaged/develop/headers/\n\n";
	}

	makefile << "OPTIMIZE := FULL\n\n";

	if (fCurrentItem == appMenuItem)
		makefile << "CFLAGS := -Wall -Werror\n\n";
	else
		makefile << "CFLAGS := -Wall\n\n";

	makefile << "CXXFLAGS := -std=c++11\n\n"
		<< "LOCALES :=\n\n"
		<< "DEBUGGER := true\n\n"
		<< "## Include the Makefile-Engine\n"
		<< "DEVEL_DIRECTORY := \\\n"
		<< "\t$(shell findpaths -r \"makefile_engine\" B_FIND_PATH_DEVELOP_DIRECTORY)\n"
		<< " include $(DEVEL_DIRECTORY)/etc/makefile-engine\n\n"
		// TODO hope to be merged upstream
		<< "$(OBJ_DIR)/%.o : %.cpp\n"
		<< "\t$(C++) -c $< $(INCLUDES) $(CFLAGS) $(CXXFLAGS) -o \"$@\"\n";

	ssize_t bytes = file.Write(makefile.String(), makefile.Length());
	if (bytes != makefile.Length())
		return B_ERROR; // TODO tune
	file.Flush();

//	fProjectFile->AddString("project_make", path.Path());
	fProjectFile->AddString("project_file", path.Path());
	return B_OK;
}

status_t
NewProjectWindow::_WriteAppfiles()
{
	status_t status;
	BFile file;
	BPath cppPath(fProjectsDirectoryText->Text());
	cppPath.Append(fProjectNameText->Text());
	cppPath.Append("src");

	BPath hPath(cppPath.Path());

	BString fileName(fAddFileText->Text());
	fileName.Remove(fileName.FindLast('.'), fileName.Length());
	BString hFileName(fileName);
	hFileName.Append(".h");

	std::string headComment  = Genio::Copyright();
//	headComment << "/*\n"
//				<< " * Copyright " << fYear << " Your Name <your@email.address>\n"
//				<< " * All rights reserved. Distributed under the terms of the MIT license.\n"
//				<< " */\n\n";

	if (fAddHeader->Value() == true) {
		std::string upperFileName = Genio::HeaderGuard(fileName.String());
		upperFileName += "_H";

		hPath.Append(hFileName);
		status = file.SetTo(hPath.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_FAIL_IF_EXISTS);
		if (status != B_OK)
			return status;

		BString hFileContent;
		hFileContent << headComment.c_str()
			<< "#ifndef " << upperFileName.c_str() << "\n"
			<< "#define " << upperFileName.c_str() << "\n\n"
			<< "#include <Application.h>\n\n"
			<< "class " << fileName << " : public BApplication {\n"
			<< "public:\n"
			<< "\t\t\t\t\t\t\t\t" << fileName << "();\n"
			<< "\tvirtual\t\t\t\t\t\t~" << fileName << "();\n\n"
			<< "private:\n\n"
			<< "};\n\n"
			<< "#endif //" << upperFileName.c_str() << "\n";

		ssize_t bytes = file.Write(hFileContent.String(), hFileContent.Length());
		if (bytes != hFileContent.Length())
			return B_ERROR; // TODO tune
		file.Flush();

		fProjectFile->AddString("project_source", hPath.Path());
	}

	cppPath.Append(fAddFileText->Text());
	status = file.SetTo(cppPath.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_FAIL_IF_EXISTS);
	if (status != B_OK)
		return status;

	BString cppFileContent;
	cppFileContent << headComment.c_str()
		<< "#include \"" << hFileName <<  "\"\n\n"
		<< "#include <String.h>\n"
		<< "#include <StringView.h>\n"
		<< "#include <Window.h>\n\n"
		<< "BString kApplicationSignature(\"application/x-vnd.Genio-"
		<< fileName << "\");\n\n"
		<< fileName << "::" << fileName << "()\n"
		<< "\t:\n"
		<< "\tBApplication(kApplicationSignature)\n"
		<< "{\n"
		<< "\tBWindow* window = new BWindow(BRect(100, 100, 599, 399), "
		<< "\"" << fileName << "\",\n"
		<< "\t\tB_TITLED_WINDOW, B_QUIT_ON_WINDOW_CLOSE);\n\n"
		<< "\tBStringView* stringView = new BStringView("
		<< "\"StringView\", \"Hello World!\");\n"
		<< "\tstringView->ResizeToPreferred();\n"
		<< "\twindow->AddChild(stringView);\n\n"
		<< "\twindow->Show();\n"
		<< "}\n\n"
		<< fileName << "::~" << fileName << "()\n"
		<< "{\n"
		<< "}\n\n"
		<< "int\n"
		<< "main(int argc, char* argv[])\n"
		<< "{\n"
		<< "\t" << fileName << " app;\n"
		<< "\tapp.Run();\n\n"
		<< "\treturn 0;\n"
		<< "}\n";

	ssize_t bytes = file.Write(cppFileContent.String(), cppFileContent.Length());
	if (bytes != cppFileContent.Length())
		return B_ERROR; // TODO tune
	file.Flush();

	fProjectFile->AddString("project_source", cppPath.Path());

	return B_OK;
}

status_t
NewProjectWindow::_WriteAppMenufiles()
{
	// Files path
	status_t status;
	BFile file;
	BPath cppPath(fProjectsDirectoryText->Text());
	cppPath.Append(fProjectNameText->Text());
	cppPath.Append("src");
	BPath cpp2Path(cppPath);

	// Headers path
	BPath hPath(cppPath.Path());
	BPath hSecondPath(cppPath.Path());

	BString fileName(fAddFileText->Text());
	BString fileSecondName(fAddSecondFileText->Text());
	BString fileNameStripped, fileSecondNameStripped;

	// Manage extension
	fileName.CopyInto(fileNameStripped, 0, fileName.FindLast("."));
	fileSecondName.CopyInto(fileSecondNameStripped, 0, fileSecondName.FindLast("."));
	BString hFileName(fileNameStripped);
	BString hSecondFileName(fileSecondNameStripped);
	hFileName.Append(".h");
	hSecondFileName.Append(".h");

	std::string headComment  = Genio::Copyright();

	// App header
	if (fAddHeader->Value() == true) {
		std::string upperFileName = Genio::HeaderGuard(fileNameStripped.String());
		upperFileName += "_H";


		hPath.Append(hFileName);
		status = file.SetTo(hPath.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_FAIL_IF_EXISTS);
		if (status != B_OK)
			return status;

		BString hFileContent;
		hFileContent << headComment.c_str()
			<< "#ifndef " << upperFileName.c_str() << "\n"
			<< "#define " << upperFileName.c_str() << "\n\n"
			<< "#include <Application.h>\n\n"
			<< "class " << fileNameStripped << " : public BApplication {\n"
			<< "public:\n"
			<< "\t\t\t\t\t\t\t\t" << fileNameStripped << "();\n"
			<< "\tvirtual\t\t\t\t\t\t~" << fileNameStripped << "();\n\n"
			<< "private:\n\n"
			<< "};\n\n"
			<< "#endif //" << upperFileName.c_str() << "\n";

		ssize_t bytes = file.Write(hFileContent.String(), hFileContent.Length());
		if (bytes != hFileContent.Length())
			return B_ERROR; // TODO tune
		file.Flush();

		fProjectFile->AddString("project_source", hPath.Path());
	}

	// Window header
	if (fAddSecondHeader->Value() == true) {
		std::string upperFileName = Genio::HeaderGuard(fileSecondNameStripped.String());
		upperFileName += "_H";

		hSecondPath.Append(hSecondFileName);
		status = file.SetTo(hSecondPath.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_FAIL_IF_EXISTS);
		if (status != B_OK)
			return status;

		// TODO Camel case ?
		BString h2FileContent;
		h2FileContent << headComment.c_str()
			<< "#ifndef " << upperFileName.c_str() << "\n"
			<< "#define " << upperFileName.c_str() << "\n\n"
			<< "#include <GroupLayout.h>\n"
			<< "#include <Window.h>\n\n"
			<< "class " << fileSecondNameStripped << " : public BWindow\n"
			<< "{\n"
			<< "public:\n"
			<< "\t\t\t\t\t\t\t\t" << fileSecondNameStripped << "();\n"
			<< "\tvirtual\t\t\t\t\t\t~" << fileSecondNameStripped << "();\n\n"
			<< "\tvirtual void\t\t\t\tMessageReceived(BMessage* message);\n\n"
			<< "private:\n"
			<< "\t\t\tBGroupLayout*\t\tfRootLayout;\n"
			<< "};\n\n"
			<< "#endif //" << upperFileName.c_str() << "\n";

		ssize_t bytes = file.Write(h2FileContent.String(), h2FileContent.Length());
		if (bytes != h2FileContent.Length())
			return B_ERROR; // TODO tune
		file.Flush();

		fProjectFile->AddString("project_source", hSecondPath.Path());
	}

	// App file
	cppPath.Append(fAddFileText->Text());
	status = file.SetTo(cppPath.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_FAIL_IF_EXISTS);
	if (status != B_OK)
		return status;

	BString cppFileContent;
	cppFileContent << headComment.c_str()
		<< "#include \"" << hFileName <<  "\"\n\n"
		<< "#include <String.h>\n\n"
		<< "#include \"" << hSecondFileName <<  "\"\n\n"

		<< "BString kApplicationSignature(\"application/x-vnd.Genio-"
		<< fileNameStripped << "\");\n\n"
		<< fileNameStripped << "::" << fileNameStripped << "()\n"
		<< "\t:\n"
		<< "\tBApplication(kApplicationSignature)\n"
		<< "{\n"
		<< "\t" << fileSecondNameStripped << "* window = new "
		<< fileSecondNameStripped << "();\n"
		<< "\twindow->Show();\n"
		<< "}\n\n"

		<< fileNameStripped << "::~" << fileNameStripped << "()\n"
		<< "{\n"
		<< "}\n\n"

		<< "int\n"
		<< "main(int argc, char* argv[])\n"
		<< "{\n"
		<< "\ttry {\n"
		<< "\t\tnew " << fileNameStripped << "();\n\n"

		<< "\t\tbe_app->Run();\n\n"

		<< "\t\tdelete be_app;\n\n"

		<< "\t} catch (...) {\n\n"

		<< "\t\tdebugger(\"Exception caught.\");\n"
		<< "\t}\n\n"
		<< "\treturn 0;\n"
		<< "}\n";

	ssize_t bytes = file.Write(cppFileContent.String(), cppFileContent.Length());
	if (bytes != cppFileContent.Length())
		return B_ERROR; // TODO tune
	file.Flush();

	fProjectFile->AddString("project_source", cppPath.Path());

	// Window file
	cpp2Path.Append(fAddSecondFileText->Text());
	status = file.SetTo(cpp2Path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_FAIL_IF_EXISTS);
	if (status != B_OK)
		return status;

	BString cpp2FileContent;
	cpp2FileContent << headComment.c_str()
		<< "#include \"" << hSecondFileName <<  "\"\n\n"
		<< "#include <Catalog.h>\n"
		<< "#include <LayoutBuilder.h>\n"
		<< "#include <MenuBar.h>\n\n"

		<< "#undef B_TRANSLATION_CONTEXT\n"
		<< "#define B_TRANSLATION_CONTEXT \"" << fileSecondNameStripped << "\"\n\n"

		<< fileSecondNameStripped << "::" << fileSecondNameStripped << "()\n"
		<< "\t:\n"
		<< "\tBWindow(BRect(100, 100, 499, 399), \""
		<< fProjectNameText->Text() << "\", B_TITLED_WINDOW,\n"
		<< "\t\t\t\t\t\t\t\t\t\t\t\tB_QUIT_ON_WINDOW_CLOSE)\n"
		<< "{\n"
		<< "\t// Menu\n"
		<< "\tBMenuBar *menuBar = new BMenuBar(\"menubar\");\n"
		<< "\tBLayoutBuilder::Menu<>(menuBar)\n"
		<< "\t\t.AddMenu(B_TRANSLATE(\"File\"))\n"
		<< "\t\t.AddItem(B_TRANSLATE(\"Quit\"), B_QUIT_REQUESTED, 'Q')\n"
		<< "\t\t.End();\n\n"

		<< "\t// Layout\n"
		<< "\tfRootLayout = BLayoutBuilder::Group<>(this, B_VERTICAL)\n"
		<< "\t\t.SetInsets(0, 0, 0, 0)\n"
		<< "\t\t.Add(menuBar, 0)\n"
		<< "\t\t.AddGlue();\n"
		<< "}\n\n"

		<< fileSecondNameStripped << "::~" << fileSecondNameStripped << "()\n"
		<< "{\n"
		<< "}\n\n"

		<< "void\n"
		<< fileSecondNameStripped << "::MessageReceived(BMessage* message)\n"
		<< "{\n"
		<< "\tswitch (message->what) {\n\n"

		<< "\t\tdefault:\n"
		<< "\t\t\tBWindow::MessageReceived(message);\n"
		<< "\t\t\tbreak;\n"
		<< "\t}\n"
		<< "}\n";

	bytes = file.Write(cpp2FileContent.String(), cpp2FileContent.Length());
	if (bytes != cpp2FileContent.Length())
		return B_ERROR; // TODO tune
	file.Flush();

	fProjectFile->AddString("project_source", cpp2Path.Path());

	return B_OK;
}

status_t
NewProjectWindow::_WriteHelloCplusfile()
{
	status_t status;
	BFile file;
	BPath cppPath(fProjectsDirectoryText->Text());
	cppPath.Append(fProjectNameText->Text());
	cppPath.Append("src");
	cppPath.Append(fAddFileText->Text());
	status = file.SetTo(cppPath.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_FAIL_IF_EXISTS);
	if (status != B_OK)
		return status;

	BString cppFileContent;
	cppFileContent
		<< "#include <iostream>\n"
		<< "#include <string>\n\n"

		<< "int main(int argc, char* argv[])\n"
		<< "{\n"
		<< "\tstd::string helloString(\"Hello world!\");\n\n"
			
		<< "\tstd::cout << helloString << std::endl;\n\n"
			
		<< "\treturn 0;\n"
		<< "}\n";

	ssize_t bytes = file.Write(cppFileContent.String(), cppFileContent.Length());
	if (bytes != cppFileContent.Length())
		return B_ERROR; // TODO tune
	file.Flush();

	fProjectFile->AddString("project_source", cppPath.Path());

	return B_OK;
}

status_t
NewProjectWindow::_WriteHelloCfile()
{
	status_t status;
	BFile file;
	BPath cPath(fProjectsDirectoryText->Text());
	cPath.Append(fProjectNameText->Text());
	cPath.Append("src");
	cPath.Append(fAddFileText->Text());
	status = file.SetTo(cPath.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_FAIL_IF_EXISTS);
	if (status != B_OK)
		return status;

	BString cFileContent;
	cFileContent
			<< "#include <stdio.h>\n\n"

			<< "void print(const char* message)\n"
			<< "{\n"
			<< "\tfprintf(stdout, \"\%s\\n\", message);\n"
			<< "}\n\n"

			<< "int\n"
			<< "main(int argc, char* argv[])\n"
			<< "{\n"
			<< "\tprint(\"Hello World!\");\n\n"
			<< "\treturn 0;\n"
			<< "}\n";

	ssize_t bytes = file.Write(cFileContent.String(), cFileContent.Length());
	if (bytes != cFileContent.Length())
		return B_ERROR; // TODO tune
	file.Flush();

	fProjectFile->AddString("project_source", cPath.Path());

	return B_OK;
}

status_t
NewProjectWindow::_WritePrinciplesfile()
{
	status_t status;
	BFile file;
	BPath cppPath(fProjectsDirectoryText->Text());
	cppPath.Append(fProjectNameText->Text());
	cppPath.Append("src");
	cppPath.Append(fAddFileText->Text());
	status = file.SetTo(cppPath.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_FAIL_IF_EXISTS);
	if (status != B_OK)
		return status;

	BString cppFileContent;
	cppFileContent
		<< "#include \"std_lib_facilities.h\"\n\n"

		<< "int main()\n"
		<< "{\n"
		<< "\tcout << \"Hello, World!\\n\";\n"
		<< "\treturn 0;\n"
		<< "}\n";

	ssize_t bytes = file.Write(cppFileContent.String(), cppFileContent.Length());
	if (bytes != cppFileContent.Length())
		return B_ERROR; // TODO tune
	file.Flush();

	fProjectFile->AddString("project_source", cppPath.Path());

	return B_OK;
}

// Trying to create a simple Makefile for newcomers.
// Leave room for user improvements.
status_t
NewProjectWindow::_WriteHelloCMakefile()
{
	status_t status;
	BFile file;
	BPath path(fProjectsDirectoryText->Text());
	path.Append(fProjectNameText->Text());

	path.Append("Makefile");
	status = file.SetTo(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_FAIL_IF_EXISTS);
	if (status != B_OK)
		return status;

	BString arch("");
	if (f_primary_architecture == "x86_gcc2")
		arch.SetTo("-x86");

	BString makefile;
	makefile <<
"## Genio simple Makefile #######################################################\n"
		<< "##\n"
		<< "CC     := gcc" << arch << "\n"
		<< "LD     := gcc" << arch << "\n"
		<< "CFLAGS := -c -Wall -O2 -g" << "\n\n"

		<< "# application name\n"
		<< "target	:= app/" << fProjectTargetText->Text() << "\n\n"

		<< "# sources\n"
		<< "sources := src/" << fAddFileText->Text() << "\n\n"

		<< "# object files\n"
		<< "objects := $(sources:.c=.o)\n"
		<<
"################################################################################\n"
		<< "all: $(target)\n\n"

		<< "# link objects to make an executable\n"
		<< "$(target): $(objects)\n"
		<< "\t${LD} -o $@ $^\n\n"

		<< "# build objects from sources\n"
		<< "$(objects): $(sources)\n"
		<< "\t${CC} ${CFLAGS} $< -o $@\n\n"

		<< "clean:\n"
		<< "\t@echo cleaning: $(objects)\n"
		<< "\t@rm -f $(objects)\n\n"

		<< "rmapp:\n"
		<< "\t@echo cleaning: $(target)\n"
		<< "\t@rm -f $(target)\n\n"

		<< ".PHONY:  clean rmapp\n";

	ssize_t bytes = file.Write(makefile.String(), makefile.Length());
	if (bytes != makefile.Length())
		return B_ERROR; // TODO tune
	file.Flush();

//	fProjectFile->AddString("project_make", path.Path());
	fProjectFile->AddString("project_file", path.Path());

	return B_OK;
}

