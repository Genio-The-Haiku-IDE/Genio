/*
 * Copyright 2023 Nexus6 
 * All rights reserved. Distributed under the terms of the MIT license.
 * Based on TrackGit (https://github.com/HaikuArchives/TrackGit)
 * Original author: Hrishikesh Hiraskar  
 * Copyright Hrishikesh Hiraskar and other HaikuArchives contributors (see GitHub repo for details)
 */
 
#include <stdio.h>

#include <AppKit.h>
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <SupportKit.h>
#include <Window.h>

#include "RemoteProjectWindow.h"
#include "Utils.h"
#include "GenioWindowMessages.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "RemoteProjectWindow"


RemoteProjectWindow::RemoteProjectWindow(BString repo, BString dirPath, const BMessenger target)
	:
	BWindow(BRect(0, 0, 600, 180), "Open remote repository",
			B_TITLED_WINDOW, B_NOT_V_RESIZABLE | B_NOT_ZOOMABLE),
	fIsCloning(false),
	fTarget(target)
{
	fURL = new BTextControl(B_TRANSLATE("URL:"), "", NULL);
	fPathBox = new PathBox("pathbox", dirPath.String(), "Path:");
	BButton* fClone = new BButton("ok", B_TRANSLATE("Clone"),
			new BMessage(kDoClone));
	BButton* fCancel = new BButton("ok", B_TRANSLATE("Cancel"),
			new BMessage(kCancel));

				// test
				fPathBox->SetPath("/boot/home/workspace/clone_test");
				fURL->SetText("https://github.com/Genio-The-Haiku-IDE/Genio");

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.SetInsets(10)
		.Add(fURL)
		.Add(fPathBox)
		.AddGroup(B_HORIZONTAL, 0)
			.AddGlue()
			.Add(fCancel)
			.Add(fClone)
			.End();

	CenterOnScreen();
	Layout(true);
	Show();
}

void
RemoteProjectWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case kDoClone:
		{
			using namespace Genio::Git;
			GitRepository *repo = nullptr;
			try {
				fIsCloning = true;
				
				auto path = fPathBox->Path();
				auto url = fURL->Text();
				repo = new GitRepository(path);
				if (!repo->InitCheck()) {
					repo->Clone(url, path);
					// open the project folder
					BMessage *msg = new BMessage(MSG_PROJECT_FOLDER_OPEN);
					entry_ref ref;
					BEntry entry(path);
					if (entry.GetRef(&ref) == B_OK) {
						msg->AddRef("refs",&ref);
						fTarget.SendMessage(msg);
						Quit();
					} else {
						OKAlert("Git", B_TRANSLATE("The local path is not valid or does not exist"), 
								B_STOP_ALERT);
					}
				} else {
					OKAlert("Git", B_TRANSLATE("The local path is already a valid Git repository or is not empty"), 
							B_STOP_ALERT);
				}
			} catch (GitException &e) {
				OKAlert("Git", e.Message().c_str(), B_STOP_ALERT);
			}
			
			if (repo != nullptr)
				delete repo;
			fIsCloning = false;
			break;
		}
		case kCancel:
		{
			if (!fIsCloning)
				Quit();
			break;
		}
		default:
			BWindow::MessageReceived(msg);
	}
}

OpenRemoteProgressWindow::OpenRemoteProgressWindow(RemoteProjectWindow* cloneWindow)
	:
	BWindow(BRect(0, 0, 300, 150), "TrackGit - Clone Progress",
			B_TITLED_WINDOW, B_NOT_CLOSABLE | B_NOT_RESIZABLE)
{
	fRemoteProjectWindow = cloneWindow;
	fTextView = new BTextView(BRect(0, 0, 280, 80), "_clone_", 
							  BRect(0, 0, 280, 80), B_FOLLOW_LEFT_RIGHT);
	fTextView->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	fTextView->MakeEditable(false);
	fTextView->MakeSelectable(false);
	fTextView->SetWordWrap(true);
	fTextView->SetText("Cloning" B_UTF8_ELLIPSIS "\nRepository");

	fProgressBar = new BStatusBar("progressBar");
	fProgressBar->SetBarHeight(20);

	BButton* fCancel = new BButton("ok", "Cancel",
								  new BMessage(kCancel));

    BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.SetInsets(10)
		.Add(fTextView)
		.Add(fProgressBar)
		.AddGroup(B_HORIZONTAL, 0)
			.AddGlue()
			.Add(fCancel)
			.End();
}


void
OpenRemoteProgressWindow::SetText(const char* text)
{
	if (LockLooper()) {
		fTextView->SetText(text);
		UnlockLooper();
	}
}


void
OpenRemoteProgressWindow::SetProgress(float progress)
{
	if (LockLooper()) {
		fProgressBar->SetTo(progress);
		UnlockLooper();
	}
}


void
OpenRemoteProgressWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case kCancel:
			fRemoteProjectWindow->PostMessage(kCancel);
			Quit();
			break;
		default:
			BWindow::MessageReceived(msg);
	}
}