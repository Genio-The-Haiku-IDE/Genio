/*
 * Copyright (C) 2010 Stephan Aßmus 
 *
 * Modified by:
 *		A. Mosca, amoscaster@gmail.com
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef TAB_MANAGER_H
#define TAB_MANAGER_H

#include <Messenger.h>
#if 0
#include <TabView.h>
#endif
#include "TabView.h"

enum {
    TABMANAGER_TAB_SELECTED		= 'Ttse',
    TABMANAGER_TAB_CLOSE_MULTI	= 'Ttcl',
    TABMANAGER_TAB_NEW_OPENED	= 'Ttno'
};

class BBitmap;
class BCardLayout;
class BGroupView;
class BMenu;
class TabContainerGroup;
class TabContainerView;
class TabManagerController;

class TabManager {
public:
    							TabManager(const BMessenger& target);
//    								, BMessage* newTabMessage);
	virtual						~TabManager();

			void				SetTarget(const BMessenger& target);
			const BMessenger&	Target() const;

#if INTEGRATE_MENU_INTO_TAB_BAR
			BMenu*				Menu() const;
#endif

			BView*				TabGroup() const;
			BView*				GetTabContainerView() const;
			BView*				ContainerView() const;

			BView*				ViewForTab(int32 tabIndex) const;
			int32				TabForView(const BView* containedView) const;
			bool				HasView(const BView* containedView) const;

	virtual void				TabSelected(int32 index, BMessage* selInfo = nullptr);
			void				DisplayTab(int32 tabIndex);
			void				SelectTab(int32 tabIndex, BMessage* selInfo = nullptr);
			void				SelectTab(const BView* containedView);
			int32				SelectedTabIndex() const;
			void				CloseTabs(int32 tabIndex[], int32 size);

			void				AddTab(BView* view, const char* label, int32 index = -1, BMessage* addInfo = nullptr);

			BView*				RemoveTab(int32 index);
			int32				CountTabs() const;

			void				MoveTabs(int32 from, int32 to);

			void				SetTabLabel(int32 tabIndex, const char* label);
	const	BString&			TabLabel(int32);
			void				SetTabIcon(const BView* containedView,
									const BBitmap* icon);
			void				SetCloseButtonsAvailable(bool available);

private:
#if INTEGRATE_MENU_INTO_TAB_BAR
			BMenu*				fMenu;
#endif
			TabContainerGroup*	fTabContainerGroup;
			TabContainerView*	fTabContainerView;
			BView*				fContainerView;
			BCardLayout*		fCardLayout;
			TabManagerController* fController;

			BMessenger			fTarget;
};

#endif // TAB_MANAGER_H
