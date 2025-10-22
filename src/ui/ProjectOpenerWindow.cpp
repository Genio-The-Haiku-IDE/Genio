/*
 * Copyright 2025, The Genio team
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "ProjectOpenerWindow.h"

#include <BarberPole.h>
#include <Button.h>
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <SeparatorView.h>
#include <StatusBar.h>
#include <StringView.h>
#include <functional>

#include "GenioWindow.h"
#include "GenioWindowMessages.h"
#include "ProjectBrowser.h"
#include "ProjectFolder.h"
#include "Task.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ProjectOpenerWindow"

const uint32 kCancel = 'canc';

using Genio::Task::Task;
using Genio::Task::TaskResult;

ProjectOpenerWindow::ProjectOpenerWindow(const entry_ref* ref,
	const BMessenger& messenger, bool activate)
	:
	BWindow(BRect(0, 0, 400, 200), B_TRANSLATE("Loading project"),
			B_MODAL_WINDOW,
			B_ASYNCHRONOUS_CONTROLS |
			B_NOT_CLOSABLE |
			B_NOT_ZOOMABLE |
			B_NOT_RESIZABLE |
			B_AVOID_FRONT |
			B_AUTO_UPDATE_SIZE_LIMITS),
	fTarget(messenger),
	fProject(nullptr)
{
	fBarberPole = new BarberPole("barber pole");
	fProgressBar = new BStatusBar("progress bar");
	fProgressLayout = new BCardLayout();
	fProgressView = new BView("progress view", 0);
	fStatusText = new BStringView("status text", nullptr);

	fProgressView->SetLayout(fProgressLayout);
	fProgressLayout->AddView(0, fBarberPole);
	fProgressLayout->AddView(1, fProgressBar);

	fStatusText->SetDrawingMode(B_OP_ALPHA);

	fStatusText->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT,
		B_ALIGN_VERTICAL_CENTER));

	// TODO: We should use a Grid instead, to align the controls more nicely
	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.SetInsets(10)
		.AddGroup(B_HORIZONTAL)
			.Add(fStatusText)
			.Add(fProgressLayout)
		.End()
	.End();

	fProgressLayout->SetVisibleItem(int32(0));

	CenterOnScreen();
	Show();
	
	_OpenProject(ref, activate);
}


/* virtual */
void
ProjectOpenerWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kCancel:
			fBarberPole->Stop();

			// TODO: Interrupt loading:
			// TODO: Send the MSG_PROJECT_OPEN_ABORTED message

			PostMessage(new BMessage(B_QUIT_REQUESTED));
			break;
		case Genio::Task::TASK_RESULT_MESSAGE:
		{
			Lock();
			fBarberPole->Stop();
			Unlock();

			BMessage openCompletedMessage(MSG_PROJECT_OPEN_COMPLETED);
			openCompletedMessage.AddPointer("project", fProject);
			openCompletedMessage.AddRef("ref", fProject->EntryRef());
			openCompletedMessage.AddBool("activate", true);
			fTarget.SendMessage(&openCompletedMessage);
	
			PostMessage(new BMessage(B_QUIT_REQUESTED));
			break;
		}
		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void
ProjectOpenerWindow::_OpenProject(const entry_ref* ref, bool activate)
{
	fProject = new ProjectFolder(*ref, fTarget);

	BString text(B_TRANSLATE("Loading project '%project_name%'"));
	text.ReplaceFirst("%project_name%", fProject->Name());
	Lock();
	fStatusText->SetText(text);
	Unlock();

	BMessage message(MSG_PROJECT_OPEN_INITIATED);
	message.AddPointer("project", fProject);
	message.AddRef("ref", ref);
	message.AddBool("activate", activate);
	
	fTarget.SendMessage(&message);
	
	status_t status = fProject->Open();
	if (status != B_OK) {
		Lock();
		fBarberPole->Stop();
		Unlock();

		// GenioWindow will delete the allocated project
		message.what = MSG_PROJECT_OPEN_ABORTED;
		fTarget.SendMessage(&message);
		return;
	}

	Lock();
	fBarberPole->Start();
	Unlock();
	BString taskName;
	taskName << fProject->Name() << "OpenerTask";
	Task<void> task
	(
		taskName,
		BMessenger(this),
		std::bind
		(
			&ProjectBrowser::ProjectFolderPopulate,
			gMainWindow->GetProjectBrowser(),
			fProject
		)
	);
	
	task.Run();
}
