/*
 * Copyright 2017 A. Mosca 
 * All rights reserved. Distributed under the terms of the MIT license.
 */
 
#ifndef KEYWORDS_H
#define KEYWORDS_H

static const char cppKeywords[] =
// From C
"auto const double float int short struct unsigned"
" break continue else for long signed switch void"
" case default enum goto register sizeof typedef volatile"
" char do extern if return static union while"
// C++
" asm dynamic_cast namespace reinterpret_cast try"
" bool explicit new static_cast typeid"
" catch false operator template typename"
" class friend private this using"
" const_cast inline public throw virtual"
" delete mutable protected true wchar_t"
// C++11
" alignas alignof char16_t char32_t constexpr decltype"
" final noexcept nullptr override static_assert thread_local"
;

static const char rustKeywords[] =
"_ abstract alignof as become"
" box break const continue crate"
" do else enum extern false"
" final fn for if impl"
" in let loop macro match"
" mod move mut offsetof override"
" priv proc pub pure ref"
" return Self self sizeof static"
" struct super trait true type"
" typeof unsafe unsized use virtual"
" where while yield"
;

// April 2016 (taken from haiku headers)
static const char haikuClasses[] =
// App Kit
"BAppDefs BApplication BClipboard BHandler BInvoker BLooper"
" BMessage BMessageFilter BMessageQueue BMessageRunner"
" BMessenger BPropertyInfo BRoster"
// Device Kit
// Game Kit
// Interface Kit
" BAlert BBitmap BBox BButton BCheckBox BColorControl BControl BDragger BFont"
" BGraphicsDefs BInput BInterfaceDefs BListItem BListView BMenu BMenuBar"
" BMenuField BMenuItem BOutlineListView BPicture BPictureButton BPoint"
" BPolygon BPopUpMenu BPrintJob BRadioButton BRect BRegion BScreen BScrollBar"
" BScrollView BShape BShelf BSlider BStatusBar BStringView BTabView"
" BTextControl BTextView BView BWindow"
// Kernel Kit
// Locale Kit
" BCatalog BCollator BCountry BCurrency BFloatFormat BFloatFormatImpl"
" BFloatFormatParameters BFormat BFormatImpl BFormatParameters"
" BFormattingConventions BGenericNumberFormat BIntegerFormat"
" BIntegerFormatImpl BIntegerFormatParameters BLanguage BLocale"
" BNumberFormat BNumberFormatParameters BTimeFormat BUnicodeChar"
// Mail Kit
" BE-mail"
// Media Kit
// Midi Kit
// Net Kit
" BNetAddress BNetBuffer BNetEndpoint"
// Network Kit
" BNetAddress BNetBuffer BNetworkCookie BNetworkCookieJar BNetEndpoint"
" BNetDebug BUrl BUrlContext"
// Storage Kit
" BAppFileInfo BDirectory BEntry BEntryList BFile BFilePanel BFindDirectory"
" BMime BNode BNodeInfo BNodeMonitor BPath BQuery BResources BResourceStrings"
" BStatable BStorageDefs BSymLink BVolume BVolumeRoster"
// Support Kit
" BArchivable BAutolock BBeep BBlockCache BBufferIO BByteOrder"
" BClassInfo BDataIO BDebug BErrors BFlattenable BList BLocker"
" BStopWatch BString BSupportDefs BTypeConstants BUTF8 Bsyslog"
// Translation Kit
" BBitmapStream BTranslationDefs BTranslationErrors BTranslationUtils"
" BTranslatorFormats BTranslatorRoster BTranslator"
// Misc
" BScintillaView BStringItem"
// Types
"  int32 status_t"
;
#endif // KEYWORDS_H
