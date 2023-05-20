// Scintilla source code edit control
// ScintillaHaiku.cxx - Haiku specific subclass of ScintillaBase
// Copyright 2011 by Andrea Anzani 
// Copyright 2014-2021 by Kacper Kasper 
// The License.txt file describes the conditions under which this software may be distributed.
// IME support implementation based on an excellent article:
// https://www.haiku-os.org/documents/dev/developing_ime_aware_applications
// with portions from BTextView.

#include <cstdlib>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <ctime>

#include <algorithm>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include <Autolock.h>
#include <Bitmap.h>
#include <Clipboard.h>
#include <Input.h>
#include <MenuItem.h>
#include <MessageRunner.h>
#include <Messenger.h>
#include <Picture.h>
#include <PopUpMenu.h>
#include <ScrollBar.h>
#include <String.h>
#include <SupportDefs.h>
#include <View.h>
#include <Window.h>

#include "ScintillaTypes.h"
#include "ScintillaMessages.h"
#include "ScintillaStructures.h"
#include "ILoader.h"
#include "ILexer.h"

#include "Debugging.h"
#include "Geometry.h"
#include "Platform.h"
#include "Scintilla.h"
#include "ScintillaView.h"

#include "CharacterCategoryMap.h"
#include "Position.h"
#include "UniqueString.h"
#include "SplitVector.h"
#include "Partitioning.h"
#include "RunStyles.h"
#include "ContractionState.h"
#include "CellBuffer.h"
#include "CallTip.h"
#include "KeyMap.h"
#include "Indicator.h"
#include "XPM.h"
#include "LineMarker.h"
#include "Style.h"
#include "ViewStyle.h"
#include "CharClassify.h"
#include "Decoration.h"
#include "CaseFolder.h"
#include "Document.h"
#include "Selection.h"
#include "PositionCache.h"
#include "EditModel.h"
#include "MarginView.h"
#include "EditView.h"
#include "Editor.h"
#include "AutoComplete.h"
#include "ScintillaBase.h"

using namespace Scintilla;
using namespace Scintilla::Internal;

namespace {
	const int32 sNotification = 'BSCN';
	const int32 sContextMenu = 'BSCM';
	const int32 sTickMessage = 'TCKM';
	const BString sMimeRectangularMarker("text/x-rectangular-marker");

	struct pair_hash {
		inline size_t operator()(const std::pair<int32, int32> &v) const {
			std::hash<int32> int_hasher;
			return int_hasher(v.first) ^ int_hasher(v.second);
		}
	};

	Keys ConvertToSciKey(const int32 beos) {
		switch(beos) {
			case B_RIGHT_ARROW: return Keys::Right; break;
			case B_LEFT_ARROW: return Keys::Left; break;
			case B_UP_ARROW: return Keys::Up; break;
			case B_DOWN_ARROW: return Keys::Down; break;
			case B_BACKSPACE: return Keys::Back; break;
			case B_RETURN: return Keys::Return; break;
			case B_TAB: return Keys::Tab; break;
			case B_ESCAPE: return Keys::Escape; break;
			case B_INSERT: return Keys::Insert; break;
			case B_DELETE: return Keys::Delete; break;
			case B_HOME: return Keys::Home; break;
			case B_END: return Keys::End; break;
			case B_PAGE_UP: return Keys::Prior; break;
			case B_PAGE_DOWN: return Keys::Next; break;
			case B_MENU_KEY: return Keys::Menu; break;
			// TODO Add, Subtract, Divide
			default: return static_cast<Keys>(beos); break;
		}
	}

	int32 ConvertToBeKey(const Keys sci) {
		switch(sci) {
			case Keys::Right: return B_RIGHT_ARROW; break;
			case Keys::Left: return B_LEFT_ARROW; break;
			case Keys::Up: return B_UP_ARROW; break;
			case Keys::Down: return B_DOWN_ARROW; break;
			case Keys::Back: return B_BACKSPACE; break;
			case Keys::Return: return B_RETURN; break;
			case Keys::Tab: return B_TAB; break;
			case Keys::Escape: return B_ESCAPE; break;
			case Keys::Insert: return B_INSERT; break;
			case Keys::Delete: return B_DELETE; break;
			case Keys::Home: return B_HOME; break;
			case Keys::End: return B_END; break;
			case Keys::Prior: return B_PAGE_UP; break;
			case Keys::Next: return B_PAGE_DOWN; break;
			case Keys::Add: return -1; break;
			case Keys::Subtract: return -1; break;
			case Keys::Divide: return -1; break;
			case Keys::Menu: return B_MENU_KEY; break;
			default: return static_cast<int32>(sci); break;
		}
	}

