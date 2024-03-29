/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include "CallTipContext.h"

#include <cstring>
#include <String.h>
#include <Debug.h>

#include "TextUtils.h"

#define parStart   '('
#define parStop    ')'
#define nextParam  ','

struct function {
	char* name   = nullptr;
	int32 length = 0; //if set it's possibly a function.
	int32 param  = 0;

};

bool operator==(const function& r, const function& l) {
	return r.name == l.name && r.length == l.length;
}

CallTipContext::CallTipContext(Editor* editor)
	:
	fEditor(editor)
{
	_Reset();
}


bool
CallTipContext::IsVisible()
{
	return fEditor->SendMessage(SCI_CALLTIPACTIVE);
}


CallTipAction
CallTipContext::UpdateCallTip(int ch, bool forceUpdate)
{
	if (!forceUpdate && ch != parStart && ch != nextParam && !IsVisible()) {
		return CALLTIP_NOTHING;
	}
	fPosition = fEditor->SendMessage(SCI_GETCURRENTPOS);

	CallTipAction action = _FindFunction();
	if (action == CALLTIP_NOTHING) {
		HideCallTip();
		return CALLTIP_NOTHING;
	}

	return action;
}


void
CallTipContext::NextCallTip()
{
	if (IsVisible() && fCurrentFunction + 1 < (int32)fSignatures.size()) {
		fCurrentFunction++;
		ShowCallTip();
	}
}


void
CallTipContext::PrevCallTip()
{
	if (IsVisible() && fCurrentFunction - 1 >= 0) {
		fCurrentFunction--;
		ShowCallTip();
	}
}


void
CallTipContext::HideCallTip()
{
	if (!IsVisible())
		return;

	fEditor->SendMessage(SCI_CALLTIPCANCEL);
	_Reset();
}


CallTipAction CallTipContext::_FindFunction()
{
	int32 line = fEditor->SendMessage(SCI_LINEFROMPOSITION, fPosition);
	int32 startpos = fEditor->SendMessage(SCI_POSITIONFROMLINE, line);
	int32 endpos = fEditor->SendMessage(SCI_GETLINEENDPOSITION, line);
	int32 len = endpos - startpos + 3;
	int32 offset = fPosition - startpos;

	char lineData[MAX_LINE_DATA];
	if ((offset < 2) || (len >= MAX_LINE_DATA)) {
		_Reset();
		return CALLTIP_NOTHING;
	}

	//buffering current line.
	fEditor->SendMessage(SCI_GETCURLINE, len, (sptr_t) (&lineData[0]));

	std::vector<function> stack;
	function curFun;
	int braceLevel = 0;
	function lastValidToken;
	function previousToken;

	for (int32 i = 0; i < offset; i++) {
		char ch = lineData[i];

		if (Contains(kWhiteSpaces, ch)) {
			//skip spaces
			continue;
		}

		function currentToken = { lineData + i, 0, 0};
		if (Contains(kWordCharacters, ch)) {
			while ((Contains(kWordCharacters, ch)) && i < offset) {
				currentToken.length++;
				ch = lineData[++i];
			}
			i--;

			lastValidToken = currentToken; //this could be a function

		} else {
			switch(ch) {
				case parStart: //'('
				{
					braceLevel++;
					stack.push_back(curFun); //let's stack it

					if (lastValidToken.name != nullptr && lastValidToken == previousToken) {
						curFun  = lastValidToken;
						curFun.param = 0;
					} else {
						// expression! (2+3)
						curFun.name = nullptr;
					}
					break;
				}
				case nextParam: //','
					//if current function is valid,
					//move to the next param
					if(curFun.name != nullptr) {
						++curFun.param;
					}
					break;
				case parStop: //')'
				{
					if (braceLevel)
						braceLevel--;
					if (stack.size() > 0) {
						curFun = stack.back();
						stack.pop_back();
					} else {
						// invalidate curFun
						curFun = function();
						lastValidToken.name = nullptr;
					}
					break;
				}
				default:
					break;
			}
		}
		//set previous.
		previousToken = currentToken;
	}

	//let's see if we have something to show!
	if (curFun.name != nullptr) {
		// pop the stack!
		while (curFun.name == nullptr && stack.size() > 0) {
			curFun = stack.back();
			stack.pop_back();
		}
	}

	CallTipAction action = CALLTIP_NOTHING;

	if (curFun.name != nullptr) {
		fCurrentParam = curFun.param;

		fCallTipPosition = startpos + (curFun.name - lineData);
		if (fCurrentFunctionName.Compare(curFun.name, curFun.length) != 0) {
			fCurrentFunctionName.SetTo(curFun.name, curFun.length);
			action = CALLTIP_NEWDATA;
		} else {
			action = CALLTIP_UPDATE;
		}
	}
	return action;
}


void
CallTipContext::UpdateSignatures(std::vector<SignatureInformation>& signatures)
{
	fCurrentFunction = -1;
	fSignatures = signatures;
	if (fSignatures.size() > 0)
		fCurrentFunction = 0;
}


void
CallTipContext::ShowCallTip()
{
	if (fSignatures.size() == 0 || fCurrentFunction < 0) {
		HideCallTip();
		return;
	}
	size_t countParams = fSignatures[fCurrentFunction].parameters.size();
	if (fCurrentParam >= countParams) {
		// we need to find a better overload!
		for (size_t i = 0; i < fSignatures.size(); ++i) {
			if (fCurrentParam < fSignatures[i].parameters.size()) {
				fCurrentFunction = i;
				break;
			}
		}
	}

	SignatureInformation& info = fSignatures[fCurrentFunction];
	BString callTipText;
	if (fSignatures.size() > 1) {
		callTipText << "\001 " << fCurrentFunction + 1 << " / " << fSignatures.size() << " \002";
	}

	callTipText << info.label.c_str();

	int32 hstart = 0;
	int32 hend = 0;
	for (size_t i = 0; i < info.parameters.size(); ++i) {
		if (i == fCurrentParam)	{
			int base = callTipText.FindFirst("\002") + 1;
			hstart = base + info.parameters[i].labelOffsets.first;
			hend   = base + info.parameters[i].labelOffsets.second;
		}
	}
	if (IsVisible())
		fEditor->SendMessage(SCI_CALLTIPCANCEL);

	fEditor->SendMessage(SCI_CALLTIPSHOW, fCallTipPosition, (sptr_t) (callTipText.String()));

	if (hstart < hend) {
		fEditor->SendMessage(SCI_CALLTIPSETHLT, hstart, hend);
	}
}


void
CallTipContext::_Reset()
{
	fCurrentParam = 0;
	fCallTipPosition = 0;
	fPosition = 0;
	fCurrentFunctionName = "";
	fSignatures.clear();
}
