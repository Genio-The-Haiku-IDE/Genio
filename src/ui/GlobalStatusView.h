/*
 * Copyright 2013-2024, Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once

#include <View.h>

class BarberPole;
class BStringView;
class BMessageRunner;

class GlobalStatusView : public BView {
public:
	GlobalStatusView();

	virtual void AttachedToWindow();
	virtual void DetachedFromWindow();
	virtual void Draw(BRect updateRect);
	virtual void MessageReceived(BMessage *message);


	virtual BSize MinSize();
	virtual BSize MaxSize();

private:
	BarberPole*		fBarberPole;
	BStringView*	fBuildStringView;
	BStringView*	fLSPStringView;
	bigtime_t		fLastStatusChange;
	BMessageRunner* fRunner;
};