	int32 ConvertToBeModifiers(const KeyMod sci) {
		return (FlagSet(sci, KeyMod::Ctrl) ? B_COMMAND_KEY : 0) |
			(FlagSet(sci, KeyMod::Shift) ? B_SHIFT_KEY : 0) |
			(FlagSet(sci, KeyMod::Alt) ? B_CONTROL_KEY : 0) |
			(FlagSet(sci, KeyMod::Super) ? B_MENU_KEY : 0) |
			(FlagSet(sci, KeyMod::Meta) ? B_OPTION_KEY : 0);
	}
}

namespace Scintilla {

class ScintillaHaiku : public ScintillaBase, public BView {
public:
	ScintillaHaiku(BRect rect);

	// functions from Scintilla:
	void Initialise();
	void Finalise();
	void SetVerticalScrollPos();
	void SetHorizontalScrollPos();
	bool ModifyScrollBars(Sci::Line, Sci::Line);
	void Copy();
	void Paste();
	void ClaimSelection();
	void NotifyChange();
	void NotifyParent(NotificationData);
	void CopyToClipboard(const SelectionText& selectedText);
	bool FineTickerRunning(TickReason reason);
	void FineTickerStart(TickReason reason, int millis, int tolerance);
	void FineTickerCancel(TickReason reason);
	void SetMouseCapture(bool);
	bool HaveMouseCapture();
	sptr_t WndProc(Message, uptr_t, sptr_t);
	sptr_t DefWndProc(Message, uptr_t, sptr_t);
	void CreateCallTipWindow(PRectangle rect);
	void AddToPopUp(const char *label, int cmd, bool enabled);
	void StartDrag();
	PRectangle GetClientRectangle() const override;
	std::string UTF8FromEncoded(std::string_view encoded) const override;
	std::string EncodedFromUTF8(std::string_view utf8) const override;

	bool Drop(BMessage* message, const BPoint& point, const BPoint& offset, bool move);

	void Draw(BRect rect);
	void KeyDown(const char* bytes, int32 len);
	void AttachedToWindow();
	void MakeFocus(bool focus = true);
	void MouseDown(BPoint point);
	void MouseUp(BPoint point);
	void MouseMoved(BPoint point, uint32 transit, const BMessage* message);
	void FrameResized(float width, float height);
	void WindowActivated(bool focus);
	void TargetedByScrollView(BScrollView* scroller);

	void MessageReceived(BMessage *msg);
	void ScrollTo(BPoint p);

private:
	bool capturedMouse;
	BMessageRunner* timers[static_cast<size_t>(TickReason::dwell) + 1];

	void _Activate();
	void _Deactivate();

	KeyMod _ConvertToSciModifiers(const int bModifiers);

	void _IMStarted(BMessage* message);
	void _IMStopped();
	void _IMChanged(BMessage* message);
	void _IMLocationRequest(BMessage* message);
	void _IMCancel();

	BMessenger msgr_input;
	bool im_started;
	int32 im_pos;
	std::string im_string;
	std::unordered_set<std::pair<int32, int32>, pair_hash> mappedShortcuts;

	BScrollView* scrollView;

	class CallTipView : public BView {
	public:
		CallTipView(BRect rect, ScintillaHaiku* sci, CallTip* ct);

		void Draw(BRect updateRect);
		void MouseDown(BPoint point);

	private:
		ScintillaHaiku* fSci;
		CallTip* fCallTip;
	};

	class CallTipWindow : public BWindow {
	public:
		CallTipWindow(BRect rect, BWindow* parent, ScintillaHaiku* sci, CallTip* ct);

