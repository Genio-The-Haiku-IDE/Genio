/*
 * Copyright 2023, Genio
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "BranchItem.h"

BranchItem::BranchItem(const char* branchName,
						const char* text,
						uint32 branchType,
						uint32 outlineLevel,
						bool expanded,
						const char *iconName)
	:
	StyledItem(text, outlineLevel, expanded, iconName),
	fBranchType(branchType),
	fBranchName(branchName)
{
}


/* virtual */
BranchItem::~BranchItem()
{
}


uint32
BranchItem::BranchType() const
{
	return fBranchType;
}


const char*
BranchItem::BranchName() const
{
	return fBranchName.String();
}

