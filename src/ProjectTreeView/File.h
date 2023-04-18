#ifndef _PFILE_H_
#define _PFILE_H_

#include <Volume.h>
#include <Bitmap.h>
#include <List.h>

#define DESKTOP_ENTRY	"/boot/home/Desktop"
#define TRASH_ENTRY		"/boot/home/Desktop/Trash"
#define ROOT_ENTRY		"/"

enum
{
TYPE_LINK=0,
TYPE_DIRECTORY,
TYPE_FILE,
TYPE_VOLUME
};

class File
{
	public:
		
		File(const File *file);
		File(const entry_ref *entryRef);
		File(const BEntry *entry);
		File(const BVolume *volume);

		~File();

		bool IsDirectory() const;
		bool IsVolume() const;
		bool IsDesktop();
		bool IsTrash();
		bool IsSystem();
		bool IsLink() const;
		bool Exists() const;
		bool TargetExists() const;
		uint8 TargetType();
		BVolume Volume(void);
		
		BBitmap* SmallIcon();
		BBitmap* LargeIcon();
		void ResetIcons();

		/* get name */
		const char* Name() const;
		
		/* get size */
		off_t Size(void);
		void SizeDesc(char *strg);
		void ResetSize() { iSize=-1LL; }

		/* get capacity (volumes only) */
		off_t Capacity(void);
		void CapacityDesc(char *strg);
		
		/* get free space (volumes only) */
		off_t FreeSpace(void);
		void FreeSpaceDesc(char *strg);

		/* get used space (volumes only) */
		off_t UsedSpace(void);
		void UsedSpaceDesc(char *strg);

		/* get stat structure */
		struct stat Stat();
		
		/* get modified date/time */
		time_t ModifiedTime();
		struct tm ModifiedTm();
		void ModifiedTmDesc(char *strg);

		/* get creation data/time */
		time_t CreatedTime();
		struct tm CreatedTm();
		void CreatedTmDesc(char *strg);
		
		/* get file type (MIME short description) */
		const char* MimeDesc(bool resolve_links=false);
		
		/* get path */
		const BPath* Path(void);
		const char* PathDesc();								

		/* get ref */
		const entry_ref* Ref() const;

		/* get parent */
		const File* Parent(); 
		
		/* get level (number of '/'s) */
		int32 Level() const;
		
		/* static functions **********/
		
		/* get leaf from the path */
		static void LeafFromPath(char *path, char *leaf);		

	protected:
		
		void Init(void);
		
		void InitStat(void);		
		
	private:
	
		entry_ref		iEntryRef;

		bool			iIsDirectory;
		bool			iIsLink;
		BPath*			iPath;
		File*			iParent;	
		struct stat	*	iStat;
		off_t 			iSize;	/* off_t==long long */
		char			iMimeDesc[B_MIME_TYPE_LENGTH];

		bool			iIsVolume;
		BVolume*		iVolume;
				
		BBitmap*		iSmallIcon;
		BBitmap* 		iLargeIcon;
		int8			iTargetType;
};

typedef BList	FileList;

#endif
