#ifndef FSUTILS_H_
#define FSUTILS_H_

#include <Errors.h>

#define FS_CLOBBER 'fscl'
#define FS_SKIP 'fssk'

status_t CheckCopiable(BEntry *src, BEntry *dest);
status_t CopyFile(BEntry *src,BEntry *dest, bool clobber);
status_t MoveFile(BEntry *src,BEntry *dest, bool clobber);
status_t DeleteFolder(BEntry *dirEntry);

#endif