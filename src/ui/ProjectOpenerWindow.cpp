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

#include "GenioWindow.h"
#include "GenioWindowMessages.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ProjectOpenerWindow"

const uint32 kCancel = 'canc';

class ProjectProgressView : public BGroupView {
public:
	ProjectProgressView(const char* name);
private:
	BButton* 					fCancel;
	BStatusBar*					fProgressBar;
	BarberPole*					fBarberPole;
	BCardLayout*				fProgressLayout;
	BView*						fProgressView;
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
			B_AUTO_UPDATE_SIZE_LIMITS),
	fTarget(messenger)
{
	SetLayout(new BGroupLayout(B_VERTICAL));
	Show();
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


// ProjectProgressView
ProjectProgressView::ProjectProgressView(const char* name)
	:
	BGroupView(name)
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
			//.Add(new BButton(B_TRANSLATE("Cancel"), new BMessage(kCancel)))
		.End()
	.End();

	fProgressLayout->SetVisibleItem(int32(0));
	
	BString text(B_TRANSLATE("Loading project '%project_name%'"));
	text.ReplaceFirst("%project_name%", name);
	fStatusText->SetText(text);
	fBarberPole->Start();
}
