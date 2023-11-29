/*
 * Copyright (C) 2010 Stephan Aßmus 
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef TAB_CONTAINER_VIEW_H
#define TAB_CONTAINER_VIEW_H

#include <GroupView.h>

#if 0
class TabView;
#else
#include "TabView.h"
#endif

#include "Draggable.h"

enum {
	MSG_CLOSE_TAB			= 'cltb',
	MSG_CLOSE_TABS_ALL		= 'clta',
	MSG_CLOSE_TABS_OTHER	= 'clto'
};

class TabContainerView : public BGroupView, public Draggable {
public:
	class Controller {
	public:
		virtual	void			TabSelected(int32 tabIndex, BMessage* selInfo = nullptr) = 0;

		virtual	bool			HasFrames() = 0;
		virtual	TabView*		CreateTabView() = 0;

//		virtual	void			DoubleClickOutsideTabs() = 0;

		virtual	void			UpdateTabScrollability(bool canScrollLeft,
									bool canScrollRight) = 0;
		virtual	void			SetToolTip(int32 selected) = 0;

		virtual	void			MoveTabs(int32 fromIndex, int32 toIndex) = 0;

		virtual void			HandleTabMenuAction(BMessage* message) = 0;

	};

public:
								TabContainerView(Controller* controller);
	virtual						~TabContainerView();

	virtual	BSize				MinSize();

	virtual	void				MessageReceived(BMessage*);

	virtual	void				Draw(BRect updateRect);

	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseUp(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 transit,
									const BMessage* dragMessage);

	virtual	void				DoLayout();

			void				AddTab(const char* label, int32 index = -1);
			void				AddTab(TabView* tab, int32 index = -1);

			TabView*			RemoveTab(int32 index, int32 currentSelection, bool isLast);
			TabView*			TabAt(int32 index) const;

			int32				IndexOf(TabView* tab) const;

			void				SelectTab(int32 tabIndex, BMessage* selInfo = nullptr);
			void				SelectTab(TabView* tab, BMessage* selInfo = nullptr);

			void				SetTabLabel(int32 tabIndex, const char* label);

			void				SetFirstVisibleTabIndex(int32 index);
			int32				FirstVisibleTabIndex() const;
			int32				MaxFirstVisibleTabIndex() const;

			bool				CanScrollLeft() const;
			bool				CanScrollRight() const;

private:
			bool				InitiateDrag(BPoint where);
			void				OnDrop(BMessage* msg);
			TabView*			_TabAt(const BPoint& where, int32* index = nullptr) const;
			void				_MouseMoved(BPoint where, uint32 transit,
									const BMessage* dragMessage);
			void				_ValidateTabVisibility();
			void				_UpdateTabVisibility();
			float				_AvailableWidthForTabs() const;
			void				_SendFakeMouseMoved();
			void				_DrawTabIndicator();

private:
			TabView*			fLastMouseEventTab;
			bool				fMouseDown;
			uint32				fClickCount;
			TabView*			fSelectedTab;
			Controller*			fController;
			int32				fFirstVisibleTabIndex;
			BRect				fDropTargetHighlightFrame;
};

#endif // TAB_CONTAINER_VIEW_H