	private:
		CallTipView* fView;
	};
};

ScintillaHaiku::ScintillaHaiku(BRect rect):
	BView(rect, "ScintillaHaikuView", B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS),
	scrollView(NULL)
{
	capturedMouse = false;

	Initialise();
}

void ScintillaHaiku::_IMStarted(BMessage* message) {
	im_started = (message->FindMessenger("be:reply_to", &msgr_input) == B_OK);
	im_pos = CurrentPosition();
	im_string = std::string();
	ClearBeforeTentativeStart();
	pdoc->TentativeStart();
}

void ScintillaHaiku::_IMStopped() {
	if (pdoc->TentativeActive())
		pdoc->TentativeUndo();
	msgr_input = BMessenger();
	im_started = false;
	im_pos = -1;
	im_string = std::string();
	pdoc->DecorationSetCurrentIndicator(INDIC_IME);
	pdoc->DecorationFillRange(0, 0, pdoc->Length());
	pdoc->DecorationSetCurrentIndicator(INDIC_IME + 1);
	pdoc->DecorationFillRange(0, 0, pdoc->Length());
}


static int32 UTF8NextChar(const char* str, uint32 offset) {
	if(offset >= strlen(str))
		return -1;
	const uint32 startOffset = offset;

	for(++offset; (str[offset] & 0xC0) == 0x80; ++offset)
		;

	if(offset == startOffset)
		return -1;
	return offset;
}

void ScintillaHaiku::_IMChanged(BMessage* message) {
	if(!im_started) return;

	const char* str;
	message->FindString("be:string", &str);
	im_string = str != nullptr ? str : std::string();

	const bool confirmed = message->GetBool("be:confirmed");

	pdoc->TentativeUndo();
	if(!confirmed)
		pdoc->TentativeStart();

	uint32 offset = 0;
	uint32 limit = im_string.size();

	if(confirmed) pdoc->BeginUndoAction();
	while(offset < limit) {
		const int32 next = UTF8NextChar(im_string.c_str(), offset);
		std::string character = im_string.substr(offset, next - offset);
		InsertCharacter(character, confirmed ? CharacterSource::ImeResult :
			CharacterSource::TentativeInput);
		offset = next != -1 ? next : limit;
	}
	if(confirmed) pdoc->EndUndoAction();

	if(!confirmed) {
		int32 start = 0, end = 0;
		for(int32 i = 0; message->FindInt32("be:clause_start", i, &start) == B_OK &&
						message->FindInt32("be:clause_end", i, &end) == B_OK; i++) {
			if(end > start) {
				pdoc->DecorationSetCurrentIndicator(INDIC_IME);
				pdoc->DecorationFillRange(im_pos + start, 1, end - start);
				pdoc->DecorationSetCurrentIndicator(INDIC_IME + 1);
				pdoc->DecorationFillRange(im_pos + start, 0, end - start);
			}
		}

		for(int32 i = 0; message->FindInt32("be:selection", i * 2, &start) == B_OK &&
						message->FindInt32("be:selection", i * 2 + 1, &end) == B_OK; i++) {
			if(end > start) {
				pdoc->DecorationSetCurrentIndicator(INDIC_IME);
				pdoc->DecorationFillRange(im_pos + start, 0, end - start);
				pdoc->DecorationSetCurrentIndicator(INDIC_IME + 1);
				pdoc->DecorationFillRange(im_pos + start, 1, end - start);
			}
		}
	}
}

void ScintillaHaiku::_IMLocationRequest(BMessage* message) {
	if(!im_started) return;

	if(im_string.empty()) return;

	BMessage reply(B_INPUT_METHOD_EVENT);
	reply.AddInt32("be:opcode", B_INPUT_METHOD_LOCATION_REQUEST);

	BPoint left_top = ConvertToScreen(BPoint(0, 0));
	uint32 offset = 0;
	uint32 limit = im_string.size();

	while(offset < limit) {
		Point loc = LocationFromPosition(im_pos + offset);
		BPoint pt = left_top;
		pt.x += loc.x;
		pt.y += loc.y;
		reply.AddPoint("be:location_reply", pt);
		reply.AddFloat("be:height_reply", vs.lineHeight);

		const int32 next = UTF8NextChar(im_string.c_str(), offset);
		offset = next != -1 ? next : limit;
	}

	msgr_input.SendMessage(&reply);
}

void ScintillaHaiku::_IMCancel() {
	if(im_started == true) {
		BMessage message(B_INPUT_METHOD_EVENT);
		message.AddInt32("be:opcode", B_INPUT_METHOD_STOPPED);
		msgr_input.SendMessage(&message);
	}
}

void ScintillaHaiku::Initialise() {
	for (size_t tr = static_cast<size_t>(TickReason::caret);
			tr <= static_cast<size_t>(TickReason::dwell);
			tr++) {
		timers[tr] = NULL;
	}

	if(imeInteraction == IMEInteraction::Inline)
		SetFlags(Flags() | B_INPUT_METHOD_AWARE);

	im_started = false;
	im_pos = -1;
}

void ScintillaHaiku::Finalise() {
	for (size_t tr = static_cast<size_t>(TickReason::caret);
			tr <= static_cast<size_t>(TickReason::dwell);
			tr++) {
		FineTickerCancel(static_cast<TickReason>(tr));
	}
	ScintillaBase::Finalise();
}

void ScintillaHaiku::SetVerticalScrollPos() {
	if(ScrollBar(B_VERTICAL) == NULL) return;
	if(LockLooper()) {
		ScrollBar(B_VERTICAL)->SetValue(topLine + 1);
		UnlockLooper();
	}
}

void ScintillaHaiku::SetHorizontalScrollPos() {
	if(ScrollBar(B_HORIZONTAL) == NULL) return;
	if(LockLooper()) {
 		ScrollBar(B_HORIZONTAL)->SetValue(xOffset + 1);
 		UnlockLooper();
	}
}

bool ScintillaHaiku::ModifyScrollBars(Sci::Line nMax, Sci::Line nPage) {
	int pageScroll = LinesToScroll();
	PRectangle rcText = GetTextRectangle();
	int pageWidth = rcText.Width();
	int pageIncrement = pageWidth / 3;
	int charWidth = vs.styles[STYLE_DEFAULT].aveCharWidth;
	int horizEndPreferred = std::max({scrollWidth, pageWidth - 1, 0});
	if (horizEndPreferred < 0 || Wrapping())
		horizEndPreferred = 0;

	if(LockLooper()) {
		if(ScrollBar(B_HORIZONTAL) != NULL) {
			ScrollBar(B_HORIZONTAL)->SetRange(1, horizEndPreferred + 1);
			ScrollBar(B_HORIZONTAL)->SetSteps(charWidth, pageIncrement);
			ScrollBar(B_HORIZONTAL)->SetProportion(0.0);
		}
		if(ScrollBar(B_VERTICAL) != NULL) {
			ScrollBar(B_VERTICAL)->SetRange(1, nMax - pageScroll + 1);
			ScrollBar(B_VERTICAL)->SetSteps(1.0, pageScroll - 1);
			ScrollBar(B_VERTICAL)->SetProportion(0.0);
		}
		UnlockLooper();
		return true;
	}
	return false;
}

void ScintillaHaiku::Copy() {
	if(!sel.Empty()) {
		SelectionText st;
		CopySelectionRange(&st);
		CopyToClipboard(st);
	}
}

void ScintillaHaiku::Paste() {
	const char* textPointer;
	char* text = NULL;
	ssize_t textLen = 0;
	bool isRectangular = false;
	BMessage* clip = NULL;
	if(be_clipboard->Lock()) {
		if((clip = be_clipboard->Data())) {
			isRectangular = (clip->FindData(sMimeRectangularMarker,
				B_MIME_TYPE, (const void **) &textPointer, &textLen) == B_OK);

			if(isRectangular == false) {
				clip->FindData("text/plain", B_MIME_TYPE,
					(const void **)&textPointer, &textLen);
			}

			text = new char[textLen];
			memcpy(text, textPointer, textLen);
		}
		be_clipboard->Unlock();
	}

	UndoGroup ug(pdoc);
	ClearSelection(multiPasteMode == MultiPaste::Each);
	InsertPasteShape(text, textLen, isRectangular ? PasteShape::rectangular : PasteShape::stream);
	EnsureCaretVisible();
	delete text;
}

void ScintillaHaiku::ClaimSelection() {
 	// Haiku doesn't have 'primary selection'
}

void ScintillaHaiku::NotifyChange() {
	// We don't send anything here
}

void ScintillaHaiku::NotifyParent(NotificationData n) {
	n.nmhdr.hwndFrom = static_cast<void*>(scrollView);
	n.nmhdr.idFrom = GetCtrlID();
	BMessage message(sNotification);
	message.AddData("notification", B_ANY_TYPE, &n, sizeof(NotificationData));
	if (n.text) {
		message.AddData("notification_text", B_ANY_TYPE, n.text, n.length == 0 ? strlen(n.text) : n.length);
	}

	BMessenger(this).SendMessage(&message);
}

void ScintillaHaiku::MessageReceived(BMessage* msg) {
	if(msg->WasDropped()) {
		BPoint dropOffset;
		BPoint dropPoint = msg->DropPoint(&dropOffset);
		ConvertFromScreen(&dropPoint);

		void* from = NULL;
		bool move = false;
		if (msg->FindPointer("be:originator", &from) == B_OK
				&& from == this)
			move = true;

		if(!Drop(msg, dropPoint, dropOffset, move)) {
			BView::MessageReceived(msg);
			return;
		}
	}

	switch(msg->what) {
	//case B_SIMPLE_DATA: {

	//} break;
	case B_INPUT_METHOD_EVENT: {
		int32 opcode;
		if(msg->FindInt32("be:opcode", &opcode) != B_OK) break;

		switch(opcode) {
		case B_INPUT_METHOD_STARTED:
			_IMStarted(msg);
			break;
		case B_INPUT_METHOD_STOPPED:
			_IMStopped();
			break;
		case B_INPUT_METHOD_CHANGED:
			_IMChanged(msg);
			break;
		case B_INPUT_METHOD_LOCATION_REQUEST:
			_IMLocationRequest(msg);
			break;
		default:
			BView::MessageReceived(msg);
		}
	} break;
	case 'lbdc': {
		ListBoxEvent event(ListBoxEvent::EventType::doubleClick);
		Scintilla::Internal::IListBoxDelegate* delegate;
		msg->FindPointer("delegate", reinterpret_cast<void**>(&delegate));
		if(delegate) {
			delegate->ListNotify(&event);
		}
	} break;
	case 'nava': {
		int32 key = msg->GetInt32("key", 0);
		int32 modifiers = msg->GetInt32("modifiers", 0);

		bool consumed;
		KeyDownWithModifiers(ConvertToSciKey(static_cast<char>(key)),
			_ConvertToSciModifiers(modifiers), &consumed);
	} break;
	case sTickMessage: {
		int32 reason;
		if(msg->FindInt32("reason", &reason) == B_OK) {
			TickFor(static_cast<TickReason>(reason));
		}
	} break;
	default:
		Command(msg->what);
		BView::MessageReceived(msg);
	break;
	}
}

void ScintillaHaiku::CopyToClipboard(const SelectionText& selectedText) {

	BMessage* clip = NULL;
	if (be_clipboard->Lock()) {
		be_clipboard->Clear();
		if ((clip = be_clipboard->Data())) {
			clip->AddData("text/plain", B_MIME_TYPE, selectedText.Data(), selectedText.Length());
			if(selectedText.rectangular) {
				clip->AddData(sMimeRectangularMarker, B_MIME_TYPE, selectedText.Data(), selectedText.Length());
			}
			be_clipboard->Commit();
		}
		be_clipboard->Unlock();
	}
}

bool ScintillaHaiku::FineTickerRunning(TickReason reason) {
	return timers[static_cast<size_t>(reason)] != NULL;
}

void ScintillaHaiku::FineTickerStart(TickReason reason, int millis, int tolerance) {
	FineTickerCancel(reason);
	BMessage message(sTickMessage);
	message.AddInt32("reason", static_cast<int32>(reason));
	timers[static_cast<size_t>(reason)] = new BMessageRunner(BMessenger(this), &message, millis * 1000);
	if(timers[static_cast<size_t>(reason)]->InitCheck() != B_OK) {
		FineTickerCancel(reason);
	}
}

void ScintillaHaiku::FineTickerCancel(TickReason reason) {
	if(timers[static_cast<size_t>(reason)] != NULL) {
		delete timers[static_cast<size_t>(reason)];
		timers[static_cast<size_t>(reason)] = NULL;
	}
}

void ScintillaHaiku::SetMouseCapture(bool on) {
	capturedMouse = on;
}

bool ScintillaHaiku::HaveMouseCapture() {
	return capturedMouse;
}

sptr_t ScintillaHaiku::WndProc(Message iMessage, uptr_t wParam, sptr_t lParam) {
	switch(iMessage) {
	case Message::SetIMEInteraction:
		ScintillaBase::WndProc(iMessage, wParam, lParam);
		if(imeInteraction == IMEInteraction::Windowed)
			SetFlags(Flags() & ~B_INPUT_METHOD_AWARE);
		else if(imeInteraction == IMEInteraction::Inline)
			SetFlags(Flags() | B_INPUT_METHOD_AWARE);
	break;
	case Message::GrabFocus:
		MakeFocus(true);
	break;
	case Message::AssignCmdKey: {
		int32 key = ConvertToBeKey(static_cast<Keys>(wParam & 0xffff));
		int32 mod = ConvertToBeModifiers(static_cast<KeyMod>(wParam >> 16));
		mappedShortcuts.emplace(std::make_pair(key, mod));
		return ScintillaBase::WndProc(iMessage, wParam, lParam);
	}
	case Message::ClearCmdKey: {
		int32 key = ConvertToBeKey(static_cast<Keys>(wParam & 0xffff));
		int32 mod = ConvertToBeModifiers(static_cast<KeyMod>(wParam >> 16));
		mappedShortcuts.erase(std::make_pair(key, mod));
		return ScintillaBase::WndProc(iMessage, wParam, lParam);
	}
	case Message::ClearAllCmdKeys:
		mappedShortcuts.clear();
		return ScintillaBase::WndProc(iMessage, wParam, lParam);
	case Message::Undo:
	case Message::Redo:
		if(!im_started)
			return ScintillaBase::WndProc(iMessage, wParam, lParam);
	break;
	default:
		return ScintillaBase::WndProc(iMessage, wParam, lParam);
	}

	return 0;
}

sptr_t ScintillaHaiku::DefWndProc(Message, uptr_t, sptr_t) {
	return 0;
}

void ScintillaHaiku::CreateCallTipWindow(PRectangle rect) {
	if (!ct.wCallTip.Created()) {
		CallTipWindow *callTip = new CallTipWindow(
			BRect(rect.left, rect.top, rect.right - 1, rect.bottom - 1),
			Window(), this, &ct);
		ct.wCallTip = callTip;
	}
}

void ScintillaHaiku::AddToPopUp(const char *label, int cmd, bool enabled) {
	BPopUpMenu *menu = static_cast<BPopUpMenu *>(popup.GetID());

	if(label[0] == 0)
		menu->AddSeparatorItem();
	else {
		BMessage *message = new BMessage(cmd);
		BMenuItem *item = new BMenuItem(label, message);
		item->SetTarget(this);
		item->SetEnabled(enabled);
		menu->AddItem(item);
	}
}

PRectangle ScintillaHaiku::GetClientRectangle() const {
	BRect rect = Bounds();
	return PRectangle(rect.left, rect.top, rect.right + 1, rect.bottom + 1);
}


std::string ScintillaHaiku::UTF8FromEncoded(std::string_view encoded) const {
	return std::string(encoded);
}

std::string ScintillaHaiku::EncodedFromUTF8(std::string_view utf8) const {
	return std::string(utf8);
}

void ScintillaHaiku::StartDrag() {
	inDragDrop = DragDrop::dragging;
	dropWentOutside = true;
	if(drag.Length()) {
		BMessage* dragMessage = new BMessage(B_SIMPLE_DATA);
		dragMessage->AddPointer("be:originator", this);
		dragMessage->AddInt32("be:actions", B_COPY_TARGET | B_MOVE_TARGET);
		dragMessage->AddData("text/plain", B_MIME_TYPE, drag.Data(), drag.Length());
		if(drag.rectangular) {
			dragMessage->AddData(sMimeRectangularMarker, B_MIME_TYPE, drag.Data(), drag.Length());
		}
		BHandler* dragHandler = NULL;
		Point s = LocationFromPosition(sel.LimitsForRectangularElseMain().start);
		Point e = LocationFromPosition(sel.LimitsForRectangularElseMain().end);
		BRect dragRect(s.x, s.y, e.x, e.y + vs.lineHeight);
		if(s.y != e.y && !sel.IsRectangular()) {
			dragRect.left = vs.textStart;
			dragRect.right = GetTextRectangle().right;
		}
		DragMessage(dragMessage, dragRect, dragHandler);
	}
}

bool ScintillaHaiku::Drop(BMessage* message, const BPoint& point, const BPoint& offset, bool move) {
	ssize_t textLen = 0;
	const char* text = NULL;
	bool isRectangular = false;

	isRectangular = (message->FindData(sMimeRectangularMarker,
				B_MIME_TYPE, (const void **) &text, &textLen) == B_OK);
	if(isRectangular == false) {
		if(message->FindData("text/plain", B_MIME_TYPE, (const void**) &text, &textLen) != B_OK) {
			return false;
		}
	}

	SetDragPosition(SelectionPosition(Sci::invalidPosition));

	Point p(static_cast<int>(point.x), static_cast<int>(point.y));
	SelectionPosition movePos = SPositionFromLocation(p, false, false, UserVirtualSpace());

	DropAt(movePos, text, textLen, move, isRectangular);

	return true;
}

void ScintillaHaiku::Draw(BRect rect) {
	SetViewColor(B_TRANSPARENT_COLOR);
	PRectangle rcText(rect.left, rect.top, rect.right + 1, rect.bottom + 1);

	std::unique_ptr<Surface> surface(Surface::Allocate(Technology::Default));
	surface->Init(static_cast<BView*>(this), static_cast<BView*>(wMain.GetID()));
	Paint(surface.get(), rcText);
	surface->Release();
}

void ScintillaHaiku::KeyDown(const char* bytes, int32 len) {
	Keys key = ConvertToSciKey(bytes[0]);
	int mod = modifiers();
	if(key == Keys::Tab && (Flags() & B_NAVIGABLE)) {
		// Don't catch keyboard navigation if it's enabled
		if(!(mod & B_SHIFT_KEY)) {
			BView::KeyDown(bytes, len);
			return;
		} else {
			mod ^= B_SHIFT_KEY;
		}
	}
	bool consumed;
	bool added = KeyDownWithModifiers(key, _ConvertToSciModifiers(mod), &consumed);
	if(!consumed)
		consumed = added;
	if(!consumed) {
		InsertCharacter(std::string_view(bytes, len), CharacterSource::DirectInput);
	}
}

void ScintillaHaiku::AttachedToWindow() {
	// TODO: buffered draw does not render properly
	// it does not fix flickering problem anyway
	WndProc(Message::SetBufferedDraw, 0, 0);
	WndProc(Message::SetCodePage, SC_CP_UTF8, 0);
	wMain = (WindowID)Parent();
}

void ScintillaHaiku::MakeFocus(bool focus) {
	BView::MakeFocus(focus);

	if(scrollView != NULL) {
		scrollView->SetBorderHighlighted(focus);
	}

	SetFocusState(focus);

	if(focus) _Activate();
	else _Deactivate();
}

void ScintillaHaiku::MouseDown(BPoint point) {
	_IMCancel();
	MakeFocus(true);
	SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
	BMessage* msg = Window()->CurrentMessage();
	if (!msg)
		return;

	const Point pt(static_cast<int>(point.x), static_cast<int>(point.y));
	const KeyMod mod = _ConvertToSciModifiers(modifiers());
	const int32 buttons = msg->GetInt32("buttons", 0);
	const int64 when = msg->GetInt64("when", 0) / 1000;

	if(buttons & B_PRIMARY_MOUSE_BUTTON) {
		ButtonDownWithModifiers(pt, when, mod);
	} else if(buttons & B_SECONDARY_MOUSE_BUTTON) {
		if (!PointInSelection(pt))
			SetEmptySelection(PositionFromLocation(pt));
		if(ShouldDisplayPopup(pt)) {
			BPoint screen = ConvertToScreen(point);
			ContextMenu(Point(screen.x, screen.y));
		} else {
			if(vs.MarginFromLocation(pt) < 0) {
				BMessenger msgr(Parent());
				BMessage context(sContextMenu);
				context.AddPoint("where", ConvertToScreen(point));
				msgr.SendMessage(&context);
			}
			RightButtonDownWithModifiers(pt, when, mod);
		}
	}
}

void ScintillaHaiku::MouseUp(BPoint point) {
	BMessage* msg = Window()->CurrentMessage();
	if (!msg)
		return;

	const KeyMod mod = _ConvertToSciModifiers(modifiers());
	const Point pt(static_cast<int>(point.x), static_cast<int>(point.y));
	const int64 when = msg->GetInt64("when", 0) / 1000;

	ButtonUpWithModifiers(pt, when, mod);
}

void ScintillaHaiku::MouseMoved(BPoint point, uint32 transit, const BMessage* message) {
	BMessage* movedMsg = Window()->CurrentMessage();
	if (!movedMsg)
		return;

	const KeyMod mod = _ConvertToSciModifiers(modifiers());
	const Point pt(static_cast<int>(point.x), static_cast<int>(point.y));
	const int64 when = movedMsg->GetInt64("when", 0) / 1000;
	ButtonMoveWithModifiers(pt, when, mod);
	switch(transit) {
		case B_ENTERED_VIEW:
		case B_INSIDE_VIEW:
			if (message != NULL && message->HasData("text/plain", B_MIME_TYPE)) {
				SetDragPosition(SPositionFromLocation(pt, false, false, UserVirtualSpace()));
			}
		break;
		case B_EXITED_VIEW:
			SetDragPosition(SelectionPosition(Sci::invalidPosition));
		break;
		default: break;
	};
}

void ScintillaHaiku::FrameResized(float width, float height) {
	ChangeSize();
}

void ScintillaHaiku::WindowActivated(bool focus) {
	if(IsFocus())
		SetFocusState(focus);
}

void ScintillaHaiku::TargetedByScrollView(BScrollView* scroller) {
	scrollView = scroller;
}

void ScintillaHaiku::ScrollTo(BPoint p) {
	if(p.x == 0) {
		Editor::ScrollTo(p.y - 1, false);
		if(ScrollBar(B_VERTICAL) != NULL) {
			ScrollBar(B_VERTICAL)->SetValue(p.y);
		}
	} else if (p.y == 0) {
		Editor::HorizontalScrollTo(p.x - 1);
	}
	DwellEnd(false);
}

void ScintillaHaiku::_Activate() {
	if(Window() != NULL) {
		BMessage* message;
		for(auto &mapping : kmap.GetKeyMap()) {
			const KeyMod modifiers = mapping.first.modifiers;
			if(FlagSet(modifiers, KeyMod::Ctrl)) {
				int32 key = ConvertToBeKey(mapping.first.key);
				int32 mod = ConvertToBeModifiers(modifiers);
				if(!Window()->HasShortcut(key, mod)) {
					message = new BMessage('nava');
					message->AddInt32("key", key);
					message->AddInt32("modifiers", mod);
					Window()->AddShortcut(key, mod, message, this);
					mappedShortcuts.emplace(std::make_pair(key, mod));
				}
			}
		}
	}
}

void ScintillaHaiku::_Deactivate() {
	if(Window() != NULL) {
		for(auto &shortcut : mappedShortcuts) {
			Window()->RemoveShortcut(shortcut.first, shortcut.second);
		}
		mappedShortcuts.clear();
	}
}

KeyMod ScintillaHaiku::_ConvertToSciModifiers(const int bModifiers) {
	bool shift = (bModifiers & B_SHIFT_KEY);
	bool alt = (bModifiers & B_CONTROL_KEY); // switched intentionally
	bool ctrl = (bModifiers & B_COMMAND_KEY);
	bool meta = (bModifiers & B_OPTION_KEY);
	bool super = (bModifiers & B_MENU_KEY);

	return ModifierFlags(shift, ctrl, alt, meta, super);
}

ScintillaHaiku::CallTipView::CallTipView(BRect rect, ScintillaHaiku* sci, CallTip* ct):
	BView(rect, "callTipView", B_FOLLOW_ALL, B_WILL_DRAW), fSci(sci), fCallTip(ct) {}

void ScintillaHaiku::CallTipView::Draw(BRect updateRect) {
	std::unique_ptr<Surface> surface(Surface::Allocate(Technology::Default));
	surface->Init(static_cast<BView*>(this), static_cast<BWindow*>(Window()));
	fCallTip->PaintCT(surface.get());
	surface->Release();
}

void ScintillaHaiku::CallTipView::MouseDown(BPoint point) {
	fCallTip->MouseClick(Point(point.x, point.y));
	fSci->CallTipClick();
}

ScintillaHaiku::CallTipWindow::CallTipWindow(BRect rect, BWindow* parent,
		ScintillaHaiku* sci, CallTip* ct)
	:
	BWindow(rect, "calltip", B_NO_BORDER_WINDOW_LOOK, B_FLOATING_SUBSET_WINDOW_FEEL,
		B_NOT_MOVABLE | B_NOT_CLOSABLE | B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE
			| B_NOT_RESIZABLE | B_AVOID_FOCUS) {
	fView = new CallTipView(BRect(0, 0, rect.Width(), rect.Height()), sci, ct);
	AddChild(fView);
	AddToSubset(parent);
}

}

