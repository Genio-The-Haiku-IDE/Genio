/*
 * Copyright 2025, Stefano Ceccherini
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once

#include "OptionList.h"

namespace Genio::UI {

class ProjectMenuField : public OptionList<BString> {
public:
	ProjectMenuField(const char* name, int32 what,
		uint32 flags = B_WILL_DRAW | B_NAVIGABLE);

	void AttachedToWindow() override;
	void DetachedFromWindow() override;
	void MessageReceived(BMessage* message) override;

private:
	void _HandleProjectListChanged(const BMessage* message);
	void _HandleActiveProjectChanged(const BMessage* message);

	int32 fWhat;
};

};

