/*
 * Copyright 2023-2024, The Genio team
 * All rights reserved. Distributed under the terms of the MIT license.
 * Author: Nexus6 <nexus6.haiku@icloud.com>
 */
 
#pragma once

#include <SupportDefs.h>

#include <filesystem>

#define FS_CLOBBER 'fscl'
#define FS_SKIP 'fssk'

class BEntry;

namespace fs = std::filesystem;
status_t FSCheckCopiable(BEntry *src, BEntry *dest);
status_t FSCopyFile(BEntry *src, BEntry *dest, bool clobber);
status_t FSMoveFile(BEntry *src, BEntry *dest, bool clobber);
status_t FSMakeWritable(const fs::path& path, bool recurse = false);
status_t FSDeleteFolder(BEntry *dirEntry);
