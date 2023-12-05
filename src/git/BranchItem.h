/*
 * Copyright 2018, Your Name <your@email.address>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef _H
#define _H


#include "StyledItem.h"


class BranchItem : public StyledItem {
public:
	BranchItem(const char* text,
				uint32 branchType = -1,
				uint32 outlineLevel = 0,
				bool expanded = true,
				const char *iconName = nullptr);
	virtual ~BranchItem();
	uint32 BranchType() const;
private:
	uint32 fBranchType;
};


#endif // _H
