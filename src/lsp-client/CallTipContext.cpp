#include "CallTipContext.h"
#include <cstring>
#include <String.h>
#include <Debug.h>

#include "TextUtils.h"


#define parStart   '('
#define parStop    ')'
#define nextParam  ','
#define funEnd     ';'

struct token {
	char* name;
	int32 length; //if set it's possibly a function.
};

struct function {
	int32 tokenId = -1;
	int32 functionId = -1;
	int32 param = 0;
	int32 braceLevel = -1;
};

CallTipContext::CallTipContext(Editor* editor)
				: fEditor(editor)
{
	_Reset();
};

bool
CallTipContext::IsVisible()
{
	return fEditor->SendMessage(SCI_CALLTIPACTIVE);
};

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

void CallTipContext::NextCallTip()
{
	if (IsVisible()  && fCurrentFunction + 1 < (int32)fSignatures.size()) {
		fCurrentFunction++;
		ShowCallTip();
	}
}

void CallTipContext::PrevCallTip()
{
	if (IsVisible() && fCurrentFunction - 1 >= 0) {
		fCurrentFunction--;
		ShowCallTip();
	}
}

void CallTipContext::HideCallTip()
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

	//tokenize the line

	std::vector< token > tokens;
	for (int32 i = 0; i < offset; i++) {
		char ch = lineData[i];
		token tk = { lineData + i, 0};
		if (Contains(wordCharacters, ch)) {
			while ((Contains(wordCharacters, ch)) && i < offset) {
				tk.length++;
				ch = lineData[++i];
			}
			tokens.push_back(tk);
			i--;
		} else {
			if (!Contains(whiteSpaces, ch)) {
				tokens.push_back(tk);
			}
		}
	}

	//logic to find current function.
	std::vector<function> functions;
	function curFun;
	int braceLevel = 0;
	for (size_t i = 0; i < tokens.size(); ++i) {
		token& curToken = tokens[i];
		if (curToken.length > 0) {
			curFun.tokenId = static_cast<int32_t>(i);
		} else {
			char ch = curToken.name[0];
			if (ch == parStart)  {  // '('
				braceLevel++;
				function newFun = curFun;
				functions.push_back(newFun); //let's stack it

				curFun.braceLevel = braceLevel;
				if (i > 0 && curFun.tokenId == static_cast<int32>(i)- 1) {
					curFun.functionId = curFun.tokenId;
					curFun.param = 0;
				} else {
					// expression! (2+3)
					curFun.functionId = -1;
				}
			} else if (ch == nextParam && curFun.functionId > -1) { // ','
				++curFun.param;
			} else if (ch == parStop) { //')'
				if (braceLevel)
					braceLevel--;
				if (functions.size() > 0) {
					curFun = functions.back();
					functions.pop_back();
				} else {
					// invalidate curFun
					curFun = function();
				}
			} else if (ch == funEnd) { //';'
				// invalidate everything
				functions.clear();
				curFun = function();
			}
		}
	}

	//let's see if we have something to show!
	if (curFun.functionId == -1)
	{	//pop the stack!
		while (curFun.functionId == -1 && functions.size() > 0)
		{
			curFun = functions.back();
			functions.pop_back();
		}
	}


	CallTipAction action = CALLTIP_NOTHING;
	if (curFun.functionId > -1)
	{
		token funcToken = tokens[curFun.functionId];
		funcToken.name[funcToken.length] = 0;
		fCurrentParam = curFun.param;

		fCallTipPosition = startpos + (funcToken.name - lineData);

		if (fCurrentFunctionName.Compare(funcToken.name, funcToken.length) != 0) {

			fCurrentFunctionName.SetTo(funcToken.name, funcToken.length);
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

void CallTipContext::ShowCallTip()
{
	if (fSignatures.size() == 0 || fCurrentFunction < 0) {
		HideCallTip();
		return;
	}
	size_t countParams = fSignatures[fCurrentFunction].parameters.size();
	if (fCurrentParam >= countParams) // we need to find a better overload!
	{
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

	int32 hstart,  hend = 0;
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

void CallTipContext::_Reset()
{
	fCurrentParam = 0;
	fCallTipPosition = 0;
	fPosition = 0;
	fCurrentFunctionName = "";
	fSignatures.clear();
}


