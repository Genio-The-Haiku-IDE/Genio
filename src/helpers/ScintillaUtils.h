/*
 * Copyright 2018 Kacper Kasper <kacperkasper@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef SCINTILLAUTILS_H
#define SCINTILLAUTILS_H


#include <ScintillaView.h>


namespace Scintilla {

/**
 * Property abstracts Scintilla's enums into a single functional entity.
 * For example SCI_GETSEARCHFLAGS and SCI_SETSEARCHFLAGS can be grouped into
 * one SearchFlags property, and used like:
 *   SearchFlags::Get(scintillaView);
 * which will get current search flags for scintillaView.
 */
template<typename Type, int GetMessageId, int SetMessageId>
class Property {
public:
	static Type Get(BScintillaView* view) {
		return view->SendMessage(GetMessageId);
	}
	static void Set(BScintillaView* view, Type value) {
		view->SendMessage(SetMessageId, value);
	}
	typedef Type type;
};


typedef std::pair<Sci_Position, Sci_Position> Range;

/**
 * PropertyRange is similar to Property, but can work with range properties,
 * like selection or target. Takes two GET type messages for beginning and end
 * of range, and one SET message which should set the range in one go.
 */
template<int GetStartMessageId, int GetEndMessageId, int SetMessageId>
class PropertyRange {
public:
	static Range Get(BScintillaView* view) {
		Range range;
		range.first = view->SendMessage(GetStartMessageId);
		range.second = view->SendMessage(GetEndMessageId);
		return range;
	}
	static void Set(BScintillaView* view, Range value) {
		view->SendMessage(SetMessageId, value.first, value.second);
	}
	typedef Range type;
};


namespace Properties {

typedef Property<int, SCI_GETSEARCHFLAGS, SCI_SETSEARCHFLAGS>
	SearchFlags;
typedef Property<Sci_Position, SCI_GETTARGETSTART, SCI_SETTARGETSTART>
	SearchTargetStart;
typedef Property<Sci_Position, SCI_GETTARGETEND, SCI_SETTARGETEND>
	SearchTargetEnd;
typedef Property<int, SCI_GETINDICATORCURRENT, SCI_SETINDICATORCURRENT>
	CurrentIndicator;
typedef Property<int, SCI_GETEOLMODE, SCI_SETEOLMODE>
	EOLMode;

typedef PropertyRange<SCI_GETTARGETSTART, SCI_GETTARGETEND, SCI_SETTARGETRANGE>
	SearchTarget;
typedef PropertyRange<SCI_GETSELECTIONSTART, SCI_GETSELECTIONEND, SCI_SETSEL>
	Selection;
}


/**
 * Guard saves property state at the beginning of the scope and restores it at
 * the end. Guard can be used with multiple properties in one statement.
 * Example:
 *   Guard<SearchTarget, SearchFlags> g(scintillaView);
 */
template<typename ...Ts>
class Guard {
public:
	Guard(BScintillaView* view) {}
};


template<typename T, typename ...Ts>
class Guard<T, Ts...> : public Guard<Ts...> {
public:
	Guard(BScintillaView* view)
		: Guard<Ts...>(view), fView(view) {
		fValue = T::Get(fView);
	}
	virtual ~Guard() {
		T::Set(fView, fValue);
	}
private:
	BScintillaView* fView;
	typename T::type fValue;
};


/**
 * UndoAction groups all Scintilla commands in the scope into one undo action.
 */
class UndoAction {
public:
	UndoAction(BScintillaView* view)
		: fView(view) {
		fView->SendMessage(SCI_BEGINUNDOACTION);
	}
	~UndoAction() {
		fView->SendMessage(SCI_ENDUNDOACTION);
	}
private:
	BScintillaView* fView;
};

}


#endif // SCINTILLAUTILS_H
