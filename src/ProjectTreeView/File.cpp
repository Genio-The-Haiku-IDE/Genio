/*
 * Original code taken from Seeker. Copyright by DarkWyrm
 * Copyright 2023 Nexus6 (nexus6.haiku@icloud.com)
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <stdio.h>
#include <string.h>
#include <strings.h>

#include <Entry.h>
#include <Directory.h>
#include <Node.h>
#include <NodeInfo.h>
#include <Path.h>
#include <Alert.h>
#include <String.h>

// #include "PrintDebugTools.h"
#include "File.h"

File::File(const File *file)
{
	Init();
	iEntryRef=file->iEntryRef;

	iTargetType=file->iTargetType;
	iIsDirectory=file->iIsDirectory;
	iIsLink=file->iIsLink;
	iSize=file->iSize;
	iIsVolume=file->iIsVolume;
	
	if(file->iVolume)
		iVolume=new BVolume(*file->iVolume);
	
}

File::File(const entry_ref *entryRef)
{
	Init();
	
	iEntryRef=*entryRef;
	BEntry entry(entryRef);
	
	if(entry.IsSymLink())
	{
		BEntry tgtentry(entryRef,true);
		iTargetType=(tgtentry.IsDirectory())?TYPE_DIRECTORY:TYPE_FILE;
		iIsLink=true;
	}
	else
		if(entry.IsDirectory())
		{
			iTargetType=TYPE_DIRECTORY;
			iIsDirectory=true;
		}
		else
			iTargetType=TYPE_FILE;
	
}

File::File(const BEntry *entry)
{
	Init();
	
	entry->GetRef(&iEntryRef); 
	if(entry->IsSymLink())
	{
		BEntry tgtentry(&iEntryRef,true);
		iTargetType=(tgtentry.IsDirectory())?TYPE_DIRECTORY:TYPE_FILE;
		iIsLink=true;
	}
	else
		if(entry->IsDirectory())
		{
			iTargetType=TYPE_DIRECTORY;
			iIsDirectory=true;
		}
		else
			iTargetType=TYPE_FILE;
}

File::File(const BVolume *volume)
{
	Init();

	iIsVolume=true;
	iVolume=new BVolume(*volume);

	BDirectory directory;
	iVolume->GetRootDirectory(&directory);
   	
	BEntry entry;
   	directory.GetEntry(&entry);
	entry.GetRef(&iEntryRef);
	iTargetType=TYPE_VOLUME;
}

File::~File()
{
	if(iPath) delete iPath;
	if(iParent) delete iParent;
	if(iStat) delete iStat;
	if(iVolume) delete iVolume;
	if(iSmallIcon) delete iSmallIcon;
	if(iLargeIcon) delete iLargeIcon;
}

void File::Init(void)
{
	iIsDirectory=false;
	iIsLink=false;
	iPath=NULL;
	iParent=NULL;
	iStat=NULL;
	iSize=-1LL;
	strcpy(iMimeDesc, "");
	
	iIsVolume=false;
	iVolume=NULL;
	
	iSmallIcon=NULL;
	iLargeIcon=NULL;
}

bool File::IsDirectory() const
{
	return iIsDirectory;
}

bool File::IsVolume() const
{
	return (iIsVolume && iVolume)?true:false;
}

bool File::IsLink() const
{
//	BEntry entry(&iEntryRef);
//	return entry.IsSymLink();

	return iIsLink;
}

bool File::IsDesktop()
{
	return (strcasecmp(PathDesc(), DESKTOP_ENTRY)==0)?true:false;
}

bool File::IsTrash()
{
	return (strcasecmp(PathDesc(), TRASH_ENTRY)==0)?true:false;
}

bool File::IsSystem()
{
	BString str(PathDesc());
	bool value=( str.Compare(DESKTOP_ENTRY)==0 ||
				str.Compare(TRASH_ENTRY)==0 ||
				str.Compare(ROOT_ENTRY)==0 ||
				str.Compare("/boot")==0 ||
				str.Compare("/boot/home")==0 ||
				str.FindFirst("/boot/beos")!=B_ERROR ||
				str.FindFirst("/boot/preferences")!=B_ERROR	);
	return value;
}

uint8 File::TargetType()
{
	// This call differs from the others in the sense that it returns
	// the file's type if not a symlink and if it *is* a symlink, returns
	// the kind for its target
/*	BEntry entry(&iEntryRef,true);
	if(entry.IsDirectory())
		return TYPE_DIRECTORY;
	return TYPE_FILE;
*/
	return iTargetType;
}

bool File::TargetExists() const
{
	if(!IsLink())
		return true;
	
	BEntry entry(&iEntryRef,true);
	return entry.Exists();
}

bool File::Exists() const
{
	BEntry entry(&iEntryRef);
	return entry.Exists();
}

