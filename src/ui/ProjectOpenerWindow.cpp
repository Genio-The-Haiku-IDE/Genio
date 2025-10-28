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
#include <Screen.h>
#include <StatusBar.h>
#include <StringView.h>

#include "GenioWindow.h"
#include "GenioWindowMessages.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ProjectOpenerWindow"

const uint32 kCancel = 'canc';

class ProjectProgressView : public BView {
public:
	ProjectProgressView(const char* name);
private:
	//BButton* 					fCancel;
	BStatusBar*					fProgressBar;
	BarberPole*					fBarberPole;
	BStringView*				fStatusText;
};



ProjectOpenerWindow::ProjectOpenerWindow(const BMessenger& messenger)
	:
	BWindow(BRect(0, 0, 400, 200), B_TRANSLATE("Loading project"), B_FLOATING_WINDOW,
			B_ASYNCHRONOUS_CONTROLS |
			B_NOT_CLOSABLE |
			B_NOT_ZOOMABLE |
			B_NOT_RESIZABLE |
			B_AVOID_FRONT |
			B_AUTO_UPDATE_SIZE_LIMITS|B_PULSE_NEEDED),
	fTarget(messenger)
{
	SetLayout(new BGroupLayout(B_VERTICAL));
}


/* virtual */
void
ProjectOpenerWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case '0001':
		{
			const char* name;
			message->FindString("project_name", &name);			
			dynamic_cast<BGroupLayout*>(GetLayout())->AddView(new ProjectProgressView(name));
			_MoveAndResize();
			if (IsHidden())
				Show();
			break;
		}
		
		case '0002':
		{
			const char* name;
			message->FindString("project_name", &name);
			BGroupLayout* layout = dynamic_cast<BGroupLayout*>(GetLayout());
			BView* view = FindView(name);
			if (view != nullptr)
				layout->RemoveView(view);
			_MoveAndResize();
			if (CountChildren() <= 0)
				Hide();
			break;
		}
		case kCancel:
		{
			/*fBarberPole->Stop();
			BMessage openAbortedMessage(MSG_PROJECT_OPEN_ABORTED);
			openAbortedMessage.AddPointer("project", fProject);
			openAbortedMessage.AddRef("ref", fProject->EntryRef());
			openAbortedMessage.AddBool("activate", fActivate);
			fTarget.SendMessage(&openAbortedMessage);
			*/
			PostMessage(new BMessage(B_QUIT_REQUESTED));
			break;
		}
		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void
ProjectOpenerWindow::_MoveAndResize()
{
	const BRect screenFrame(BScreen().Frame());
	MoveTo(screenFrame.right - Frame().Width(), screenFrame.bottom - Frame().Height());
}


// ProjectProgressView
ProjectProgressView::ProjectProgressView(const char* name)
	:
	BView(name, B_WILL_DRAW)
{
	fBarberPole = new BarberPole("barber pole");
	fProgressBar = new BStatusBar("progress bar");
	fStatusText = new BStringView("status text", nullptr);

	fStatusText->SetDrawingMode(B_OP_ALPHA);

	fStatusText->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT,
		B_ALIGN_VERTICAL_CENTER));

	// TODO: We should use a Grid instead, to align the controls more nicely
	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.SetInsets(10)
		.AddGroup(B_HORIZONTAL)
			.Add(fStatusText)
			.Add(fBarberPole)
			//.Add(new BButton(B_TRANSLATE("Cancel"), new BMessage(kCancel)))
		.End()
	.End();

	BString text(B_TRANSLATE("Loading project '%project_name%'"));
	text.ReplaceFirst("%project_name%", name);
	fStatusText->SetText(text);

	fBarberPole->Start();
}
