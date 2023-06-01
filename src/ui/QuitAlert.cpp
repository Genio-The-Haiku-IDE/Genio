/*
 * Copyright 2016-2018 Kacper Kasper <kacperkasper@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "QuitAlert.h"

#include <Application.h>
#include <Button.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <LayoutBuilder.h>
#include <ScrollView.h>
#include <StringView.h>

#include <string>
#include <vector>

#include "Utils.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "QuitAlert"


namespace {
const int kSemTimeOut = 50000;
const uint32 kMaxItems = 4;
}


QuitAlert::QuitAlert(const std::vector<std::string> &unsavedFiles)
	:
	BWindow(BRect(100, 100, 200, 200), B_TRANSLATE("Unsaved files"), B_MODAL_WINDOW,
		B_NOT_CLOSABLE | B_NOT_ZOOMABLE | B_NOT_RESIZABLE | B_AUTO_UPDATE_SIZE_LIMITS, 0),
	fUnsavedFiles(unsavedFiles),
	fCheckboxes(unsavedFiles.size(), nullptr),
	fAlertValue(0)
{
	_InitInterface();
	CenterOnScreen();
}


QuitAlert::~QuitAlert()
{
	if (fAlertSem >= B_OK)
		delete_sem(fAlertSem);
}


void
QuitAlert::_InitInterface()
{
	fMessageString = new BStringView("message", B_TRANSLATE("There are unsaved changes.\nSelect the files to save."));
	fSaveAll = new BButton(B_TRANSLATE("Save all"), new BMessage((uint32) Actions::SAVE_ALL));
	fSaveSelected = new BButton(B_TRANSLATE("Save selected"), new BMessage((uint32) Actions::SAVE_SELECTED));
	fDontSave = new BButton(B_TRANSLATE("Don't save"), new BMessage((uint32) Actions::DONT_SAVE));
	fCancel = new BButton(B_TRANSLATE("Cancel"), new BMessage(B_QUIT_REQUESTED));
	fCancel->MakeDefault(true);
	BGroupView* filesView = new BGroupView(B_VERTICAL, 0);
	filesView->SetViewUIColor(B_CONTROL_BACKGROUND_COLOR);
	fScrollView = new BScrollView("files", filesView, 0, false, true);
	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.Add(fMessageString)
		.Add(fScrollView)
		.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
			.Add(fSaveAll)
			.Add(fSaveSelected)
			.Add(fDontSave)
			.AddGlue()
			.Add(fCancel)
		.End()
		.SetInsets(B_USE_SMALL_INSETS);

	font_height fh;
	be_plain_font->GetHeight(&fh);
	float textHeight = fh.ascent + fh.descent + fh.leading + 5;
	fScrollView->SetExplicitSize(BSize(B_SIZE_UNSET,
		textHeight * std::min<uint32>(fUnsavedFiles.size(), kMaxItems) + 25.0f));
	BScrollBar* bar = fScrollView->ScrollBar(B_VERTICAL);
	bar->SetSteps(textHeight / 2.0f, textHeight * 3.0f / 2.0f);
	bar->SetRange(0.0f, fUnsavedFiles.size() > kMaxItems ?
		(textHeight + 3.0f) * (fUnsavedFiles.size() - kMaxItems) : 0.0f);

	BGroupLayout* files = filesView->GroupLayout();
	files->SetInsets(B_USE_SMALL_INSETS);
	for(size_t i = 0; i < fUnsavedFiles.size(); ++i) {
		fCheckboxes[i] = new BCheckBox("file", fUnsavedFiles[i].c_str(), new BMessage((uint32) i));
		SetChecked(fCheckboxes[i]);
		files->AddView(fCheckboxes[i]);
	}
}


void
QuitAlert::Show()
{
	BWindow::Show();
	fScrollView->SetExplicitSize(BSize(Bounds().Width(), B_SIZE_UNSET));
}


// borrowed from BAlert
std::vector<bool>
QuitAlert::Go()
{
	fAlertSem = create_sem(0, "AlertSem");
	if (fAlertSem < 0) {
		Quit();
		return std::vector<bool>(0);
	}

	// Get the originating window, if it exists
	BWindow* window = dynamic_cast<BWindow*>(
		BLooper::LooperForThread(find_thread(nullptr)));

	Show();

	if (window != nullptr) {
		status_t status;
		for (;;) {
			do {
				status = acquire_sem_etc(fAlertSem, 1, B_RELATIVE_TIMEOUT,
					kSemTimeOut);
				// We've (probably) had our time slice taken away from us
			} while (status == B_INTERRUPTED);

			if (status == B_BAD_SEM_ID) {
				// Semaphore was finally nuked in MessageReceived
				break;
			}
			window->UpdateIfNeeded();
		}
	} else {
		// No window to update, so just hang out until we're done.
		while (acquire_sem(fAlertSem) == B_INTERRUPTED) {
		}
	}

	// Have to cache the value since we delete on Quit()
	auto value = fAlertValue;
	if (Lock())
		Quit();

	return value;
}


void
QuitAlert::MessageReceived(BMessage* message)
{
	switch(message->what) {
		case Actions::SAVE_ALL: {
			fAlertValue = std::vector<bool>(fUnsavedFiles.size(), true);
		} break;
		case Actions::SAVE_SELECTED: {
			fAlertValue = std::vector<bool>(fUnsavedFiles.size(), false);
			for(uint32 i = 0; i < fCheckboxes.size(); ++i) {
				fAlertValue[i] = fCheckboxes[i]->Value() ? true : false;
			}
		} break;
		case Actions::DONT_SAVE: {
			fAlertValue = std::vector<bool>(fUnsavedFiles.size(), false);
		} break;
		default: {
			BWindow::MessageReceived(message);
		} return;
	}
	delete_sem(fAlertSem);
	fAlertSem = -1;
}