BVolume File::Volume(void)
{
	if(iVolume)
		return(*iVolume);
	
	return BVolume(iEntryRef.device);
}

BBitmap* File::SmallIcon()
{
	if(!iSmallIcon) 
	{
		BRect iconBounds(0.0, 0.0, 15.0, 15.0);
		iSmallIcon=new BBitmap(iconBounds, B_RGBA32);	

		// We create a BEntry first because if file is a symlink,
		// then we want the link's target's icon
		if(IsVolume())
			iVolume->GetIcon(iSmallIcon,B_MINI_ICON);
		else
		if(IsLink() && !TargetExists())
		{
			BMimeType mime("application/x-vnd.Be-symlink");
			mime.GetIcon(iSmallIcon,B_MINI_ICON);
		}
		else
		{
			BEntry entry(&iEntryRef,true);
			BNode node(&entry);
			BNodeInfo nodeInfo(&node);
			nodeInfo.GetTrackerIcon(iSmallIcon, B_MINI_ICON);
		}
	}

	return iSmallIcon;
}

BBitmap* File::LargeIcon()
{
	if(!iLargeIcon)
	{
		BRect iconBounds(0.0, 0.0, 31.0, 31.0);
		iLargeIcon=new BBitmap(iconBounds, B_RGBA32);	

		// We create a BEntry first because if file is a symlink,
		// then we want the link's target's icon
		if(IsVolume())
			iVolume->GetIcon(iLargeIcon,B_LARGE_ICON);
		else
		if(IsLink() && !TargetExists())
		{
			BMimeType mime("application/x-vnd.Be-symlink");
			mime.GetIcon(iLargeIcon,B_LARGE_ICON);
		}
		else
		{
			BEntry entry(&iEntryRef,true);
			BNode node(&entry);
			BNodeInfo nodeInfo(&node);
			nodeInfo.GetTrackerIcon(iLargeIcon, B_LARGE_ICON);
		}
	}
	
	return iLargeIcon;
}

void File::ResetIcons()
{
	if(iSmallIcon)
	{
		delete iSmallIcon;
		iSmallIcon=NULL;
	}
		
	if(iLargeIcon)
	{
		delete iLargeIcon;
		iLargeIcon=NULL;
	}
}

const char* File::Name() const
{
	return iEntryRef.name;
}

off_t File::Size(void)
{
	if(iSize==-1LL)
	{
		BFile file;
		file.SetTo(&iEntryRef, B_READ_ONLY);
		file.GetSize(&iSize);
	}

	return iSize;
}

static void SizeToDesc(off_t size, char *strg)
{
	float tsize;

	// less than 1KB ?
	if(size < 1024LL) {
		if(size==1LL) {
			sprintf(strg, "%ld byte", size);
		} else {
			sprintf(strg, "%ld bytes", size);
		}		

	// less than 1MB ?
	} else if(size < (off_t) (1024LL * 1024LL)) {

		tsize=size;
		tsize /=1024.0;
		sprintf(strg, "%0.1f KB", tsize);		
	
	// less than 1GB ?
	} else if(size < (off_t) (1024LL * 1024LL * 1024LL)) {

		tsize=size;
		tsize /=1024.0 * 1024.0;
		sprintf(strg, "%0.1f MB", tsize);		

	// more than 1GB ?
	} else if(size > (off_t) (1024LL * 1024LL * 1024LL)) {

		tsize=size;
		tsize /=1024.0 * 1024.0 * 1024.0;
		sprintf(strg, "%0.2f GB", tsize);		
	}
}

void File::SizeDesc(char *strg)
{
	off_t size=Size();
	
	if(!IsDirectory() && !IsLink())
		SizeToDesc(size, strg);
	else
		strcpy(strg, "-");
}

// get capacity (volumes only)
off_t File::Capacity(void)
{
	if(IsVolume())
		return iVolume->Capacity();
	else
		return 0LL;
}

void File::CapacityDesc(char *strg)
{
	if(IsVolume())
		SizeToDesc(Capacity(), strg);
	else
		strcpy(strg, "");
}
		
// get free space (volumes only)
off_t File::FreeSpace(void)
{
	if(IsVolume())
		return iVolume->FreeBytes();
	else
		return 0LL;
}

void File::FreeSpaceDesc(char *strg)
{
	if(IsVolume())
		SizeToDesc(FreeSpace(), strg);
	else
		strcpy(strg, "");
}

// get used space (volumes only)
off_t File::UsedSpace(void)
{
	if(IsVolume())
		return (iVolume->Capacity() - iVolume->FreeBytes());
	else
		return 0LL;
}

void File::UsedSpaceDesc(char *strg)
{
	if(IsVolume())
		SizeToDesc(UsedSpace(), strg);
	else
		strcpy(strg, "");
}

