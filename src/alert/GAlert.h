/*
 * Copyright 2023 Nexus6 <nexus6.haiku@icloud.com>
 * Parts taken from QuitAlert.cpp
 * Copyright 2016-2018 Kacper Kasper <kacperkasper@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#pragma once

#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <ScrollView.h>
#include <StringView.h>
#include <Window.h>

#include <vector>

#include "GMessage.h"

class BButton;
class BStringView;
class BScrollView;

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "GTextAlert"

enum GAlertButtons {
	OkButton,
	CancelButton
};

template <typename T>
struct GAlertResult {
	GAlertButtons Button;
	T Result;
};

template <typename T>
class GAlert : public BWindow {
public:
	GAlert(const char *title, const char *message)
		:
		BWindow(BRect(100, 100, 200, 200), title, B_MODAL_WINDOW,
			B_NOT_CLOSABLE | B_NOT_ZOOMABLE | B_NOT_RESIZABLE | B_AUTO_UPDATE_SIZE_LIMITS, 0),
		fOK(nullptr),
		fCancel(nullptr),
		fTitle(title),
		fMessage(message),
		fMessageString(nullptr),
		fScrollView(nullptr),
		fMainView(nullptr),
		fAlertSem(0)
	{
		_InitInterface();
		CenterOnScreen();
	}

	virtual ~GAlert() {}

	virtual void MessageReceived(BMessage* message)
	{
		switch (message->what) {
			case OkButton: {
				break;
			}
			case CancelButton: {
				break;
			}
			default: {
				BWindow::MessageReceived(message);
				return;
			}
		}
		delete_sem(fAlertSem);
		fAlertSem = -1;
	}


	GAlertResult<T>	 Go()
	{
		fAlertSem = create_sem(0, "AlertSem");
		if (fAlertSem < 0) {
			Quit();
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

		auto value = fResult;
		if (Lock())
			Quit();

		return value;
	}

protected:
	BButton*					fOK;
	BButton*					fCancel;
	GAlertResult<T>				fResult;

	virtual void Show()
	{
		BWindow::Show();
	}

	BView* GetPlaceholderView() { return fMainView; }

private:
	const int					kSemTimeOut = 50000;
	const uint32 				kMaxItems = 4;
	const BString				fTitle;
	const BString				fMessage;
	BStringView*				fMessageString;
	BScrollView*				fScrollView;
	BView* 						fMainView;
	sem_id						fAlertSem;

	void _InitInterface()
	{
		fMessageString = new BStringView("message", fMessage);
		fOK = new BButton(B_TRANSLATE("Ok"), new BMessage(OkButton));
		fCancel = new BButton(B_TRANSLATE("Cancel"), new BMessage(CancelButton));
		fOK->MakeDefault(true);
		fMainView = new BBox(B_NO_BORDER, nullptr);
		BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
			.Add(fMessageString)
			.Add(fMainView)
			.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
				.Add(fOK)
				.Add(fCancel)
			.End()
			.SetInsets(B_USE_SMALL_INSETS);
	}
};