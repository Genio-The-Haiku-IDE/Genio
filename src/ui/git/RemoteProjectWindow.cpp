/*
 * Copyright 2023 Nexus6 
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "RemoteProjectWindow.h"

#include <cstdio>
#include <regex>

#include <AppKit.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <git2.h>
#include <LayoutBuilder.h>
#include <SeparatorView.h>
#include <StatusBar.h>
#include <StringView.h>
#include <Url.h>
#include <Window.h>

#include "GenioWindowMessages.h"
#include "GitCredentialsWindow.h"
#include "GitRepository.h"
#include "Utils.h"

#include "BeDC.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "RemoteProjectWindow"

BHandler *this_handler = nullptr;

#define VIEW_INDEX_BARBER_POLE	(int32) 0
#define VIEW_INDEX_PROGRESS_BAR	(int32) 1
#define VIEW_INDEX_CLONE_BUTTON	(int32) 0
#define VIEW_INDEX_CLOSE_BUTTON	(int32) 1

RemoteProjectWindow::RemoteProjectWindow(BString repo, BString dirPath, const BMessenger target)
	:
	BWindow(BRect(0, 0, 600, 200), B_TRANSLATE("Open remote Git project"),
			B_TITLED_WINDOW,
			B_ASYNCHRONOUS_CONTROLS |
			B_NOT_ZOOMABLE |
			B_NOT_RESIZABLE |
			B_AVOID_FRONT |
			B_AUTO_UPDATE_SIZE_LIMITS |
			B_CLOSE_ON_ESCAPE),
	fIsCloning(false),
	fTarget(target),
	fClone(nullptr),
	fCancel(nullptr),
	fProgressBar(nullptr),
	fBarberPole(nullptr),
	fProgressLayout(nullptr),
	fProgressView(nullptr),
	fStatusText(nullptr)
{
	fURL = new BTextControl(B_TRANSLATE("URL:"), "", NULL);
	fPathBox = new PathBox("pathbox", dirPath.String(), B_TRANSLATE("Base path:"));
	fDestDir = new BTextControl(B_TRANSLATE("Destination directory:"), "", nullptr);
	fClone = new BButton("clone button", B_TRANSLATE("Clone"),
			new BMessage(kDoClone));
	fCancel = new BButton("cancel button", B_TRANSLATE("Cancel"),
			new BMessage(kCancel));
	fClose = new BButton("close button", B_TRANSLATE("Close"),
			new BMessage(kClose));

	fBarberPole = new BarberPole("barber pole");
	fProgressBar = new BStatusBar("progress bar");
	fProgressLayout = new BCardLayout();
	fProgressView = new BView("progress view", 0);
	fStatusText = new BStringView("status text", nullptr);
	fButtonsView = new BView("buttons view", 0);
	fButtonsLayout = new BCardLayout();

	fProgressView->SetLayout(fProgressLayout);
	fProgressLayout->AddView(VIEW_INDEX_BARBER_POLE, fBarberPole);
	fProgressLayout->AddView(VIEW_INDEX_PROGRESS_BAR, fProgressBar);

	fButtonsView->SetLayout(fButtonsLayout);
	fButtonsLayout->AddView(VIEW_INDEX_CLONE_BUTTON, fClone);
	fButtonsLayout->AddView(VIEW_INDEX_CLOSE_BUTTON, fClose);

	fStatusText->SetDrawingMode(B_OP_ALPHA);

	// TODO: Improve size
	fURL->SetExplicitMinSize(BSize(500, B_SIZE_UNSET));

	fStatusText->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT,
		B_ALIGN_VERTICAL_CENTER));

#if 0
	// test
	fURL->SetText("https://github.com/Genio-The-Haiku-IDE/Genio");
#endif

	fURL->SetTarget(this);
	fURL->SetModificationMessage(new BMessage(kUrlModified));

	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.SetInsets(10)
		.AddGlue()
		.Add(fURL)
		.Add(fPathBox)
		.Add(fDestDir)
		.Add(fProgressLayout)
		.AddGroup(B_HORIZONTAL)
			.Add(fStatusText)
			.AddGlue()
			.Add(fCancel)
			.Add(fButtonsLayout)
		.End();

	_SetIdle();

	SetDefaultButton(fClone);
	CenterOnScreen();
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
	fDestDir->SetEnabled(true);
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
				_SetProgress(100, "Finished!");
				fButtonsLayout->SetVisibleItem(VIEW_INDEX_CLOSE_BUTTON);
			} catch(std::exception &ex) {
				if (BString(ex.what()) != "CANCEL_CREDENTIALS") {
					OKAlert("OpenRemoteProject", BString("An error occurred while opening a remote project: ")
						<< ex.what(), B_INFO_ALERT);
					fStatusText->SetText("An error occurred!");
				}
				_SetIdle();
			}
			_ResetControls();
			break;
		}
		case kDoClone:
		{
			_SetBusy();
			fIsCloning = true;
			fClone->SetEnabled(false);
			fCancel->SetEnabled(true);
			fDestDir->SetEnabled(false);
			fPathBox->SetEnabled(false);
			fURL->SetEnabled(false);

			auto callback = [](const git_transfer_progress *stats, void *payload) -> int {
				int current_progress = stats->total_objects > 0 ?
					(100*stats->received_objects) /
					stats->total_objects :
					0;
				int kbytes = stats->received_bytes / 1024;

				BString progressString;
				progressString << "Cloning "
					<< stats->received_objects << "/"
					<< stats->total_objects << " objects"
					<< " (" << kbytes << " kb)";

				BMessage msg(kProgress);
				msg.AddString("progress_text", progressString);
				msg.AddFloat("progress_value", current_progress);
				BMessenger(this_handler).SendMessage(&msg);
				return 0;
			};

			// taken from TrackGit
/*			auto authentication_callback = [](git_cred** out, const char* url,
												const char* username_from_url,
												unsigned int allowed_types,
												void* payload) -> int {
				char username[39], password[128];
				int error = 0;

				// strcpy(username, username_from_url);

				GitCredentialsWindow* window = new GitCredentialsWindow(username, password);

				thread_id thread = window->Thread();
				status_t win_status = B_OK;
				wait_for_thread(thread, &win_status);

				if (strlen(username) != 0 && strlen(password) != 0) {
					if (allowed_types & GIT_CREDENTIAL_SSH_KEY) {
						error = git_credential_ssh_key_from_agent(out, username);
					} else {
						error = git_cred_userpass_plaintext_new(out, username, password);
					}
				}

				if (strlen(username) == 0)
					return CANCEL_CREDENTIALS;

				return error;
			};*/

			BPath fullPath(fPathBox->Path());
			fullPath.Append(fDestDir->Text());
			GitRepository repo(fullPath.Path());
			fCurrentTask = make_shared<Task<BPath>>
			(
				"GitClone",
				new BMessenger(this),
				std::bind
				(
					&GitRepository::Clone,
					&repo,
					fURL->Text(),
					fullPath,
					callback,
					&GitCredentialsWindow::authentication_callback
				)
			);

			_SetProgress(0, nullptr);
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
					_ResetControls();
					_SetProgress(100, "Canceled!");
				}
			} else {
				Quit();
			}
			break;
		}
		case kClose:
		{
			Quit();
			break;
		}
		case kProgress:
		{
			BString label;
			auto text = msg->GetString("progress_text");
			auto progress = msg->GetFloat("progress_value", 0);
			label << text << " " << (int32)progress << "%";
			if (LockLooper()) {
				if (progress != 0)
					_SetProgress(progress, label);
				UnlockLooper();
			}
			break;
		}
		case kUrlModified:
		{
			BString basedirPath = fPathBox->Path();
			auto repoName = _ExtractRepositoryName(fURL->Text());
			fDestDir->SetText(repoName);

			BeDC("Genio").SendMessage(repoName);
			break;
		}
		default:
			BWindow::MessageReceived(msg);
			break;
	}
}


