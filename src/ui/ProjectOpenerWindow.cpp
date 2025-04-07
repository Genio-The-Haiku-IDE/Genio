/*
 * Copyright 2025, The Genio team
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "ProjectOpenerWindow.h"

#include <BarberPole.h>
#include <Button.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <FilePanel.h>
#include <LayoutBuilder.h>
#include <SeparatorView.h>
#include <StatusBar.h>
#include <StringView.h>

#include "GenioWindow.h"
#include "GenioWindowMessages.h"
#include "ProjectBrowser.h"
#include "ProjectFolder.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ProjectOpenerWindow"

ProjectOpenerWindow::ProjectOpenerWindow(const entry_ref* ref,
	const BMessenger& messenger, bool activate)
	:
	BWindow(BRect(0, 0, 600, 200), B_TRANSLATE("Opening project"),
			B_TITLED_WINDOW,
			B_ASYNCHRONOUS_CONTROLS |
			B_NOT_ZOOMABLE |
			B_NOT_RESIZABLE |
			B_AVOID_FRONT |
			B_AUTO_UPDATE_SIZE_LIMITS |
			B_CLOSE_ON_ESCAPE),
	fTarget(messenger)
{
	/*fURL = new BTextControl(B_TRANSLATE("URL:"), "", NULL);
	fPathBox = new BTextControl(B_TRANSLATE("Base path:"), dirPath.String(), nullptr);
	fCancel = new BButton("cancel button", B_TRANSLATE("Cancel"),
			new BMessage(kCancel));
*/
	fCancel = new BButton("cancel button", B_TRANSLATE("Cancel"),
			new BMessage('9999'));

	fBarberPole = new BarberPole("barber pole");
	fProgressBar = new BStatusBar("progress bar");
	fProgressLayout = new BCardLayout();
	fProgressView = new BView("progress view", 0);
	fStatusText = new BStringView("status text", nullptr);
	//fButtonsView = new BView("buttons view", 0);
	//fButtonsLayout = new BCardLayout();

	fProgressView->SetLayout(fProgressLayout);
	fProgressLayout->AddView(0, fBarberPole);
	fProgressLayout->AddView(1, fProgressBar);

	fStatusText->SetDrawingMode(B_OP_ALPHA);

	fStatusText->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT,
		B_ALIGN_VERTICAL_CENTER));

	// TODO: We should use a Grid instead, to align the controls more nicely
	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.SetInsets(10)
		.AddGlue()
		.Add(fProgressLayout)
		.AddGroup(B_HORIZONTAL)
			.Add(fStatusText)
			.AddGlue()
			.Add(fCancel)
		.End();

	SetDefaultButton(fCancel);
	CenterOnScreen();
	Show();
	
	_OpenProject(ref, activate);
}


void
ProjectOpenerWindow::_OpenProject(const entry_ref* ref, bool activate)
{
	ProjectFolder* newProject = new ProjectFolder(*ref, fTarget);
	
	BMessage message(MSG_PROJECT_OPEN_INITIATED);
	message.AddPointer("project", newProject);
	message.AddRef("ref", ref);
	message.AddBool("activate", activate);
	
	fTarget.SendMessage(&message);
	
	status_t status = newProject->Open();
	
	if (status != B_OK) {
		message.what = MSG_PROJECT_OPEN_ABORTED;
		fTarget.SendMessage(&message);
		return;
	}

	// TODO: Locking ?
	gMainWindow->GetProjectBrowser()->ProjectFolderPopulate(newProject);

	message.what = MSG_PROJECT_OPEN_COMPLETED;
	fTarget.SendMessage(&message);
	
	//PostMessage(new BMessage(B_WINDOW_CLOSE));
}


