/*
 * Copyright 2014-2017 Kacper Kasper 
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef GOTOLINEWINDOW_H
#define GOTOLINEWINDOW_H


#include <Window.h>


class BButton;
class BTextControl;


enum {
	GTLW_CANCEL				= 'gtlc',
	GTLW_GO					= 'gtlg'
};


class GoToLineWindow : public BWindow {
public:
							GoToLineWindow(BWindow* owner);

			void			MessageReceived(BMessage* message);
			void			ShowCentered(BRect ownerRect);
			void			WindowActivated(bool active);

private:
			BTextControl*	fLine;
			BButton*		fGo;
			BButton*		fCancel;

			BWindow*		fOwner;
};


#endif // GOTOLINEWINDOW_H
