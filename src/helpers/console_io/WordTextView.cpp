/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 *
 * Freely derived and inspired from the Haiku Terminal application.
 *
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "WordTextView.h"

#include <string>

#include <Array.h>
#include <FindDirectory.h>
#include <Path.h>
#include <Window.h>


#include "TextUtils.h"

//// TODO: move these function in a common TextUtilities class..

const std::string kNumericChars("1234567890");
const std::string kDefaultAdditionalWordCharacters(":@-+./_~");

WordTextView::WordTextView(const char* name)
	:
	BTextView(name)
{
}


bool
WordTextView::_Classify(char c)
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

	BTextView::FreeRunArray(tra);
}


void
WordTextView::_ResetStyle()
{
	if (fStopPosition == -1 && fStartPosition == -1)
		return;

	_SetStyle(false);
}


void
WordTextView::MouseDown(BPoint where)
{
	BTextView::MouseDown(where);
	int32 pointPosition = OffsetAt(where);
	if (pointPosition >= fStartPosition && pointPosition <= fStopPosition) {
		entry_ref  	ref;
		if (get_ref_for_path(fCurrentPath.String(), &ref) == B_OK) {
			BMessage msg(B_REFS_RECEIVED);
			msg.AddRef("refs", &ref);
			if (fCurrentPositions[0] != 0)
				msg.AddInt32("be:line", fCurrentPositions[0]);

			Window()->PostMessage(&msg);
		}
	}
}


void
WordTextView::MouseMoved(BPoint where, uint32 code, const BMessage *dragMessage)
{
	BTextView::MouseMoved(where, code, dragMessage);

	int32 pointPosition = OffsetAt(where);
	if (pointPosition >= fStartPosition && pointPosition <= fStopPosition)
		return;

	int32 startPosition = -1;
	int32 stopPosition = -1;
	BString word = "";
	fCurrentPath = "";
	fCurrentPositions[0] = fCurrentPositions[1] = 0;

	_ResetStyle();
	bool found = _GetFileAt(where, pointPosition, word, startPosition, stopPosition);

	fStopPosition  = stopPosition;
	fStartPosition = startPosition;
	fLastWord = word;

	if (found)
		_SetStyle(true);
}


bool
WordTextView::_GetFileAt(BPoint where, const int32 pointPosition, BString& word,
	int32& startPosition, int32& stopPosition)
{
	int32 line = LineAt(where);
	if (line >= CountLines())
		return false;

	int32 startLine 	= OffsetAt(line);
	int32 maxPosition 	= TextLength() - 1;

	startPosition = pointPosition;
	stopPosition  = pointPosition;

	//backward:
	for (;;) {
		if (_Classify(ByteAt(startPosition))) {
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
	for (;;) {
		if (_Classify(ByteAt(stopPosition))) {
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

	GetText(startPosition, len, fBuffer);
	word.SetTo(fBuffer, len);

	// let's try to find the ':' chars and decide if we can extract line and column
	// information.
	int32 index = 0;
	int32 firstIndex = -1;
	int32 indexPosition = -1;
	fCurrentPositions[0] = 0;
	fCurrentPositions[1] = 0;
	if ((index = word.FindFirst(':', index)) >= 0) {
		firstIndex = index;
		indexPosition = 0;
		index++;
		while (index < len) {
			if (Contains(kNumericChars, fBuffer[index])) {
				fCurrentPositions[indexPosition] = fCurrentPositions[indexPosition] * 10 + (fBuffer[index] - '0');
			} else if (fBuffer[index] == ':') {
				indexPosition++;
				if (indexPosition > 1)
					break;
			} else {
				break;
			}
			index++;
		}
	}

	if (firstIndex > 0)
		word.Truncate(firstIndex);

	return _EntryExists(word, fCurrentPath);
}


bool
WordTextView::_EntryExists(const BString& path,	BString& _actualPath) const
{
	if (path.IsEmpty())
		return false;

	if (path[0] == '/' || fCurrentDirectory.IsEmpty()) {
		_actualPath = path;
	} else if (path == "~" || path.StartsWith("~/")) {
		// Replace '~' with the user's home directory. We don't handle "~user"
		// here yet.
		BPath homeDirectory;
		if (find_directory(B_USER_DIRECTORY, &homeDirectory) != B_OK)
			return false;
		_actualPath = homeDirectory.Path();
		_actualPath << path.String() + 1;
	} else {
		_actualPath.Truncate(0);
		_actualPath << fCurrentDirectory << '/' << path;
	}

	struct stat st;
	return lstat(_actualPath, &st) == 0;
}

void
WordTextView::MakeFocus(bool focus)
{
	BTextView::MakeFocus(focus);
}
