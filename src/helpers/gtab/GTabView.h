/*
 * Copyright 2024, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once


#include <CardView.h>
#include <GroupView.h>
#include <LayoutBuilder.h>
#include <StringView.h>


class GTabScrollLeftButton;
class TabsContainer;
class GTabScrollRightButton;
class GTabMenuTabButton;
class GTabButton;
class GTab;

typedef uint32 tab_affinity;

class GTabView : public BGroupView {
	public:
					GTabView(const char* name,
							 tab_affinity affinity,
							 orientation orientation = B_HORIZONTAL,
							 bool closeButton = false,
							 bool menuButton = false);

			GTab*	AddTab(const char* label, BView* view, int32 index = -1);

			void	UpdateScrollButtons(bool left, bool right);

			void	AttachedToWindow() override;
			void	MessageReceived(BMessage* message) override;

			void	MoveTabs(GTab* fromTab, GTab* toTab, TabsContainer* fromContainer);
			void	SelectTab(GTab* tab);

	virtual void	OnMenuTabButton();

	protected:

			void	AddTab(GTab* tab, BView* view, int32 index = -1);

	virtual void	OnTabRemoved(GTab* tab) {};
	virtual void	OnTabAdded(GTab* tab, BView* view) {};
	virtual void	OnTabSelected(GTab* tab) {};

	TabsContainer*	Container() const { return fTabsContainer; }
			void	DestroyTabAndView(GTab* tab); //Remove and delete a tab and the view.

		virtual GTab*	CreateTabView(const char* label);
		virtual GTab*	CreateTabView(GTab* clone);

			BCardView*	CardView() const { return fCardView;}
		virtual BMenuItem* CreateMenuItem(GTab*);


	private:
		void	_Init(tab_affinity affinity);
		void	_FixContentOrientation(BView* view);
		void	_AddViewToCard(BView* view, int32 index);

		GTabScrollLeftButton*	fScrollLeftTabButton;
		TabsContainer*			fTabsContainer;
		GTabScrollRightButton*	fScrollRightTabButton;
		GTabMenuTabButton*		fTabMenuTabButton;
		BCardView*				fCardView;
		bool					fCloseButton;
		orientation				fContentOrientation;
		bool					fMenuButton;
};
