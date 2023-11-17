/*
 * Copyright 2023 Andrea Anzani 
 * Copyright 2019 Kacper Kasper 
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef FINDREPLACEHANLDER_H
#define FINDREPLACEHANDLER_H


#include <string>

#include <Handler.h>
#include <Message.h>
#include <MessageFilter.h>

#include "ScintillaUtils.h"
#include "Editor.h"

//NOTE: Genio is still not ready to support IncrementalSearch.
//		for now we disable this Koder feature.

//#define IncrementalSearchFilter


class FindReplaceHandler : public BHandler {
public:
	enum {
		FIND		= 'find',
		REPLACE		= 'repl',
		REPLACEFIND	= 'fnrp',
		REPLACEALL	= 'rpla',
	};
					FindReplaceHandler(BHandler* replyHandler = nullptr);
					~FindReplaceHandler();
	virtual void	MessageReceived(BMessage* message);

#ifdef IncrementalSearchFilter
	BMessageFilter*	IncrementalSearchFilter() const { return fIncrementalSearchFilter; }


private:
	enum {
		INCREMENTAL_SEARCH_CHAR			= 'incs',
		INCREMENTAL_SEARCH_BACKSPACE	= 'incb',
		INCREMENTAL_SEARCH_CANCEL		= 'ince',
		INCREMENTAL_SEARCH_COMMIT		= 'incc'
	};

	class IncrementalSearchMessageFilter : public BMessageFilter
	{
	public:
		IncrementalSearchMessageFilter(BHandler* handler);

		virtual	filter_result	Filter(BMessage* message, BHandler** target);

	private:
		BHandler *fHandler;
	};
#endif

	struct search_info {
		bool inSelection : 1;
		bool matchCase : 1;
		bool matchWord : 1;
		bool wrapAround : 1;
		bool backwards : 1;
		bool regex : 1;
		std::string find;
		std::string replace;
		Editor*	editor = nullptr;

		bool operator ==(const search_info& rhs) const {
			return inSelection == rhs.inSelection
				&& matchCase == rhs.matchCase
				&& matchWord == rhs.matchWord
				&& backwards == rhs.backwards
				&& wrapAround == rhs.wrapAround
				&& regex == rhs.regex
				&& find == rhs.find
				&& replace == rhs.replace
				&& editor == rhs.editor;
		}
		bool operator !=(const search_info& rhs) const {
			return !(*this == rhs);
		}
	};
	Sci_Position	_Find(Editor* editor, std::string search, Sci_Position start,
							Sci_Position end, bool matchCase, bool matchWord,
							bool regex);
	search_info		_UnpackSearchMessage(BMessage& message);

	template<typename T>
	typename T::type	Get(Editor* editor) { return T::Get(editor); }
	template<typename T>
	void				Set(Editor* editor, typename T::type value) { T::Set(editor, value); }


	BHandler*		fReplyHandler;

	Scintilla::Range	fSearchTarget;
	Scintilla::Range	fSearchLastResult;
	std::string			fSearchLast;
	int					fSearchLastFlags;
	bool				fNewSearch;
	search_info			fSearchLastInfo;

	bool				fIncrementalSearch;
	std::string			fIncrementalSearchTerm;
	Scintilla::Range	fSavedSelection;
#ifdef IncrementalSearchFilter
	BMessageFilter*		fIncrementalSearchFilter;
#endif
};


#endif // FINDREPLACEHANDLER_H
