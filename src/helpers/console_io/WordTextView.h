/*
 * Copyright 2023, Andrea Anzani 
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef WordTextView_H
#define WordTextView_H

#include <TextView.h>

#define MAX_WORD_SIZE	255

class WordTextView : public BTextView {
public:
	WordTextView(const char* name);

	void	MouseMoved (BPoint where, uint32 code, const BMessage *dragMessage);
	void 	MouseDown  (BPoint where);
	void	MakeFocus(bool focus = true);

private:

	void	_ResetStyle();
	bool	_GetFileAt(BPoint where, const int32 pointPosition, BString& _link, int32& _start, int32& _end);
	void	_SetStyle(bool underline);

	bool	_EntryExists(const BString& path, BString& _actualPath) const;

	bool	_Classify(char c);

	int32	fStartPosition = -1;
	int32	fStopPosition = -1;
	BString fLastWord = "";
	BString fCurrentPath = "";
	int32	fCurrentPositions[2];
	BString fCurrentDirectory; //Not used but could be usefull..

	char	fBuffer[MAX_WORD_SIZE + 1];
};


#endif // WordTextView_H_
