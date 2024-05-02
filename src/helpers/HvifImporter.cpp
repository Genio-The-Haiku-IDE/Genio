/*
 * Copyright 2024, The Genio team
 * All rights reserved. Distributed under the terms of the MIT license.
 * Author: Nexus6 
 */


#include "HvifImporter.h"
#include <String.h>


HvifFilter::HvifFilter()
{
}


HvifFilter::~HvifFilter()
{
}


bool
HvifFilter::Filter(const entry_ref* ref, BNode* node, struct stat_beos* stat,
						const char* filetype)
{
    if (strcmp("application/x-vnd.Be-directory", filetype) == 0) {
        return true;
	}
    if (strcmp("application/x-vnd.Be-symlink", filetype) == 0) {
		BEntry entry(ref, true);
		if (entry.IsDirectory())
			return true;
	}
	if (strcmp("image/x-hvif", filetype) == 0) {
		return true;
	}
    BString name(ref->name);
    int32 dot = name.FindLast('.');
    if (dot != B_ERROR) {
        if (name.IFindFirst(".hvif", dot) != B_ERROR)
            return true;
    }
    return false;
}


BString HvifImporter(entry_ref &ref)
{
	return "";
}

