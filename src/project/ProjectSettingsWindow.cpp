/*
 * Copyright 2017..2018 A. Mosca <amoscaster@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <iostream>
#include <memory>
#include <string>

#include <Alignment.h>
#include <Catalog.h>
#include <Directory.h>
#include <LayoutBuilder.h>
#include <SeparatorView.h>

#include "GenioNamespace.h"
#include "ProjectFolder.h"
#include "ProjectSettingsWindow.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ProjectSettingsWindow"

enum
{
	MSG_CANCEL_CLICKED				= 'canc',
	MSG_SAVE_CLICKED				= 'save',
	MSG_BUILD_MODE_SELECTED			= 'bmse',
};

ProjectSettingsWindow::ProjectSettingsWindow(ProjectFolder *project)
	:
	BWindow(BRect(0, 0, 799, 599), "ProjectSettingsWindow", B_MODAL_WINDOW,
													B_ASYNCHRONOUS_CONTROLS | 
													B_NOT_ZOOMABLE |
//													B_NOT_RESIZABLE |
													B_AVOID_FRONT |
													B_AUTO_UPDATE_SIZE_LIMITS |
													B_CLOSE_ON_ESCAPE)
	, fProject(project)
{
	_InitWindow();

	CenterOnScreen();

	_LoadProject();
}

ProjectSettingsWindow::~ProjectSettingsWindow()
{
}

bool
ProjectSettingsWindow::QuitRequested()
{
	Quit();
	return true;
}

void
ProjectSettingsWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case MSG_CANCEL_CLICKED: {
			PostMessage(B_QUIT_REQUESTED);
			break;
		}
		case MSG_SAVE_CLICKED: {
			_CloseProject();
			PostMessage(B_QUIT_REQUESTED);
			break;
		}
		default: {
			BWindow::MessageReceived(msg);
			break;
		}
	}
}

void
ProjectSettingsWindow::_CloseProject()
{
	if (fProject != nullptr) {
		_SaveChanges();
	}
}

void
ProjectSettingsWindow::_InitWindow()
{
	// "Build Commands" Box
	fBuildCommandsBox = new BBox("BuildCommandsBox");
	fBuildCommandsBox->SetLabel(B_TRANSLATE("Build Commands"));

	fReleaseBuildCommandText = new BTextControl(B_TRANSLATE("Release build comand:"), "", nullptr);
	fDebugBuildCommandText = new BTextControl(B_TRANSLATE("Debug build comand:"), "", nullptr);

	fReleaseCleanCommandText = new BTextControl(B_TRANSLATE("Release clean comand:"), "", nullptr);
	fDebugCleanCommandText = new BTextControl(B_TRANSLATE("Debug clean comand:"), "", nullptr);

	fReleaseProjectTargetText = new BTextControl(B_TRANSLATE("Release project target:"), "", nullptr);
	fDebugProjectTargetText = new BTextControl(B_TRANSLATE("Debug project target:"), "", nullptr);
	
	fReleaseExecuteArgsText = new BTextControl(B_TRANSLATE("Release execute arguments:"), "", nullptr);
	fDebugExecuteArgsText = new BTextControl(B_TRANSLATE("Debug execute arguments:"), "", nullptr);
	
	fRunInTerminal = new BCheckBox("RunInTerminalCheckBox", B_TRANSLATE("Run in Terminal"), nullptr);
	
	fEnableGit = new BCheckBox("EnableGitCheckBox", B_TRANSLATE("Enable Git"), nullptr);
	fExcludeSettingsGit = new BCheckBox("ExcludeSettingsGitCheckBox", B_TRANSLATE("Exclude settings file from Git"), nullptr);

	BLayoutBuilder::Grid<>(fBuildCommandsBox)
	.SetInsets(10.0f, 24.0f, 10.0f, 10.0f)
	.Add(fReleaseBuildCommandText->CreateLabelLayoutItem(), 0, 1)
	.Add(fReleaseBuildCommandText->CreateTextViewLayoutItem(), 1, 1)
	.Add(fReleaseCleanCommandText->CreateLabelLayoutItem(), 2, 1)
	.Add(fReleaseCleanCommandText->CreateTextViewLayoutItem(), 3, 1)
	.Add(fDebugBuildCommandText->CreateLabelLayoutItem(), 0, 2)
	.Add(fDebugBuildCommandText->CreateTextViewLayoutItem(), 1, 2)
	.Add(fDebugCleanCommandText->CreateLabelLayoutItem(), 2, 2)
	.Add(fDebugCleanCommandText->CreateTextViewLayoutItem(), 3, 2)
	.End()
	;

	// "Target" Box
	fTargetBox = new BBox("TargetBox");
	fTargetBox->SetLabel(B_TRANSLATE("Target"));

	BLayoutBuilder::Grid<>(fTargetBox)
	.SetInsets(10.0f, 24.0f, 10.0f, 10.0f)
	.Add(fReleaseProjectTargetText->CreateLabelLayoutItem(), 0, 0)
	.Add(fReleaseProjectTargetText->CreateTextViewLayoutItem(), 1, 0)
	.Add(fReleaseExecuteArgsText->CreateLabelLayoutItem(), 2, 0)
	.Add(fReleaseExecuteArgsText->CreateTextViewLayoutItem(), 3, 0)
	.Add(fDebugProjectTargetText->CreateLabelLayoutItem(), 0, 1)
	.Add(fDebugProjectTargetText->CreateTextViewLayoutItem(), 1, 1)
	.Add(fDebugExecuteArgsText->CreateLabelLayoutItem(), 2, 1)
	.Add(fDebugExecuteArgsText->CreateTextViewLayoutItem(), 3, 1)
	.Add(fRunInTerminal, 0, 2)
	.End();
	
	// "Source Control" Box
	fSourceControlBox = new BBox("SourceControlBox");
	fSourceControlBox->SetLabel(B_TRANSLATE("Source Control"));

	BLayoutBuilder::Grid<>(fSourceControlBox)
	.SetInsets(10.0f, 24.0f, 10.0f, 10.0f)
	.Add(fEnableGit, 0, 0)
	.Add(fExcludeSettingsGit, 0, 1)
	.AddGlue(0,2,4)
	.End();

	// "Project" global Box
	fProjectBox = new BBox("projectBox", B_WILL_DRAW | B_FRAME_EVENTS |
		B_NAVIGABLE_JUMP, B_NO_BORDER);
	fProjectBoxLabel = B_TRANSLATE("Project:");
	fProjectBox->SetLabel(fProjectBoxLabel);

	BLayoutBuilder::Grid<>(fProjectBox)
	.SetInsets(10.0f, 24.0f, 10.0f, 10.0f)
	.Add(fBuildCommandsBox, 0, 4, 4)
	.Add(fTargetBox, 0, 5, 4)
	.Add(fSourceControlBox, 0, 6, 4)
	.AddGlue(0, 7, 4)
	;

	// Cancel button
	BButton* cancelButton = new BButton("cancel",
		B_TRANSLATE("Cancel"), new BMessage(MSG_CANCEL_CLICKED));
	// Save button
	BButton* saveButton = new BButton("save",
		B_TRANSLATE("Save"), new BMessage(MSG_SAVE_CLICKED));

	// Window layout
	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.SetInsets(10.0f)
		.AddGroup(B_HORIZONTAL)
			.AddGroup(B_VERTICAL)
				.Add(fProjectBox)
				.AddGroup(B_HORIZONTAL)
					.AddGlue()
					.Add(cancelButton)
					.Add(saveButton)
					.AddGlue()
				.End()
			.End()
		.End()
	;
}

void
ProjectSettingsWindow::_LoadProject()
{
	// Init controls
	// Release controls
	fProjectBoxProjectLabel.SetTo("");
	fReleaseProjectTargetText->SetText("");
	fReleaseBuildCommandText->SetText("");
	fReleaseCleanCommandText->SetText("");
	fReleaseExecuteArgsText->SetText("");
	// Debug controls
	fDebugProjectTargetText->SetText("");
	fDebugBuildCommandText->SetText("");
	fDebugCleanCommandText->SetText("");
	fDebugExecuteArgsText->SetText("");
	// Others
	fRunInTerminal->SetValue(B_CONTROL_OFF);
	fEnableGit->SetValue(B_CONTROL_OFF);
	fExcludeSettingsGit->SetValue(B_CONTROL_OFF);

	fProjectBoxProjectLabel << fProjectBoxLabel << "\t\t" << fProject->Name();
	fProjectBox->SetLabel(fProjectBoxProjectLabel);

	BuildMode originalBuildMode = fProject->GetBuildMode();
	
	fProject->SetBuildMode(BuildMode::ReleaseMode);
	fReleaseProjectTargetText->SetText(fProject->GetTarget());
	fReleaseBuildCommandText->SetText(fProject->GetBuildCommand());
	fReleaseCleanCommandText->SetText(fProject->GetCleanCommand());
	fReleaseExecuteArgsText->SetText(fProject->GetExecuteArgs());
	
	fProject->SetBuildMode(BuildMode::DebugMode);
	fDebugProjectTargetText->SetText(fProject->GetTarget());
	fDebugBuildCommandText->SetText(fProject->GetBuildCommand());
	fDebugCleanCommandText->SetText(fProject->GetCleanCommand());
	fDebugExecuteArgsText->SetText(fProject->GetExecuteArgs());
	
	fProject->SetBuildMode(originalBuildMode);
	
	if (fProject->RunInTerminal())
		fRunInTerminal->SetValue(B_CONTROL_ON);
	
	if (fProject->Git())
		fEnableGit->SetValue(B_CONTROL_ON);
		
	if (fProject->ExcludeSettingsOnGit())
		fExcludeSettingsGit->SetValue(B_CONTROL_ON);
}

void
ProjectSettingsWindow::_SaveChanges()
{	
	fProject->SetTarget(fReleaseProjectTargetText->Text(), BuildMode::ReleaseMode);
	fProject->SetBuildCommand(fReleaseBuildCommandText->Text(), BuildMode::ReleaseMode);
	fProject->SetCleanCommand(fReleaseCleanCommandText->Text(), BuildMode::ReleaseMode);
	fProject->SetExecuteArgs(fReleaseExecuteArgsText->Text(), BuildMode::ReleaseMode);
	
	fProject->SetTarget(fDebugProjectTargetText->Text(), BuildMode::DebugMode);
	fProject->SetBuildCommand(fDebugBuildCommandText->Text(), BuildMode::DebugMode);
	fProject->SetCleanCommand(fDebugCleanCommandText->Text(), BuildMode::DebugMode);
	fProject->SetExecuteArgs(fDebugExecuteArgsText->Text(), BuildMode::DebugMode);
	
	if (fRunInTerminal->Value() == B_CONTROL_ON)
		fProject->RunInTerminal(true);
	else
		fProject->RunInTerminal(false);
		
	if (fEnableGit->Value() == B_CONTROL_ON)
		fProject->Git(true);
	else
		fProject->Git(false);
		
	if (fExcludeSettingsGit->Value() == B_CONTROL_ON)
		fProject->ExcludeSettingsOnGit(true);
	else
		fProject->ExcludeSettingsOnGit(false);
}