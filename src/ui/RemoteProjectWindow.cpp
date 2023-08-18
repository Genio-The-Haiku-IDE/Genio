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
#include "helpers/Task.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "RemoteProjectWindow"

using namespace Genio::Task;

RemoteProjectWindow::RemoteProjectWindow(BString repo, BString dirPath, const BMessenger target)
	:
	BWindow(BRect(0, 0, 600, 180), "Open remote repository",
			B_TITLED_WINDOW, B_NOT_V_RESIZABLE | B_NOT_ZOOMABLE),
	fIsCloning(false),
	fTarget(target),
	fClone(nullptr),
	fCancel(nullptr)
{
	fURL = new BTextControl(B_TRANSLATE("URL:"), "", NULL);
	fPathBox = new PathBox("pathbox", dirPath.String(), "Path:");
	fClone = new BButton("ok", B_TRANSLATE("Clone"),
			new BMessage(kDoClone));
	fCancel = new BButton("ok", B_TRANSLATE("Cancel"),
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
		case TASK_RESULT_MESSAGE:
		{
			try {
				auto result = TaskResult<status_t>(*msg).GetResult();
				OKAlert("", BString("Result: ") << result, B_INFO_ALERT);
			} catch(std::exception &ex) {
				OKAlert("", BString("Exception: ") << ex.what(), B_INFO_ALERT);
			}
			
		}
		break;
		case kDoClone:
		{
			fIsCloning = true;
			fClone->SetEnabled(false);
			fCancel->SetEnabled(true);
			fPathBox->SetEnabled(false);
			fURL->SetEnabled(false);
			
			Genio::Task::Task<status_t> task("test", new BMessenger(this), 
												&Genio::Git::GitRepository::Clone, 
												fURL->Text(), 
												fPathBox->Path());
			task.Run();
	
			break;
		}
		case kCancel:
		{
			if (!fIsCloning) {
				BAlert* alert = new BAlert("CancelCloneing", B_TRANSLATE("Do you want to stop cloning the repository?"),
					B_TRANSLATE("Continue"), B_TRANSLATE("Stop"), nullptr,
					B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);

				alert->SetShortcut(0, B_ESCAPE);

				int32 choice = alert->Go();

				if (choice == 1) {
					Quit();
				}
			}
			break;
		}
		case kFinished:
		{
			try {
				if (!fIsCloning) {
					BAlert* alert = new BAlert("CancelCloneing", B_TRANSLATE("Do you want to stop cloning the repository?"),
						B_TRANSLATE("Continue"), B_TRANSLATE("Stop"), nullptr,
						B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);

					alert->SetShortcut(0, B_ESCAPE);

					int32 choice = alert->Go();

					if (choice == 1) {
						Quit();
					}
				}
			} catch (Genio::Git::GitException &e) {
				OKAlert("Git", e.Message().c_str(), B_STOP_ALERT);
			}
			
			fIsCloning = false;
			fClone->SetEnabled(true);
			fCancel->SetEnabled(false);
			fPathBox->SetEnabled(true);
			fURL->SetEnabled(true);
			
			break;
		}
		default:
			BWindow::MessageReceived(msg);
	}
}

// OpenRemoteProgressWindow::OpenRemoteProgressWindow(RemoteProjectWindow* cloneWindow)
	// :
	// BWindow(BRect(0, 0, 300, 150), "TrackGit - Clone Progress",
			// B_TITLED_WINDOW, B_NOT_CLOSABLE | B_NOT_RESIZABLE)
// {
	// fRemoteProjectWindow = cloneWindow;
	// fTextView = new BTextView(BRect(0, 0, 280, 80), "_clone_", 
							  // BRect(0, 0, 280, 80), B_FOLLOW_LEFT_RIGHT);
	// fTextView->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	// fTextView->MakeEditable(false);
	// fTextView->MakeSelectable(false);
	// fTextView->SetWordWrap(true);
	// fTextView->SetText("Cloning" B_UTF8_ELLIPSIS "\nRepository");
// 
	// fProgressBar = new BStatusBar("progressBar");
	// fProgressBar->SetBarHeight(20);
// 
	// BButton* fCancel = new BButton("ok", "Cancel",
								  // new BMessage(kCancel));
// 
    // BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		// .SetInsets(10)
		// .Add(fTextView)
		// .Add(fProgressBar)
		// .AddGroup(B_HORIZONTAL, 0)
			// .AddGlue()
			// .Add(fCancel)
			// .End();
// }
// 
// 
// void
// OpenRemoteProgressWindow::SetText(const char* text)
// {
	// if (LockLooper()) {
		// fTextView->SetText(text);
		// UnlockLooper();
	// }
// }
// 
// 
// void
// OpenRemoteProgressWindow::SetProgress(float progress)
// {
	// if (LockLooper()) {
		// fProgressBar->SetTo(progress);
		// UnlockLooper();
	// }
// }
// 
// 
// void
// OpenRemoteProgressWindow::MessageReceived(BMessage* msg)
// {
	// switch (msg->what) {
		// case kCancel:
			// fRemoteProjectWindow->PostMessage(kCancel);
			// Quit();
			// break;
		// default:
			// BWindow::MessageReceived(msg);
	// }
// }