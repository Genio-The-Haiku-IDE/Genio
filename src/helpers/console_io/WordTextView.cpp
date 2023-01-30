/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "WordTextView.h"
#include <stdio.h>
#include <string>
#include <Array.h>

bool Contains(std::string const &s, char ch) noexcept {
	return s.find(ch) != std::string::npos;
}

constexpr bool IsASpace(int ch) noexcept {
	return (ch == ' ') || ((ch >= 0x09) && (ch <= 0x0d));
}

std::string kDefaultAdditionalWordCharacters(":@-+./_~");
std::string wordCharacters("1234567890_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");


bool
Classify(char c)
{
	if (IsASpace(c))
		return false;

	if (Contains(wordCharacters,c))
		return true;

	if (Contains(kDefaultAdditionalWordCharacters, c))
		return true;

	return false;
}

void	
WordTextView::_SetStyle(bool underline)
{
	int32 len  = 0;
	text_run_array * tra = BTextView::RunArray	(fStartPosition, fStopPosition, &len);
	
	if (len == 0)
		return;
		
	if (tra->count > 0) {
		tra->runs[0].font.SetFace( underline ? B_UNDERSCORE_FACE : B_REGULAR_FACE);
		SetRunArray(fStartPosition, fStopPosition + 1, tra);
	}
	
	free( tra );
}

void	
WordTextView::_Reset() 
{
	if (fStopPosition == -1 && fStartPosition == -1)
		return;
	
	_SetStyle(false);
}

void	
WordTextView::MouseMoved (BPoint where, uint32 code, const BMessage *dragMessage)
{
	BTextView::MouseMoved(where, code, dragMessage);
	
	
	int32 pointPosition = OffsetAt(where);
	//printf("%d %d %d\n", fStartPosition, pointPosition, fStopPosition);
	if (pointPosition >= fStartPosition && pointPosition <= fStopPosition)
		return;

	int32 startPosition = -1;
	int32 stopPosition = -1;
	BString word = "";
	
	_Reset();
	bool found = _GetHyperLinkAt(where, pointPosition, word, startPosition, stopPosition);
	
	fStopPosition  = stopPosition;
	fStartPosition = startPosition;
	fLastWord = word;
	
	if (found)
		OnWord(fLastWord);
	
}

void	
WordTextView::OnWord(BString& newWord)
{
	//printf("New word: %s \n", newWord.String());
	_SetStyle(true);
	
}

bool
WordTextView::_GetHyperLinkAt(BPoint where, const int32 pointPosition, BString& word, int32& startPosition, int32& stopPosition)
{
	int32 line = LineAt(where);
	if (line >= CountLines())
		return false;
	
	int32 startLine 	= OffsetAt(line);
	int32 maxPosition 	= TextLength() - 1;
		
	startPosition = pointPosition;
	stopPosition  = pointPosition;
	
	//backward:
	for(;;) {
		if (Classify(ByteAt(startPosition))) {
			if (startLine == startPosition){
				break;
			}
			startPosition--;
		} else {
			if (startPosition < maxPosition)
				startPosition++;
			break;
		}	
	}
	
	//forward:
	for(;;) {
		if (Classify(ByteAt(stopPosition))) {
			if (maxPosition == stopPosition) {
				break;
			}
			stopPosition++;	
		} else {
			if (stopPosition > startLine)
				stopPosition--;			
			break;
		}	
	}

	
	int32 len = stopPosition - startPosition + 1 ;
	if (len < 1 || len > MAX_WORD_SIZE)
		return false;

	fBuffer[len] = '\0';
	
	GetText	(startPosition, len, fBuffer);
	word.SetTo(fBuffer, len);
	/*
	//following code inspired by Terminal:

	// Collect a list of colons in the string and their respective positions in
	// the text buffer. We do this up-front so we can unlock the text buffer
	// while we're doing all the entry existence tests.
	
	typedef Array<int32> ColonList;
	ColonList colonPositions;
	int32 searchPos = startPosition;


	for (int32 index = 0; (index = word.FindFirst(':', index)) >= 0;) {
		int32 foundStart;
		int32 foundEnd;
		if (!word.Find(":", searchPos, true, true, false, foundStart,
				foundEnd)) {
			return false;
		}

		CharPosition colonPosition;
		colonPosition.index = index;
		colonPosition.position = foundStart;
		if (!colonPositions.Add(colonPosition))
			return false;

		index++;
		searchPos = foundEnd;
	}*/
/*
	textBufferLocker.Unlock();

	// Since we also want to consider ':' a potential path delimiter, in two
	// nested loops we chop off components from the beginning respective the
	// end.
	BString originalText = text;
	TermPos originalStart = _start;
	TermPos originalEnd = _end;

	int32 colonCount = colonPositions.Count();
	for (int32 startColonIndex = -1; startColonIndex < colonCount;
			startColonIndex++) {
		int32 startIndex;
		if (startColonIndex < 0) {
			startIndex = 0;
			_start = originalStart;
		} else {
			startIndex = colonPositions[startColonIndex].index + 1;
			_start = colonPositions[startColonIndex].position;
			if (_start >= pos)
				break;
			_start.x++;
				// Note: This is potentially a non-normalized position (i.e.
				// the end of a soft-wrapped line). While not that nice, it
				// works anyway.
		}

		for (int32 endColonIndex = colonCount; endColonIndex > startColonIndex;
				endColonIndex--) {
			int32 endIndex;
			if (endColonIndex == colonCount) {
				endIndex = originalText.Length();
				_end = originalEnd;
			} else {
				endIndex = colonPositions[endColonIndex].index;
				_end = colonPositions[endColonIndex].position;
				if (_end <= pos)
					break;
			}

			originalText.CopyInto(text, startIndex, endIndex - startIndex);
			if (text.IsEmpty())
				continue;

			// check, whether the file exists
			BString actualPath;
			if (_EntryExists(text, actualPath)) {
				_link = HyperLink(text, actualPath, HyperLink::TYPE_PATH);
				return true;
			}

			// As such this isn't an existing path. We also want to recognize:
			// * "<path>:<line>"
			// * "<path>:<line>:<column>"

			BString path = text;

			for (int32 i = 0; i < 2; i++) {
				int32 colonIndex = path.FindLast(':');
				if (colonIndex <= 0 || colonIndex == path.Length() - 1)
					break;

				char* numberEnd;
				strtol(path.String() + colonIndex + 1, &numberEnd, 0);
				if (*numberEnd != '\0')
					break;

				path.Truncate(colonIndex);
				if (_EntryExists(path, actualPath)) {
					BString address = path == actualPath
						? text
						: BString(actualPath) << (text.String() + colonIndex);
					_link = HyperLink(text, address,
						i == 0
							? HyperLink::TYPE_PATH_WITH_LINE
							: HyperLink::TYPE_PATH_WITH_LINE_AND_COLUMN);
					return true;
				}
			}
		}
	}
*/
	return true;
}
