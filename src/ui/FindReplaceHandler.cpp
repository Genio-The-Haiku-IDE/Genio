/*
 * Copyright 2023 Andrea Anzani <andrea.anzani@gmail.com>
 * Copyright 2019 Kacper Kasper <kacperkasper@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "FindReplaceHandler.h"

#include <Looper.h>
#include <Message.h>
#include <MessageFilter.h>
#include <Messenger.h>
#include <ScintillaView.h>


namespace Sci = Scintilla;
using namespace Sci::Properties;


FindReplaceHandler::FindReplaceHandler(BHandler* replyHandler)
	:
	fReplyHandler(replyHandler),
	fSearchTarget(-1, -1),
	fSearchLastResult(-1, -1),
	fSearchLast(""),
	fSearchLastFlags(0)
{
#ifdef IncrementalSearchFilter
	fIncrementalSearchFilter = new IncrementalSearchMessageFilter(this);
#endif
}


FindReplaceHandler::~FindReplaceHandler()
{
#ifdef IncrementalSearchFilter
	delete fIncrementalSearchFilter;
#endif
}


void
FindReplaceHandler::MessageReceived(BMessage* message)
{
	search_info info = _UnpackSearchMessage(*message);

	Editor*	fEditor = info.editor;

	if (!fEditor)
		return;

	Sci::Guard<SearchTarget, SearchFlags> guard(fEditor);

	const int length = info.editor->SendMessage(SCI_GETLENGTH);
	const Sci_Position anchor = info.editor->SendMessage(SCI_GETANCHOR);
	const Sci_Position current = info.editor->SendMessage(SCI_GETCURRENTPOS);

#ifdef IncrementalSearchFilter
	const auto incrementalSearch = [&]() {
		if(fIncrementalSearch == false) {
			fIncrementalSearch = true;
			fSavedSelection = { anchor, current };
		}
		Sci_Position start = std::min(anchor, current);
		Sci_Position pos = _Find(fIncrementalSearchTerm, start, length, false, false, false);

		if(pos == -1) {
			pos = _Find(fIncrementalSearchTerm, 0, start, false, false, false);
		}

		if(pos == -1) {
			Set<Selection>(fSavedSelection);
		} else {
			Set<Selection>(Get<SearchTarget>());
		}
	};
#endif
	switch(message->what) {
		case REPLACEFIND:
			if(info.replace.empty() && info.find.empty()) {
				info.replace = fSearchLastInfo.replace;
			}
			// fallthrough
		case REPLACE: {
			int replaceMsg = (info.regex ? SCI_REPLACETARGETRE : SCI_REPLACETARGET);
			if(fSearchLastResult != Sci::Range{ -1, -1 }) {
				// we need to search again, because whitespace highlighting messes with
				// the results
				Set<SearchFlags>(info.editor, fSearchLastFlags);
				Set<SearchTarget>(info.editor, fSearchLastResult);
				fEditor->SendMessage(SCI_SEARCHINTARGET, (uptr_t) fSearchLastInfo.find.size(), (sptr_t) fSearchLastInfo.find.c_str());
				fEditor->SendMessage(replaceMsg, -1, (sptr_t) info.replace.c_str());
				Sci::Range target = Get<SearchTarget>(info.editor);
				if(fSearchLastInfo.backwards == true) {
					std::swap(target.first, target.second);
				}
				fEditor->SendMessage(SCI_SETANCHOR, target.first);
				fEditor->SendMessage(SCI_SETCURRENTPOS, target.second);
				fSearchLastResult = { -1, -1 };
			}
		}
		if(message->what != REPLACEFIND) break;
		case FIND: {
			if(info.find.empty()) {
				info = fSearchLastInfo;
			}
			if((fSearchLastInfo.backwards == true && (anchor != fSearchLastResult.first
					|| current != fSearchLastResult.second))
				|| (fSearchLastInfo.backwards == false && (anchor != fSearchLastResult.second
					|| current != fSearchLastResult.first))
				|| info != fSearchLastInfo) {
				fNewSearch = true;
			}
			if(message->what == REPLACEFIND) {
				info = fSearchLastInfo;
			}

			if(fNewSearch == true) {
				if(info.inSelection == true) {
					fSearchTarget = Get<Selection>(info.editor);
					if(info.backwards == true) {
						std::swap(fSearchTarget.first, fSearchTarget.second);
					}
				} else {
					fSearchTarget = info.backwards
						? Sci::Range(std::min(anchor, current), 0)
						: Sci::Range(std::max(anchor, current), length);
				}
			}

			auto temp = fSearchTarget;

			if(fNewSearch == false) {
				temp.first = current;
			}

			Sci_Position pos = _Find(info.editor, info.find, temp.first, temp.second,
				info.matchCase, info.matchWord, info.regex);

			if(pos == -1 && info.wrapAround == true) {
				Sci_Position startAgain;
				if(info.inSelection == true) {
					startAgain = fSearchTarget.first;
				} else {
					startAgain = (info.backwards ? length : 0);
				}
				pos = _Find(info.editor, info.find, startAgain, fSearchTarget.second,
					info.matchCase, info.matchWord, info.regex);
			}
			if(pos != -1) {
				fSearchLastResult = Get<SearchTarget>(info.editor);
				if(info.backwards == true) {
					std::swap(fSearchLastResult.first, fSearchLastResult.second);
				}
				fEditor->SendMessage(SCI_SETANCHOR, fSearchLastResult.first);
				fEditor->SendMessage(SCI_SETCURRENTPOS, fSearchLastResult.second);
				fEditor->SendMessage(SCI_SCROLLCARET);
			}
			if(fReplyHandler != nullptr) {
				BMessage reply(FIND);
				reply.AddBool("found", pos != -1);
				message->SendReply(&reply, fReplyHandler);
			}
			fNewSearch = false;
			fSearchLastInfo = info;
		} break;
		case REPLACEALL: {
			Sci::UndoAction action(fEditor);
			int replaceMsg = (info.regex ? SCI_REPLACETARGETRE : SCI_REPLACETARGET);
			int occurences = 0;
			fEditor->SendMessage(info.inSelection ? SCI_TARGETFROMSELECTION : SCI_TARGETWHOLEDOCUMENT);
			auto target = Get<SearchTarget>(info.editor);
			Sci_Position pos;
			do {
				pos = _Find(info.editor, info.find, target.first, target.second,
					info.matchCase, info.matchWord, info.regex);
				if(pos != -1) {
					fEditor->SendMessage(replaceMsg, -1, (sptr_t) info.replace.c_str());
					target.first = Get<SearchTargetEnd>(info.editor);
					target.second = fEditor->SendMessage(SCI_GETLENGTH);
					occurences++;
				}
			} while(pos != -1);
			if(fReplyHandler != nullptr) {
				BMessage reply(REPLACEALL);
				reply.AddInt32("replaced", occurences);
				message->SendReply(&reply, fReplyHandler);
			}
		} break;
#ifdef IncrementalSearchFilter
		case INCREMENTAL_SEARCH_CHAR: {
			const char* character = message->GetString("character", "");
			fIncrementalSearchTerm.append(character);
			incrementalSearch();
		} break;
		case INCREMENTAL_SEARCH_BACKSPACE: {
			if(!fIncrementalSearchTerm.empty()) {
				fIncrementalSearchTerm.pop_back();
				incrementalSearch();
			}
		} break;
		case INCREMENTAL_SEARCH_CANCEL: {
			fIncrementalSearch = false;
			fIncrementalSearchTerm = "";
			Set<Selection>(fSavedSelection);
			Looper()->RemoveCommonFilter(fIncrementalSearchFilter);
		} break;
		case INCREMENTAL_SEARCH_COMMIT: {
			fIncrementalSearch = false;
			search_info si = {};
			si.wrapAround = true;
			si.find = fIncrementalSearchTerm;
			fIncrementalSearchTerm = "";
			Looper()->RemoveCommonFilter(fIncrementalSearchFilter);
		} break;
	#endif
	}

}

#ifdef IncrementalSearchFilter
FindReplaceHandler::IncrementalSearchMessageFilter::IncrementalSearchMessageFilter(BHandler* handler)
	:
	BMessageFilter(B_KEY_DOWN),
	fHandler(handler)
{
}


filter_result
FindReplaceHandler::IncrementalSearchMessageFilter::Filter(BMessage* message, BHandler** target)
{
	if(message->what == B_KEY_DOWN) {
		BLooper *looper = Looper();
		const char* bytes;
		message->FindString("bytes", &bytes);
		if(bytes[0] == B_RETURN) {
			looper->PostMessage(INCREMENTAL_SEARCH_COMMIT, fHandler);
		} else if(bytes[0] == B_ESCAPE) {
			looper->PostMessage(INCREMENTAL_SEARCH_CANCEL, fHandler);
		} else if(bytes[0] == B_BACKSPACE) {
			looper->PostMessage(INCREMENTAL_SEARCH_BACKSPACE, fHandler);
		} else {
			BMessage msg(INCREMENTAL_SEARCH_CHAR);
			msg.AddString("character", &bytes[0]);
			Looper()->PostMessage(&msg, fHandler);
		}
		return B_SKIP_MESSAGE;
	}
	return B_DISPATCH_MESSAGE;
}
#endif

Sci_Position
FindReplaceHandler::_Find(Editor* editor, std::string search, Sci_Position start,
	Sci_Position end, bool matchCase, bool matchWord, bool regex)
{
	int searchFlags = 0;
	if(matchCase == true)
		searchFlags |= SCFIND_MATCHCASE;
	if(matchWord == true)
		searchFlags |= SCFIND_WHOLEWORD;
	if(regex == true)
		searchFlags |= SCFIND_REGEXP | SCFIND_CXX11REGEX;
	Set<SearchFlags>(editor, searchFlags);
	fSearchLastFlags = searchFlags;

	Set<SearchTarget>(editor, {start, end});

	fSearchLast = search;
	Sci_Position pos = editor->SendMessage(SCI_SEARCHINTARGET,
		(uptr_t) search.size(), (sptr_t) search.c_str());
	return pos;
}


FindReplaceHandler::search_info
FindReplaceHandler::_UnpackSearchMessage(BMessage& message)
{
	search_info info;
	info.inSelection = message.GetBool("inSelection");
	info.matchCase = message.GetBool("matchCase");
	info.matchWord = message.GetBool("matchWord");
	info.wrapAround = message.GetBool("wrapAround");
	info.backwards = message.GetBool("backwards");
	info.regex = message.GetBool("regex");
	info.find = message.GetString("findText", "");
	info.replace = message.GetString("replaceText", "");
	info.editor = (Editor*)message.GetPointer("editor", "");
	return info;
}
