/*
 * Copyright 2013-2023, Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once

#include <View.h>

class BBitmap;
class BStatusBar;
class BStringView;
class SquareBitmapView;
class GlobalStatusView : public BView {
public:
	GlobalStatusView();

	virtual void AttachedToWindow();
	virtual void Draw(BRect updateRect);
	virtual void MessageReceived(BMessage *message);
	virtual void Pulse();

	virtual BSize MinSize();
	virtual BSize MaxSize();

private:
	BStatusBar*		fStatusBar;
	BStringView*	fStringView;
	bigtime_t		fLastStatusChange;
};
