#ifndef FSUTILS_H_
#define FSUTILS_H_

#include <Errors.h>
#include <filesystem>

#define FS_CLOBBER 'fscl'
#define FS_SKIP 'fssk'

namespace fs = std::filesystem;
status_t FSCheckCopiable(BEntry *src, BEntry *dest);
status_t FSCopyFile(BEntry *src,BEntry *dest, bool clobber);
status_t FSMoveFile(BEntry *src,BEntry *dest, bool clobber);
status_t FSChmod(const fs::path& path, fs::perms perm, bool recurse = false);
status_t FSDeleteFolder(BEntry *dirEntry);

#endif