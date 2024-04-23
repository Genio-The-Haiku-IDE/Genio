/*
 * Copyright 2024, The Genio team
 * All rights reserved. Distributed under the terms of the MIT license.
 * Author: Nexus6 <nexus6.haiku@icloud.com>
 */


#include "HvifImporter.h"
#include <String.h>


HvifFilter::HvifFilter()
	:
	BRefFilter()
{
}


HvifFilter::~HvifFilter()
{
}


bool
HvifFilter::Filter(const entry_ref* ref, BNode* node, struct stat_beos* stat,
						const char* filetype)
{
	bool isHvif = false;
	if (BString(ref->name).IEndsWith(".rdef"))
		isHvif = true;
	return isHvif;
}


BString HvifImporter(entry_ref &ref)
{
	return "HVIF!";
}