BString
RemoteProjectWindow::_ExtractRepositoryName(BString url)
{
	BeDC dc("Genio");
	BString repoName = "";
	std::string surl = url.String();
	std::string strPattern = "^(https|git)(:\\/\\/|@)([^\\/:]+)[\\/:]([^\\/:]+)\\/(.+)(.git)?*$";
	std::smatch matches;
	std::regex rgx(strPattern);

	if(std::regex_search(surl, matches, rgx)) {
		dc.SendMessage("Match found\n");
		for (size_t i = 0; i < matches.size(); ++i) {
			dc.SendFormat("%d: '%s'", i, matches[i].str().c_str());
		}
		repoName.SetToFormat("%s", matches[5].str().c_str());
		repoName.RemoveAll(".git");
		BeDC dc(repoName);
	} else {
		dc.SendMessage("Match not found\n");
		repoName = "";
	}
	return repoName;
}


void
RemoteProjectWindow::_SetBusy()
{
	fBarberPole->Start();
	if (fProgressLayout->VisibleIndex() != VIEW_INDEX_BARBER_POLE)
		fProgressLayout->SetVisibleItem(VIEW_INDEX_BARBER_POLE);
}


void
RemoteProjectWindow::_SetIdle()
{
	fBarberPole->Stop();
	fProgressLayout->SetVisibleItem(VIEW_INDEX_BARBER_POLE);
}


void
RemoteProjectWindow::_SetProgress(float value, const char* text)
{
	fProgressBar->SetTo(value);
	fStatusText->SetText(text);
	if (fProgressLayout->VisibleIndex() != VIEW_INDEX_PROGRESS_BAR)
		fProgressLayout->SetVisibleItem(VIEW_INDEX_PROGRESS_BAR);
}
