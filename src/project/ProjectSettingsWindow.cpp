/*
 * Copyright 2017..2018 A. Mosca 
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "ProjectSettingsWindow.h"

#include <Alignment.h>
#include <Catalog.h>
#include <Directory.h>
#include <LayoutBuilder.h>
#include <SeparatorView.h>
#include <iostream>
#include <string>

#include "GenioNamespace.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ProjectSettingsWindow"

enum
{
	MSG_EXIT_CLICKED				= 'excl',
	MSG_PROJECT_SELECTED			= 'prse',
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
	_CloseProject();
	Quit();

	return true;
}

void
ProjectSettingsWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case MSG_EXIT_CLICKED: {
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
	// Projects Menu
	fProjectMenuField = new BMenuField("ProjectMenuField", nullptr,
										new BMenu(B_TRANSLATE("Choose Project:")));

	// "Editables" Box
	fBuildCommandsBox = new BBox("EditablesBox");
	fBuildCommandsBox->SetLabel(B_TRANSLATE("Editables"));

	fProjectTargetText = new BTextControl(B_TRANSLATE("Project target:"), "", nullptr);

	fBuildCommandText = new BTextControl(B_TRANSLATE("Build comand:"), "", nullptr);

	fCleanCommandText = new BTextControl(B_TRANSLATE("Clean comand:"), "", nullptr);

	fProjectScmText = new BTextControl(B_TRANSLATE("Source control:"), "", nullptr);

	fProjectTypeText = new BTextControl(B_TRANSLATE("Project type:"), "", nullptr);

	BLayoutBuilder::Grid<>(fBuildCommandsBox)
	.SetInsets(10.0f, 24.0f, 10.0f, 10.0f)
	.Add(fProjectTargetText->CreateLabelLayoutItem(), 0, 1, 1)
	.Add(fProjectTargetText->CreateTextViewLayoutItem(), 1, 1, 3)
	.Add(fBuildCommandText->CreateLabelLayoutItem(), 0, 2)
	.Add(fBuildCommandText->CreateTextViewLayoutItem(), 1, 2)
	.Add(fCleanCommandText->CreateLabelLayoutItem(), 2, 2)
	.Add(fCleanCommandText->CreateTextViewLayoutItem(), 3, 2)
	.Add(fProjectScmText->CreateLabelLayoutItem(), 0, 3)
	.Add(fProjectScmText->CreateTextViewLayoutItem(), 1, 3)
	.Add(fProjectTypeText->CreateLabelLayoutItem(), 2, 3)
	.Add(fProjectTypeText->CreateTextViewLayoutItem(), 3, 3)
	.End()
	;

	// "Runtime" Box
	fRuntimeBox = new BBox("RuntimeBox");
	fRuntimeBox->SetLabel(B_TRANSLATE("Runtime"));

	fRunArgsText = new BTextControl(B_TRANSLATE("Runtime args:"), "", nullptr);

	BLayoutBuilder::Grid<>(fRuntimeBox)
	.SetInsets(10.0f, 24.0f, 10.0f, 10.0f)
	.Add(fRunArgsText->CreateLabelLayoutItem(), 0, 1, 2)
	.Add(fRunArgsText->CreateTextViewLayoutItem(), 2, 1, 2)
	.End()
	;

	// "Project" global Box
	fProjectBox = new BBox("projectBox");
	fProjectBoxLabel = B_TRANSLATE("Project:");
	fProjectBox->SetLabel(fProjectBoxLabel);

	BLayoutBuilder::Grid<>(fProjectBox)
	.SetInsets(10.0f, 24.0f, 10.0f, 10.0f)
	.Add(fProjectMenuField, 0, 1, 4)
//	.Add(new BSeparatorView(B_HORIZONTAL), 0, 2, 4)
//	.AddGlue(0, 3, 4)
	.Add(fBuildCommandsBox, 0, 4, 4)
	.Add(fRuntimeBox, 0, 5, 4)
	.AddGlue(0, 6, 4)
	;

	// Exit button
	BButton* exitButton = new BButton("exit",
		B_TRANSLATE("Exit"), new BMessage(MSG_EXIT_CLICKED));

	// Window layout
	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
//		.SetInsets(2.0f)
		.AddGroup(B_HORIZONTAL)
			.AddGroup(B_VERTICAL)
				.Add(fProjectBox)
				.AddGroup(B_HORIZONTAL)
					.AddGlue()
					.Add(exitButton)
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
	fProjectBoxProjectLabel.SetTo("");
	fProjectTargetText->SetText("");
	fBuildCommandText->SetText("");
	fCleanCommandText->SetText("");
	fProjectScmText->SetText("");
	fProjectTypeText->SetText("");
	fRunArgsText->SetText("");

	fProjectBoxProjectLabel << fProjectBoxLabel << "\t\t" << fProject->Name();
	fProjectBox->SetLabel(fProjectBoxProjectLabel);

	fProjectTargetText->SetText(fProject->GetTarget());
	fBuildCommandText->SetText(fProject->GetBuildCommand());
	fCleanCommandText->SetText(fProject->GetCleanCommand());

	// if (fIdmproFile->FindString("project_scm", &fProjectScmString) == B_OK)
		// fProjectScmText->SetText(fProjectScmString);

	// if (fIdmproFile->FindString("project_type", &fProjectTypeString) == B_OK)
		// fProjectTypeText->SetText(fProjectTypeString);

	// if (fIdmproFile->FindString("project_run_args", &fRunArgsString) == B_OK)
		// fRunArgsText->SetText(fRunArgsString);

	// BString file;
	// int count = 0;

	// while (fIdmproFile->FindString("parseless_item", count++, &file) == B_OK) {
		// file.Append("\n");
		// fParselessText->Insert(file);
	// }
}

void
ProjectSettingsWindow::_SaveChanges()
{
	// BString target(fProjectTargetText->Text());
	// if (target != fTargetString)
		// fIdmproFile->SetBString("project_target", target);

	// BString build(fBuildCommandText->Text());
	// if (build != fBuildString)
		// fIdmproFile->SetBString("project_build_command", build);

	// BString clean(fCleanCommandText->Text());
	// if (clean != fCleanString)
		// fIdmproFile->SetBString("project_clean_command", clean);

	// BString runargs(fRunArgsText->Text());
	// if (runargs != fRunArgsString)
		// fIdmproFile->SetBString("project_run_args", runargs);

	// BString scm(fProjectScmText->Text());
	// if (scm != fProjectScmString)
		// fIdmproFile->SetBString("project_scm", scm);

	// BString type(fProjectTypeText->Text());
	// if (type != fProjectTypeString)
		// fIdmproFile->SetBString("project_type", type);
}
