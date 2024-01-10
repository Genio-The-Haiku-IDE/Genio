#pragma once

#include "Editor.h"
#include <SupportDefs.h>
#include <cstdio>
#include <cstring>
#include <vector>
#include "protocol_objects.h"

enum CallTipAction {
	CALLTIP_NOTHING = 0, //nothing to do
	CALLTIP_NEWDATA = 2, //request LSP for new data (using the Position())
	CALLTIP_UPDATE  = 3  //just call ShowCalltip() to refresh the current calltip
};

#define MAX_LINE_DATA	256

class CallTipContext {
public:

	explicit CallTipContext(Editor* editor);

	CallTipAction UpdateCallTip(int ch, bool forceUpdate = false);

	void NextCallTip();
	void PrevCallTip();
	bool IsVisible();
	void HideCallTip();
	void ShowCallTip();

	void UpdateSignatures(std::vector<SignatureInformation>& funcs);

	int32 Position() { return fPosition; };

private:

	CallTipAction _FindFunction();
	void 		  _Reset();

	Editor* fEditor;
	int32	fCallTipPosition;
	int32 	fPosition;
	BString fCurrentFunctionName;
	int32 	fCurrentFunction;
	size_t 	fCurrentParam;

	// LSP signatures
	std::vector<SignatureInformation> fSignatures;
};
