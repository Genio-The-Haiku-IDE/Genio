#include "ConsoleIOTabView.h"

#include <AutoDeleter.h>
#include <Button.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <LayoutBuilder.h>
#include <ScrollView.h>
#include <String.h>

#include "ConsoleIOTab.h"
#include "ConsoleIOThread.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ConsoleIOView"

enum {
	MSG_CLEAR_OUTPUT	= 'clou',
	MSG_STOP_PROCESS	= 'stpr',
};

ConsoleIOTabView::ConsoleIOTabView(const BString& name, const BMessenger& target)
	: BGroupView(B_VERTICAL, 0.0f), fWindowTarget(target)
{
	SetName(name);

	try {
		_Init(BMessenger(this));
	} catch (...) {
		throw;
	}

	SetFlags(Flags() | B_PULSE_NEEDED);
}


ConsoleIOTabView::~ConsoleIOTabView()
{
 	fConsoleIOTab->Stop();
}



status_t
ConsoleIOTabView::RunCommand(BMessage* cmd_message)
{
	fStopButton->SetEnabled(true);
	return fConsoleIOTab->RunCommand(cmd_message);
}



void
ConsoleIOTabView::MessageReceived(BMessage* message)
{
	switch (message->what) {

		case CONSOLEIOTHREAD_EXIT:
			fStopButton->SetEnabled(false);

			if (message->GetBool("internalStop", false) == false) {
				fWindowTarget.SendMessage(message);
			}
		break;
		case MSG_STOP_PROCESS:
		{
			fConsoleIOTab->Stop();
		}
		break;
		case MSG_CLEAR_OUTPUT:
		{
			Clear();
			break;
		}
		default:
			BGroupView::MessageReceived(message);
			break;
	}
}


void
ConsoleIOTabView::AttachedToWindow()
{
	BGroupView::AttachedToWindow();

	fClearButton->SetTarget(this);
	fStopButton->SetTarget(this);
	fStopButton->SetEnabled(false);

	fConsoleIOTab->SetMessenger(BMessenger(this));
}


void
ConsoleIOTabView::Clear()
{
	fConsoleIOTab->Clear();
}


void
ConsoleIOTabView::_Init(const BMessenger& target)
{
	fConsoleIOTab = new ConsoleIOTab("nome", target);

	fClearButton = new BButton(B_TRANSLATE("Clear"), new BMessage(MSG_CLEAR_OUTPUT));
	fStopButton  = new BButton(B_TRANSLATE("Stop"), new BMessage(MSG_STOP_PROCESS));

	BLayoutBuilder::Group<>(this, B_HORIZONTAL, 0.0f)
		.Add(fConsoleIOTab, 3.0f)
		.AddGroup(B_VERTICAL, 0.0f)
			.SetInsets(B_USE_SMALL_SPACING)
			.AddGlue()
			.Add(fClearButton)
			.Add(fStopButton)
		.End()
	.End();
}