struct BScintillaPrivate {
	ScintillaHaiku* sciControl;
};

/* BScrollView doesn't update scroll bars properly when resizing.
   Since layouts work fine and it is preferred method of creating
   new apps, we can leave BRect constructor unimplemented.
*/
#if 0
BScintillaView::BScintillaView(BRect rect, const char* name, uint32 resizingMode, uint32 flags, border_style border):
	BScrollView(name, NULL, resizingMode, flags, true, true, border) {
	sciControl = new ScintillaHaiku(rect);
	ResizeTo(rect.Width(), rect.Height());
	SetTarget(sciControl);
	sciControl->ResizeTo(rect.Width() - B_V_SCROLL_BAR_WIDTH, rect.Height() - B_H_SCROLL_BAR_HEIGHT);
}
#endif

BScintillaView::BScintillaView(const char* name, uint32 flags, bool horizontal, bool vertical, border_style border):
	BScrollView(name, NULL, flags, horizontal, vertical, border) {
	p = new BScintillaPrivate;
	p->sciControl = new ScintillaHaiku(Bounds());
	SetTarget(p->sciControl);
}

BScintillaView::~BScintillaView() {
	SetTarget(NULL);
	delete p->sciControl;
	delete p;
}

sptr_t BScintillaView::SendMessage(unsigned int iMessage, uptr_t wParam, sptr_t lParam) {
	return SendMessage(static_cast<Scintilla::Message>(iMessage), wParam, lParam);
}

