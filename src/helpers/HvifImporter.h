/*
 * Copyright 2024, The Genio team
 * All rights reserved. Distributed under the terms of the MIT license.
 * Author: Nexus6 
 */
#pragma once

#include <FilePanel.h>
#include <SupportDefs.h>

class HvifFilter : public BRefFilter {
public:
            HvifFilter();
            ~HvifFilter();
	bool	Filter(const entry_ref* ref, BNode* node, struct stat_beos* stat, const char* filetype);
};


BString HvifImporter(entry_ref &ref);
