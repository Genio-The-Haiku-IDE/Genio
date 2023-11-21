/*
 * Copyright 2023 Nexus6 <nexus6.haiku@icloud.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "RemoteProjectWindow.h"

#include <cstdio>
#include <functional>
#include <regex>

#include <git2.h>

#include <BarberPole.h>
#include <Button.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <FilePanel.h>
#include <LayoutBuilder.h>
#include <SeparatorView.h>
#include <StatusBar.h>
#include <StringView.h>
#include <Url.h>
#include <Window.h>

#include "GException.h"
#include "GenioWindowMessages.h"
#include "GitCredentialsWindow.h"
#include "GitRepository.h"
#include "Utils.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "RemoteProjectWindow"

BHandler *this_handler = nullptr;

#define VIEW_INDEX_BARBER_POLE	(int32) 0
#define VIEW_INDEX_PROGRESS_BAR	(int32) 1
#define VIEW_INDEX_CLONE_BUTTON	(int32) 0
#define VIEW_INDEX_CLOSE_BUTTON	(int32) 1

const int32 kOpenFilePanel = 'opFP';
const int32 kPathChosen = 'paCH';

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
	fURL(nullptr),
	fPathBox(nullptr),
	fBrowseButton(nullptr),
	fIsCloning(false),
	fTarget(target),
	fClone(nullptr),
	fCancel(nullptr),
	fProgressBar(nullptr),
	fBarberPole(nullptr),
	fProgressLayout(nullptr),
	fProgressView(nullptr),
	fButtonsView(nullptr),
	fStatusText(nullptr),
	fDestDirLabel(nullptr),
	fDestDir(nullptr),
	fFilePanel(nullptr)
{
	fURL = new BTextControl(B_TRANSLATE("URL:"), "", NULL);
	fPathBox = new BTextControl(B_TRANSLATE("Base path:"), dirPath.String(), nullptr);
	fBrowseButton = new BButton("browse", B_TRANSLATE("Browse" B_UTF8_ELLIPSIS),
		new BMessage(kOpenFilePanel));
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
	fBrowseButton->SetTarget(this);

	// TODO: We should use a Grid instead, to align the controls more nicely
	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.SetInsets(10)
		.AddGlue()
		.Add(fURL)
		.AddGroup(B_HORIZONTAL)
			.Add(fPathBox)
			.Add(fBrowseButton)
		.End()
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
RemoteProjectWindow::_OpenProject(const BString& localPath)
{
	// open the project folder
	BMessage *msg = new BMessage(MSG_PROJECT_FOLDER_OPEN);
	entry_ref ref;
	BEntry entry(localPath.String());
	if (entry.GetRef(&ref) == B_OK) {
		msg->AddRef("refs",&ref);
		fTarget.SendMessage(msg);
	} else {
		throw GException(B_ERROR, B_TRANSLATE("The local path is not valid or does not exist"));
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
					(100 * stats->received_objects) /
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

			BPath fullPath(fPathBox->Text());
			fullPath.Append(fDestDir->Text());
			Genio::Git::GitRepository repo(fullPath.Path());
			fCurrentTask = make_shared<Task<BPath>>
			(
				"GitClone",
				BMessenger(this),
				std::bind
				(
					&Genio::Git::GitRepository::Clone,
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
				BAlert* alert = new BAlert("OpenRemoteProject",
					B_TRANSLATE("Do you want to stop cloning the repository?"),
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
			BString basedirPath = fPathBox->Text();
			auto repoName = _ExtractRepositoryName(fURL->Text());
			fDestDir->SetText(repoName);
			LogInfo(repoName);
			break;
		}
		case kOpenFilePanel:
		{
			BEntry entry(fPathBox->Text());
			entry_ref ref;
			if (entry.GetRef(&ref) != B_OK)
				break;
			BMessenger messenger(this);
			if (fFilePanel == nullptr) {
				fFilePanel = new BFilePanel(B_OPEN_PANEL, &messenger, &ref,
					B_DIRECTORY_NODE | B_SYMLINK_NODE, false, new BMessage(kPathChosen),
					NULL, false, true);
			}
			fFilePanel->Show();
			break;
		}
		case kPathChosen:
		{
			entry_ref ref;
			if (msg->FindRef("refs", &ref) == B_OK) {
				BPath path(&ref);
				fPathBox->SetText(path.Path());
			}
		}
		// fall through:
		case B_CANCEL:
			// Clicked cancel on filepanel
			delete fFilePanel;
			fFilePanel = nullptr;
			break;
		default:
			BWindow::MessageReceived(msg);
			break;
	}
}


BString
RemoteProjectWindow::_ExtractRepositoryName(const BString& url)
{
	BString repoName = "";
	std::string surl = url.String();
	std::string strPattern = "^(https|git)(:\\/\\/|@)([^\\/:]+)[\\/:]([^\\/:]+)\\/(.+)(.git)?*$";
	std::smatch matches;
	std::regex rgx(strPattern);

	if(std::regex_search(surl, matches, rgx)) {
		LogInfo("Match found\n");
		for (size_t i = 0; i < matches.size(); ++i) {
			LogInfo("%d: '%s'", i, matches[i].str().c_str());
		}
		repoName.SetToFormat("%s", matches[5].str().c_str());
		repoName.RemoveAll(".git");
	} else {
		LogInfo("Match not found\n");
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
