#ifndef FSUTILS_H_
#define FSUTILS_H_

#include <Errors.h>
#include <filesystem>

#define FS_CLOBBER 'fscl'
#define FS_SKIP 'fssk'

namespace fs = std::filesystem;
status_t CheckCopiable(BEntry *src, BEntry *dest);
status_t CopyFile(BEntry *src,BEntry *dest, bool clobber);
status_t MoveFile(BEntry *src,BEntry *dest, bool clobber);
void chmodr(const fs::path& path, fs::perms perm);
#endif