/*
 * Copyright 2024, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "GTabView.h"
#include "TabButtons.h"
#include "GTab.h"
#include "TabsContainer.h"
#include <PopUpMenu.h>

enum {

	kLeftTabButton	= 'GTlb',
	kRightTabButton = 'GTrb',
	kMenuTabButton  = 'GTmb',
	kSelectedTabButton = 'GTse'

};

GTabView::GTabView(const char* name,
				   tab_affinity affinity,
				   orientation content_orientation,
				   bool closeButton,
				   bool menuButton) :

		BGroupView(name, B_VERTICAL, 0.0f),
		fScrollLeftTabButton(nullptr),
		fTabsContainer(nullptr),
		fScrollRightTabButton(nullptr),
		fTabMenuTabButton(nullptr),
		fCardView(nullptr),
		fCloseButton(closeButton),
		fContentOrientation(content_orientation),
		fMenuButton(menuButton)
{
	_Init(affinity);
}

GTab*
GTabView::AddTab(const char* label, BView* view, int32 index)
{
	GTab* tab = CreateTabView(label);
	AddTab(tab, view, index);
	return tab;
}

void
GTabView::AddTab(GTab* tab, BView* view, int32 index)
{
	fTabsContainer->AddTab(tab, index);
	fCardView->CardLayout()->AddView(index, view);
	_FixContentOrientation(view);
	OnTabAdded(tab, view);
#if 0
	//debug code:
	printf("-------- debug size for %s \n", Name());
	BSize containerSize = fTabsContainer->Bounds().Size();
	printf(" containerSize %f,%f\n", containerSize.Width(), containerSize.Height());
	float accu = 0.0f;
	for(int32 i=0;i<fTabsContainer->CountTabs();i++){
		GTab* tab = fTabsContainer->TabAt(i);
		accu +=  tab->Bounds().Width();
		printf("%s w %f accu %f left %f\n", tab->Label().String(), tab->Bounds().Width(), accu, containerSize.Width() - accu);
	}
#endif
}



int32
GTabView::CountTabs()
{
	return fTabsContainer->CountTabs();
}


void
GTabView::DestroyTabAndView(GTab* tab)
{
	//Remove the View from CardView
	int32 fromIndex = fTabsContainer->IndexOfTab(tab);
	BLayoutItem* fromLayout = fCardView->CardLayout()->ItemAt(fromIndex);
	BView*	fromView = fromLayout->View();
	if (!fromView)
		return;

	fromView->RemoveSelf();

	fCardView->CardLayout()->RemoveItem(fromLayout);

	GTab* rtab = fTabsContainer->RemoveTab(tab);

	OnTabRemoved(rtab);

	if (rtab)
		delete rtab;

	delete fromView;
}


void
GTabView::UpdateScrollButtons(bool left, bool right)
{
	fScrollLeftTabButton->SetEnabled(left);
	fScrollRightTabButton->SetEnabled(right);
	fTabMenuTabButton->SetEnabled(fTabsContainer->CountTabs() > 0);
}


void
GTabView::AttachedToWindow()
{
	fScrollLeftTabButton->SetTarget(this);
	fScrollRightTabButton->SetTarget(this);
	fTabsContainer->SetTarget(this);
	fTabMenuTabButton->SetTarget(this);
}


void
GTabView::MessageReceived(BMessage* message)
{
	switch(message->what) {
		case kLeftTabButton:
			fTabsContainer->ShiftTabs(-1);
		break;
		case kRightTabButton:
			fTabsContainer->ShiftTabs(+1);
		break;
		case kSelectedTabButton:
		{
			int32 index = message->GetInt32("index", 0);
			if (index > -1)
				fCardView->CardLayout()->SetVisibleItem(index);
		}
		break;
		case GTabCloseButton::kTVCloseTab:
		{
			if (fCloseButton == false)
				return;

			GTab* tab = (GTab*)message->GetPointer("tab", nullptr);
			if (tab != nullptr) {
				DestroyTabAndView(tab);
			}
		}
		break;
		case kMenuTabButton:
		{
			OnMenuTabButton();
			break;
		}
		default:
			BGroupView::MessageReceived(message);
	};
}


void
GTabView::OnMenuTabButton()
{
	BPopUpMenu* tabMenu = new BPopUpMenu("tab menu", true, false);
	int tabCount = fTabsContainer->CountTabs();
	for (int i = 0; i < tabCount; i++) {
		GTab* tab = fTabsContainer->TabAt(i);

		if (tab) {
			BMenuItem* item = new BMenuItem(tab->Label(), nullptr);
			tabMenu->AddItem(item);
			if (tab->IsFront())
				item->SetMarked(true);
		}
	}

	// Force layout to get the final menu size. InvalidateLayout()
	// did not seem to work here.
	tabMenu->AttachedToWindow();
	BRect buttonFrame = fTabMenuTabButton->Frame();
	BRect menuFrame = tabMenu->Frame();
	BPoint openPoint = ConvertToScreen(buttonFrame.LeftBottom());
	// Open with the right side of the menu aligned with the right
	// side of the button and a little below.
	openPoint.x -= menuFrame.Width() - buttonFrame.Width();
	openPoint.y += 2;

	BMenuItem *selected = tabMenu->Go(openPoint, false, false,
		ConvertToScreen(buttonFrame));
	if (selected) {
		selected->SetMarked(true);
		int32 index = tabMenu->IndexOf(selected);
		if (index != B_ERROR)
			SelectTab(fTabsContainer->TabAt(index));
	}
	fTabMenuTabButton->MenuClosed();
	delete tabMenu;

}


void
GTabView::_Init(tab_affinity affinity)
{
	fTabsContainer = new TabsContainer(this, affinity, new BMessage(kSelectedTabButton));

	fScrollLeftTabButton  = new GTabScrollLeftButton(new BMessage(kLeftTabButton), fTabsContainer);
	fScrollRightTabButton = new GTabScrollRightButton(new BMessage(kRightTabButton), fTabsContainer);


	fTabMenuTabButton = new GTabMenuTabButton(new BMessage(kMenuTabButton));

	fCardView = new BCardView("_cardview_");

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0.0f)
		.AddGroup(B_HORIZONTAL, 0.0f)
			.Add(fScrollLeftTabButton)
			.Add(fTabsContainer)
			.AddGroup(B_HORIZONTAL, 0.0f)
				.Add(fScrollRightTabButton)
				.Add(fTabMenuTabButton)
				.SetExplicitAlignment(BAlignment(B_ALIGN_RIGHT, B_ALIGN_VERTICAL_CENTER))
				.End()
			.SetExplicitAlignment(BAlignment(B_ALIGN_USE_FULL_WIDTH, B_ALIGN_VERTICAL_UNSET))
			.End()
		.Add(fCardView)
		.AddGlue(0);

	if (fMenuButton == false)
		fTabMenuTabButton->Hide();

	UpdateScrollButtons(false, false);
}


void
GTabView::_FixContentOrientation(BView* view)
{
	BLayout* layout = view->GetLayout();
	if (!layout)
		return;
	BGroupLayout* grpLayout = dynamic_cast<BGroupLayout*>(layout);
	if (!grpLayout) {
		return;
	}
	if (grpLayout->Orientation() != fContentOrientation)
	{
		grpLayout->SetOrientation(fContentOrientation);
	}
}


void
GTabView::MoveTabs(GTab* fromTab, GTab* toTab, TabsContainer* fromContainer)
{
	//Remove the View from CardView
	int32 fromIndex = fromContainer->IndexOfTab(fromTab);
	int32 toIndex = -1 ;

	if (toTab == nullptr) {
		toIndex =  fTabsContainer->CountTabs(); // last position
		if (fTabsContainer == fromContainer )
			toIndex--; //exclude filler?
	} else {
		toIndex = fTabsContainer->IndexOfTab(toTab);
	}

	BLayoutItem* fromLayout = fromContainer->GetGTabView()->CardView()->CardLayout()->ItemAt(fromIndex);
	BView*	fromView = fromLayout->View();
	if (!fromView)
		return;

	fromView->RemoveSelf();

	fromLayout->RemoveSelf();

	GTab* removedTab = fromContainer->RemoveTab(fromTab);

	fromContainer->GetGTabView()->OnTabRemoved(fromTab);

	GTab* newTab = CreateTabView(removedTab);

	AddTab(newTab, fromView, toIndex);
	SelectTab(newTab);

	delete removedTab;
}

void
GTabView::SelectTab(GTab* tab)
{
	fTabsContainer->SelectTab(tab);
}


GTab*
GTabView::CreateTabView(const char* label)
{
	return fCloseButton ? new GTabCloseButton(label, this)
						: new GTab(label);
}

GTab*
GTabView::CreateTabView(GTab* clone)
{
	return CreateTabView(clone->Label());
}






