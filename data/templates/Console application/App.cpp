#include <Directory.h>
#include <Entry.h>
#include <Path.h>
#include <stdio.h>
#include <String.h>
#include <chrono>

// It's better to use constant global integers instead of #defines because constants
// provide strong typing and don't lead to weird errors like #defines can.
const uint16 BYTES_PER_KB = 1024;
const uint32 BYTES_PER_MB = 1048576;
const uint64 BYTES_PER_GB = 1099511627776ULL;

int		ListDirectory(const entry_ref &dirRef);
int		CountTotalEntries(const entry_ref &dirRef);
int		CountEntries(const entry_ref &dirRef);
BString	MakeSizeString(const uint64 &size);

int
main(int argc, char **argv)
{
	// We want to require one argument in addition to the program name when invoked
	// from the command line.
	if (argc == 1)
	{
		printf("Usage: listdir <path> [<path> ...]\n");
		return 0;
	}
	
	// Here we'll do some sanity checks to make sure that the path we were given
	// actually exists and it's not a file.
for (int i=1; i<argc; i++) {
	printf("------ %s ------\n",argv[i]);
	BEntry entry(argv[i]);
	if (!entry.Exists())
	{
		printf("%s does not exist\n",argv[i]);
		return 1;
	}
	
	if (!entry.IsDirectory())
	{
		printf("%s is not a directory\n",argv[i]);
		return 1;
	}
	
	// An entry_ref is a typedef'ed structure which points to a file, directory, or
	// symlink on disk. The entry must actually exist, but unlike a BFile or BEntry, it
	// doesn't use up a file handle.
	entry_ref ref;
	entry.GetRef(&ref);
	
	// std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
	// auto ret = ListDirectory(ref);
	// std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
	// auto elapsed1 = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
	// printf("ListDirectory(): Elapsed period: %ld microseconds\n", elapsed1);
	
	// CountEntries
	std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
	auto ret = CountEntries(ref);
	std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
	auto elapsed1 = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
	printf("CountEntries(): %d entries\n", ret);
	printf("CountEntries(): Elapsed period: %ld microseconds\n", elapsed1);
	
	// begin = std::chrono::steady_clock::now();
	// auto totalEntries = CountTotalEntries(ref);
	// end = std::chrono::steady_clock::now();
	// auto elapsed2 = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
	// printf("CountTotalEntries() %d entries\n",totalEntries);
	// printf("CountTotalEntries(): Elapsed period: %ld microseconds\n", elapsed2);
}
	return 0;
	
	 // haiku 1080590 microseconds
	 // genio 20458 microseconds
}

int
CountEntries(const entry_ref& ref)
{
	int count = 0;
	BEntry nextEntry;
	BEntry entry(&ref);	
				
	if (entry.IsDirectory())
	{
		entry_ref nextRef;
		BDirectory dir(&entry);
		count = dir.CountEntries();
		while(dir.GetNextEntry(&nextEntry)==B_OK)
		{
			nextEntry.GetRef(&nextRef);
			count += CountEntries(nextRef);
		}
	}
	// count++;
	return count;
}

int
ListDirectory(const entry_ref &dirRef)
{
	// This function does all the work of the program
	
	BDirectory dir(&dirRef);
	if (dir.InitCheck() != B_OK)
	{
		printf("Couldn't read directory %s\n",dirRef.name);
		return 1;
	}
	
	// First thing we'll do is quickly scan the directory to find the length of the
	// longest entry name. This makes it possible to left justify the file sizes
	int32 entryCount = 0;
	uint32 maxChars = 0;
	entry_ref ref;
	
	// Calling Rewind() moves the BDirectory's index to the beginning of the list.
	dir.Rewind();
	
	// GetNextRef() will return B_ERROR when the BDirectory has gotten to the end of
	// its list of entries.
	while (dir.GetNextRef(&ref) == B_OK)
	{
		if (ref.name)
			maxChars = MAX(maxChars,strlen(ref.name));
	}
	maxChars++;
	char padding[maxChars];
	
	BEntry entry;
	dir.Rewind();
	
	// Here we'll call GetNextEntry() instead of GetNextRef() because a BEntry will
	// enable us to get certain information about each entry, such as the entry's size.
	// Also, because it inherits from BStatable, we can differentiate between
	// directories and files with just one function call.
	while (dir.GetNextEntry(&entry) == B_OK)
	{
		char name[B_FILE_NAME_LENGTH];
		entry.GetName(name);
		
		BString formatString;
		formatString << "%s";
		
		unsigned int length = strlen(name);
		if (length < maxChars)
		{
			uint32 padLength = maxChars - length;
			memset(padding, ' ', padLength);
			padding[padLength - 1] = '\0';
			formatString << padding;
		}
		
		formatString << "\n";
		printf(formatString.String(),name);
		entryCount++;
		if (entry.IsDirectory())
		{
			// We'll display the "size" of a directory by listing how many
			// entries it contains
			BDirectory subdir(&entry);
			formatString << "\t" << subdir.CountEntries() << " items";
			entry_ref nextRef;
			entry.GetRef(&nextRef);
			auto ret = ListDirectory(nextRef);
		}
		else
		{
			off_t fileSize;
			entry.GetSize(&fileSize);
			formatString << "\t" << MakeSizeString(fileSize);
		}

	}
	printf("%ld entries\n",entryCount);
	return 0;
}

int
CountTotalEntries(const entry_ref &dirRef)
{
	// This function does all the work of the program
	
	BDirectory dir(&dirRef);
	if (dir.InitCheck() != B_OK)
	{
		return -1;
	}
	
	int32 entryCount = 0;
	entry_ref ref;
	
	BEntry entry;
	dir.Rewind();
	entryCount += dir.CountEntries();
	
	while (dir.GetNextEntry(&entry) == B_OK)
	{
		if (entry.IsDirectory())
		{
			BDirectory subdir(&entry);
			// entryCount += subdir.CountEntries();
			entry_ref nextRef;
			entry.GetRef(&nextRef);
			entryCount += CountTotalEntries(nextRef);
		}
	}
	return entryCount;
}


BString
MakeSizeString(const uint64 &size)
{
	// This function just makes converts the raw byte counts provided by
	// BEntry's GetSize() method into something more people-friendly.
	BString sizeString;
	if (size < BYTES_PER_KB)
		sizeString << size << " bytes";
	else if (size < BYTES_PER_MB)
		sizeString << (float(size) / float(BYTES_PER_KB)) << " KB";
	else if (size < BYTES_PER_GB)
		sizeString << (float(size) / float(BYTES_PER_MB)) << " MB";
	else
		sizeString << (float(size) / float(BYTES_PER_GB)) << " GB";
	return sizeString;
}

