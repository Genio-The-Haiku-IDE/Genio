/*
 * Copyright 2013-2024, Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once

#include <View.h>

class BarberPole;
class BMessageRunner;
class BStatusBar;
class BStringView;
class GlobalStatusView : public BView {
public:
	GlobalStatusView();

	virtual void AttachedToWindow();
	virtual void DetachedFromWindow();
	virtual void Draw(BRect updateRect);
	virtual void MessageReceived(BMessage *message);

private:
			void	_ResetRunner(BMessageRunner**  runner);
			void	_StartRunner(BMessageRunner** runner, uint32 what);

	BarberPole*		fBuildBarberPole;
	BStringView*	fBuildStringView;
	BStringView*	fLSPStringView;
	BStatusBar*		fLSPStatusBar;
	BStringView*	fLastFindStatus;
	bigtime_t		fLastStatusChange;
	BMessageRunner* fRunnerBuild;
	BMessageRunner* fRunnerFind;
};