sptr_t BScintillaView::SendMessage(Scintilla::Message iMessage, uptr_t wParam, sptr_t lParam) {
	BAutolock lock(Window());
	return p->sciControl->WndProc(iMessage, wParam, lParam);
}

void BScintillaView::MessageReceived(BMessage* msg) {
	switch(msg->what) {
 		case sNotification: {
 			SCNotification* n = NULL;
 			ssize_t size;
 			if (msg->FindData("notification", B_ANY_TYPE, 0, (const void**)&n, &size) == B_OK) {
 				msg->FindData("notification_text", B_ANY_TYPE, 0, (const void**)&(n->text), &size);
 				NotificationReceived(n);
 			}
 		}
 		break;
		case sContextMenu: {
			BPoint point;
			if(msg->FindPoint("where", &point) == B_OK) {
				ContextMenu(point);
			}
		} break;
		case B_CUT:
			SendMessage(SCI_CUT);
		break;
		case B_COPY:
			SendMessage(SCI_COPY);
		break;
		case B_PASTE:
			SendMessage(SCI_PASTE);
		break;
		case B_SELECT_ALL:
			SendMessage(SCI_SELECTALL);
		break;
		case B_UNDO:
			SendMessage(SCI_UNDO);
		break;
		case B_REDO:
			SendMessage(SCI_REDO);
		break;
 		default:
 			BScrollView::MessageReceived(msg);
 		break;
 	}
}

void BScintillaView::MakeFocus(bool focus) {
	p->sciControl->MakeFocus(focus);
}

void BScintillaView::NotificationReceived(SCNotification* notification) {
}

void BScintillaView::ContextMenu(BPoint point) {
}

int32 BScintillaView::TextLength()
{
	return SendMessage(SCI_GETLENGTH, 0, 0);
}

void BScintillaView::GetText(int32 offset, int32 length, char* buffer)
{
	if(offset == 0) {
		SendMessage(SCI_GETTEXT, length, (sptr_t) buffer);
	} else {
		Sci_TextRange tr;
		tr.chrg.cpMin = offset;
		tr.chrg.cpMax = offset + length;
		tr.lpstrText = buffer;
		SendMessage(SCI_GETTEXTRANGE, 0, (sptr_t) &tr);
	}
}

void BScintillaView::SetText(const char* text)
{
	SendMessage(SCI_SETTEXT, 0, (sptr_t) text);
}
