/*
 * Copyright 2023 Nexus6 
 * All rights reserved. Distributed under the terms of the MIT license.
 */
 
#include <stdio.h>

#include <AppKit.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <LayoutBuilder.h>
#include <SupportKit.h>
#include <Window.h>

#include "RemoteProjectWindow.h"
#include "Utils.h"
#include "GenioWindowMessages.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "RemoteProjectWindow"

std::map<float,bool> progress_tracker;
BHandler *this_handler = nullptr;

RemoteProjectWindow::RemoteProjectWindow(BString repo, BString dirPath, const BMessenger target)
	:
	BWindow(BRect(0, 0, 600, 200), "Open remote repository",
			B_TITLED_WINDOW, B_NOT_V_RESIZABLE | B_NOT_ZOOMABLE),
	fIsCloning(false),
	fTarget(target),
	fClone(nullptr),
	fCancel(nullptr),
	fProgressTextView(nullptr),
	fProgressBar(nullptr)
{
	fURL = new BTextControl(B_TRANSLATE("URL:"), "", NULL);
	fPathBox = new PathBox("pathbox", dirPath.String(), "Path:");
	fClone = new BButton("ok", B_TRANSLATE("Clone"),
			new BMessage(kDoClone));
	fCancel = new BButton("ok", B_TRANSLATE("Cancel"),
			new BMessage(kCancel));

	// fProgressTextView = new BTextView(BRect(0, 0, 280, 80), "_clone_", 
							  // BRect(0, 0, 280, 80), B_FOLLOW_LEFT_RIGHT);
	// fProgressTextView->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	// fProgressTextView->MakeEditable(false);
	// fProgressTextView->MakeSelectable(false);
	// fProgressTextView->SetWordWrap(true);
	// fProgressTextView->SetText("Cloning" B_UTF8_ELLIPSIS "\nRepository");

	fProgressBar = new BStatusBar("progressBar");
	fProgressBar->SetBarHeight(be_plain_font->Size() * 1.5);

	// test
	fPathBox->SetPath("/boot/home/workspace/clone_test");
	fURL->SetText("https://github.com/Genio-The-Haiku-IDE/Genio");


	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.SetInsets(10)
		.Add(fURL)
		.Add(fPathBox)
		.AddGlue()
		// .Add(fProgressTextView)
		.Add(fProgressBar)
		.AddGlue()
		.AddGroup(B_HORIZONTAL, 0)
			.AddGlue()
			.Add(fCancel)
			.Add(fClone)
			.End();

	CenterOnScreen();
	Layout(true);
	Show();
	
	this_handler = this;
}

void
RemoteProjectWindow::_OpenProject(const path& localPath)
{
	// open the project folder
	BMessage *msg = new BMessage(MSG_PROJECT_FOLDER_OPEN);
	entry_ref ref;
	BEntry entry(localPath.c_str());
	if (entry.GetRef(&ref) == B_OK) {
		msg->AddRef("refs",&ref);
		fTarget.SendMessage(msg);
	} else {
		throw runtime_error(B_TRANSLATE("The local path is not valid or does not exist"));
	}
}

void 
RemoteProjectWindow::_ResetControls() 
{
	fIsCloning = false;
	fClone->SetEnabled(true);
	fCancel->SetEnabled(false);
	fPathBox->SetEnabled(true);
	fURL->SetEnabled(true);
}

void
RemoteProjectWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case TASK_RESULT_MESSAGE:
		{
			try {
				auto result = TaskResult<BPath>::Instantiate(msg)->GetResult();
				_OpenProject(result.Path());
			} catch(std::exception &ex) {
				OKAlert("OpenRemoteProject", BString("An error occurred while opening a remote project: ") << ex.what(), B_INFO_ALERT);
			}
			_ResetControls();
			Quit();
		}
		break;
		case kDoClone:
		{
			fIsCloning = true;
			fClone->SetEnabled(false);
			fCancel->SetEnabled(true);
			fPathBox->SetEnabled(false);
			fURL->SetEnabled(false);
			
			auto callback = [](const git_transfer_progress *stats, void *payload) -> int {

								int current_progress = stats->total_objects > 0 ?
									(100*stats->received_objects) /
									stats->total_objects :
									0;
								int kbytes = stats->received_bytes / 1024;
								
								BString progressString;
								progressString << "Network " << current_progress 
									<< " (" << kbytes << " kb, "
									<< stats->received_objects << "/"
									<< stats->total_objects << ")\n";
								
								BMessage msg(kProgress);
								msg.AddString("progress_text", progressString);
								msg.AddFloat("progress_value", current_progress);
								BMessenger(this_handler).SendMessage(&msg);
								return 0;
							};
			
			fCurrentTask = make_shared<Task<BPath>>("GitClone", new BMessenger(this), 
													&GitRepository::Clone, 
													fURL->Text(), 
													BPath(fPathBox->Path()),
													callback);
			fCurrentTask->Run();
	
			break;
		}
		case kCancel:
		{
			if (fIsCloning) {
				BAlert* alert = new BAlert("OpenRemoteProject", B_TRANSLATE("Do you want to stop cloning the repository?"),
					B_TRANSLATE("Continue"), B_TRANSLATE("Stop"), nullptr,
					B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);

				alert->SetShortcut(0, B_ESCAPE);

				int32 choice = alert->Go();

				if (choice == 1) {
					fCurrentTask->Stop();
				}
				_ResetControls();
			} else {
				Quit();
			}
			break;
		}
		case kProgress:
		{
			auto text = msg->GetString("progress_text");
			auto progress = msg->GetFloat("progress_value", 0);
			if (LockLooper()) {
				if (text != nullptr)
					fProgressBar->SetTrailingText(text);
				if (progress != 0)
					fProgressBar->SetTo(progress);
				UnlockLooper();
			}
		}
		break;
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