/*
 * Copyright 2002-2012, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Vlad Slepukhin
 *		Siarzhuk Zharski
 *
 * Copied from Haiku commit a609673ce8c942d91e14f24d1d8832951ab27964.
 * Modifications:
 * Copyright 2018-2019 Kacper Kasper <kacperkasper@gmail.com>
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * Distributed under the terms of the MIT License.
 */
#ifndef EDITOR_STATUS_VIEW_H
#define EDITOR_STATUS_VIEW_H


#include <Entry.h>
#include <String.h>
#include <View.h>

#include "StatusView.h"

class BPopUpMenu;
class BScrollView;
class Editor;

#define EDITORCONFIG_YES_GLYPH "\xF0\x9F\x97\x8E"
#define EDITORCONFIG_NO_GLYPH "\xF0\x9F\x8C\x90"

namespace editor {

class StatusView : public controls::StatusView {
public:
	enum {
		UPDATE_STATUS		= 'upda'
	};

							StatusView(Editor* editor);
							~StatusView();

	virtual	void			AttachedToWindow();
			void			SetStatus(BMessage* mesage);
	virtual	void			Draw(BRect bounds);
	virtual	void			MouseDown(BPoint point);
	virtual void			MouseMoved(BPoint point, uint32 transit, const BMessage* message);

protected:
	virtual	float			Width();

private:
			void			_ShowDirMenu();
			void			_DrawNavigationButton(BRect rect);
			static void		_CreateMenu(BWindow*);
			BRect			_GetCellRect(int32 cell);


private:
	enum {
		kPositionCell,
		kOverwriteMode,
		kLineFeed,
		kFileStateCell,
		kEditorConfig,
		kTrimWhitespace,
		kIndent,
		kStatusCellCount
	};
			BString			fCellText[kStatusCellCount];
			float			fCellWidth[kStatusCellCount];
			bool			fNavigationPressed;
			float			fNavigationButtonWidth;

			static BPopUpMenu*		sMenu;
};

} // namespace editor

#endif  // EDITOR_STATUS_VIEW_H
