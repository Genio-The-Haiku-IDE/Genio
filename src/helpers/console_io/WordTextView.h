/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef WordTextView_H
#define WordTextView_H


#include <TextView.h>


#define MAX_WORD_SIZE	255

class WordTextView : public BTextView {
public:
	WordTextView(const char* name):BTextView(name){}
	
	void	MouseMoved (BPoint where, uint32 code, const BMessage *dragMessage);
	
	virtual void	OnWord(BString& newWord);
	
private:

	void	_Reset();
	bool	_GetHyperLinkAt(BPoint where, const int32 pointPosition, BString& _link, int32& _start, int32& _end);
	void	_SetStyle(bool underline);
	int32	fStartPosition = -1;
	int32	fStopPosition = -1;
	BString fLastWord = "";
	
	char	fBuffer[MAX_WORD_SIZE + 1];
	
};


#endif // WordTextView_H_
