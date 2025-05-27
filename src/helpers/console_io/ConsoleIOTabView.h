#pragma once

#include <GroupView.h>
#include <Messenger.h>

class BButton;
class BCheckBox;
class BTextView;
class ConsoleIOTab;

class ConsoleIOTabView : public BGroupView {
public:
								ConsoleIOTabView(const BString& name, const BMessenger& target);
								~ConsoleIOTabView();

	virtual	void				MessageReceived(BMessage* message);
	virtual	void				AttachedToWindow();

			void				Clear();
			status_t			RunCommand(BMessage* cmd_message);

private:
			void				_Init(const BMessenger& target);

private:
			BMessenger			fWindowTarget;
			BButton*			fClearButton;
			BButton*			fStopButton;
			ConsoleIOTab*		fConsoleIOTab;
};