struct stat File::Stat()
{
	if(!iStat)
	{
		BEntry entry(&iEntryRef);
		iStat=new struct stat;		
		
		entry.GetStat(iStat);	
	}

	return *iStat;
}
// convert time to string
static void TimeToDisplayString(struct tm atime, char *strg)
{
#ifdef UNUSED
	char *dayName[]={	"Monday",
						"Tuesday",
						"Wednesday",
						"Thursday",
						"Friday",
						"Saturday",
						"Sunday" };
#endif
	
	char *shrtDayName[]={ "Mon",
							"Tue",
							"Wed",
							"Thu",
							"Fri",
							"Sat",
							"Sun"  };
							
#ifdef UNUSED
	char *monthName[]={ "January",
						  "February",
						  "March",
						  "April",
						  "May",
						  "June",
						  "July",
						  "August",
						  "September",
						  "October",
						  "November",
						  "December"   };
#endif
						 
	char *shrtMonthName[]={ "Jan",
							  "Feb",
							  "Mar",
							  "Apr",
							  "May",
							  "Jun",
							  "Jul",
							  "Aug",
							  "Sep",
							  "Oct",
							  "Nov",
							  "Dec"  };
							  
	char *amPmStrg[]={ "AM", "PM" };
	
	int amPmIndx=(atime.tm_hour > 12);							  
	sprintf(strg, "%s, %s %02d %04d, %02d:%02d %s",
				   shrtDayName[atime.tm_wday], shrtMonthName[atime.tm_mon],
				   atime.tm_mday, 1900 + atime.tm_year, atime.tm_hour % 12, atime.tm_min, 
				   amPmStrg[amPmIndx]);
}

time_t File::ModifiedTime()
{
	return Stat().st_mtime;
}

struct tm File::ModifiedTm()
{
	time_t time = ModifiedTime();
	return *localtime(&time);
}

void File::ModifiedTmDesc(char *strg)
{
	TimeToDisplayString(ModifiedTm(), strg);
}

time_t File::CreatedTime()
{
	return Stat().st_crtime;
}

struct tm File::CreatedTm()
{
	time_t time = CreatedTime();
	return *localtime(&time);
}

void File::CreatedTmDesc(char *strg)
{
	TimeToDisplayString(CreatedTm(), strg);
}

// get file type (MIME short description)
const char* File::MimeDesc(bool resolve_links)
{
	char mimeStrg[B_MIME_TYPE_LENGTH];
	if(strlen(iMimeDesc)==0 || (resolve_links && IsLink()))
	{
		BEntry entry(&iEntryRef,resolve_links);

		File target(&entry);
		
		if(target.IsVolume())
			return("Be Volume");
		else
		if(target.IsDirectory())
			return("Folder");
		else
		if(target.IsLink())
			return("Symbolic Link");
		else
		{
			BNode node(&entry);
			BNodeInfo nodeInfo(&node);
			
			nodeInfo.GetType(mimeStrg);
	
			BMimeType mimeType(mimeStrg);
			if(mimeType.GetShortDescription(iMimeDesc)==B_BAD_VALUE)
				sprintf(iMimeDesc,"Generic File");
			
			if(strlen(iMimeDesc)==0 && resolve_links==false)
				strcpy(iMimeDesc, mimeStrg);
		}
	}	

	return (resolve_links && IsLink())?mimeStrg:iMimeDesc;
}

const BPath* File::Path()
{
	if(!iPath)
	{
		BEntry entry(&iEntryRef);
		iPath=new BPath(&entry);
	}
	
	return iPath;
}

const char* File::PathDesc()
{
	return Path()->Path();
}

const entry_ref* File::Ref() const
{
	return &iEntryRef;
}

// get parent
const File* File::Parent()
{
	if(!iParent) 
	{
		BEntry entry(&iEntryRef);
		BEntry parent;
		if(entry.GetParent(&parent)==B_OK)
			iParent=new File(&parent);
	}
	
	return iParent;
}

// get distance from root measured in number of folders
int32 File::Level() const
{
	BEntry entry(&iEntryRef);
	BPath *path=new BPath(&entry);
	
	char pathDesc[B_PATH_NAME_LENGTH];
	if(path->Path()) {	
		strcpy(	pathDesc, path->Path());
	} else {
		strcpy(pathDesc, "");
	}
	delete path;
	
	int32 level=0;
	for (uint32 i=0; i < strlen(pathDesc); i++) {
		if(pathDesc[i]=='/') {
			level +=1;
		}
	}

	return level;
}

void File::LeafFromPath(char *path, char *leaf)
{
	strcpy(leaf, "");
	
	// skip the trailing path separator
	for (int32 i=strlen(path) - 1; i >=0; i--)
	{
		if(path[i]=='/')
		{
			strcpy(leaf, &path[i+1]);
			break;
		}
	} 
}


