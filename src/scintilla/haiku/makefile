# Make file for Scintilla on Haiku
# Copyright 1998-2010 by Neil Hodgson 
# Copyright 2011 by Andrea Anzani 
# Copyright 2014-2021 by Kacper Kasper 
# The License.txt file describes the conditions under which this software may be distributed.
# This makefile assumes GCC is used and changes will be needed to use other compilers.

srcdir ?= .
.SUFFIXES: .cxx .c .o .h .a
ARFLAGS = rc
RANLIB = touch

STATICLIB=$(srcdir)/../bin/libscintilla.a
SHAREDLIB=$(srcdir)/../bin/libscintilla.so

vpath %.h $(srcdir) $(srcdir)/../src $(srcdir)/../include
vpath %.c $(srcdir)
vpath %.cxx $(srcdir) $(srcdir)/../src

INCLUDEDIRS=-I $(srcdir)/../include -I $(srcdir)/../src
CXXBASEFLAGS=-Wall -pedantic -DHAIKU $(INCLUDEDIRS) $(DEPRECATED)

ifdef NO_CXX11_REGEX
REFLAGS=-DNO_CXX11_REGEX
endif

ifdef DEBUG
CTFLAGS=-DDEBUG -g $(CXXBASEFLAGS)
else
CTFLAGS=-DNDEBUG -Os $(CXXBASEFLAGS)
endif

CXXTFLAGS:=--std=c++17 $(CTFLAGS) $(REFLAGS) 

.cxx.o:
	$(CXX) $(CONFIGFLAGS) $(CXXTFLAGS) $(CXXFLAGS) -c $<
.c.o:
	$(CC) $(CONFIGFLAGS) $(CTFLAGS) $(CFLAGS) -w -c $<

SRCOBJS:= \
	AutoComplete.o \
	CallTip.o CaseConvert.o CaseFolder.o CellBuffer.o \
	ChangeHistory.o \
	CharacterCategoryMap.o CharacterType.o CharClassify.o \
	ContractionState.o \
	DBCS.o Decoration.o Document.o \
	EditModel.o Editor.o EditView.o \
	Geometry.o \
	Indicator.o \
	KeyMap.o \
	LineMarker.o \
	MarginView.o \
	PerLine.o PositionCache.o \
	RESearch.o RunStyles.o \
	Selection.o Style.o \
	UniConversion.o UniqueString.o \
	ViewStyle.o \
	XPM.o \
       UndoHistory.o
	
HAIKUOBJS:= \
	PlatHaiku.o \
	ScintillaBase.o \
	ScintillaHaiku.o

all: $(STATICLIB) $(SHAREDLIB)

clean:
	rm -f *.o $(STATICLIB) $(SHAREDLIB)

DEPSRCS=$(srcdir)/*.cxx $(srcdir)/../src/*.cxx
deps: deps.mak
deps.mak:
	$(CXX) -MM $(CONFIGFLAGS) $(CXXTFLAGS) $(DEPSRCS) | sed -e 's/\/usr.* //' | grep [a-zA-Z] >deps.mak

$(STATICLIB): $(SRCOBJS) $(HAIKUOBJS)
	$(AR) $(ARFLAGS) $@ $^ 
	$(RANLIB) $@

$(SHAREDLIB): $(SRCOBJS) $(HAIKUOBJS)
	$(CXX) -shared -o $@ $^

# Automatically generate header dependencies with "make deps"
include deps.mak
