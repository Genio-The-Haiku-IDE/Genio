#pragma once

#include <GroupView.h>
#include <Messenger.h>

class BButton;
class BCheckBox;
class BTextView;
class ConsoleIOTab;

class ConsoleIOTabView : public BGroupView {
public:
								ConsoleIOTabView(const BString& name, const BMessenger& target,
								                 const BString& theme);
								~ConsoleIOTabView();

	virtual	void				MessageReceived(BMessage* message);
	virtual	void				AttachedToWindow();

			void				Clear();
			void				SetTheme(BString theme);
			status_t			RunCommand(BMessage* cmd_message);

private:
			void				_Init(const BMessenger& target, const BString& theme);

private:
			BMessenger			fWindowTarget;
			BButton*			fClearButton;
			BButton*			fStopButton;
			ConsoleIOTab*		fConsoleIOTab;
};
