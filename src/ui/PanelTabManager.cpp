/*
 * Copyright 2024, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "PanelTabManager.h"

#include <Debug.h>

#include "GTab.h"
#include "GTabView.h"
#include "ConfigManager.h"

extern ConfigManager gCFG;

const static uint32 kShowPanelMessage = 'SHPA';

class GTabID : public GTabCloseButton {
public:
	GTabID(tab_id id, const char* label, BString previousOwner, BHandler* handler)
		:
		GTabCloseButton(label, handler),
		fId(id),
		fPreviousOwner(previousOwner)
	{
	}

	tab_id	GetID() const { return fId; }
	BString	GetPreviousOwner() const { return fPreviousOwner; }
	void	SetPreviousOwner(const char* owner) { fPreviousOwner.SetTo(owner); }

	BSize	MinSize() override
	{
		return MaxSize();
	}

	BSize	MaxSize() override
	{
		BSize min;
		float w = StringWidth(Label().String());
		float spacing = be_control_look->DefaultLabelSpacing();
		min.width = w + spacing * 4 * 2;
		min.height = TabViewTools::DefaultTabHeight();
		return min;
	}

	void SetLabel(const char* label) override
	{
		GTab::SetLabel(label);
		InvalidateLayout();
		Invalidate();
	}
private:
	tab_id	fId;
	BString	fPreviousOwner; //will become 'isHidden'?
};

struct tab_info {
	GTabID* tab;
	BView* view;
};

typedef std::map<tab_id, tab_info> TabIdMap;

class PanelTabView : public GTabView {
public:
	PanelTabView(PanelTabManager* manager, const char* name,
			tab_affinity affinity, orientation orientation)
		:
		GTabView(name, affinity, orientation, true), fManager(manager)
	{
	}

	void AddTab(BView* panel, tab_id id, BString prevOwner, int32 index=-1, bool select = false)
	{
		GTabID* tab = new GTabID(id, panel->Name(), prevOwner, this);
		GTabView::AddTab(tab, panel, index);
		if (select) {
			GTabView::SelectTab(tab);
		}
	}

	bool HasTab(tab_id id) const
	{
		return fIdMap.contains(id);
	}

	GTabID*	GetTab(tab_id id)
	{
		if (!HasTab(id))
			return nullptr;

		return fIdMap[id].tab;
	}

	void SelectTab(tab_id id)
	{
		ASSERT(fIdMap.contains(id) == true);
		GTabView::SelectTab(fIdMap[id].tab);
	}

	void SetLabelForTab(tab_id id, const char* label)
	{
		ASSERT(fIdMap.contains(id) == true);
		fIdMap[id].tab->SetLabel(label);
		fIdMap[id].tab->Invalidate();
	}

	void SaveTabsConfiguration(BMessage& config)
	{
		// To maintain the right order, let's use
		// the index interface (usually discouraged)
		for (int32 i = 0;i < Container()->CountTabs(); i++) {
			GTabID* tabid = dynamic_cast<GTabID*>(Container()->TabAt(i));
			if (tabid != nullptr) {
				BMessage tab('TAB ');
				tab.AddInt32("id", tabid->GetID());
				tab.AddString("panel_group", Name());
				tab.AddString("previous_owner", tabid->GetPreviousOwner());
				tab.AddInt32("index", i);
				tab.AddBool("selected", tabid->IsFront());
				config.AddMessage("tab", &tab);
			}
		}
	}

	void FillPanelsMenu(BMenu* menu)
	{
		for (int32 i = 0; i < Container()->CountTabs(); i++) {
			GTabID* tabid = dynamic_cast<GTabID*>(Container()->TabAt(i));
			if (tabid != nullptr) {
				BMessage* tab = new BMessage(kShowPanelMessage);
				tab->AddInt32("id", tabid->GetID());
				tab->AddString("panel_group", Name());
				tab->AddInt32("index", i);
				tab->AddBool("selected", tabid->IsFront());

				BString label = tabid->Label();
				BMenuItem*	menuItem = new BMenuItem(label.String(), tab);
				menuItem->SetTarget(this);
				menu->AddItem(menuItem);

				if (!fManager->IsPanelClosed(tabid->GetID()))
					menuItem->SetMarked(true);
			}
		}
	}

protected:
	void OnTabRemoved(GTab* _tab) override
	{
		GTabID* tab = dynamic_cast<GTabID*>(_tab);
		ASSERT(tab != nullptr && fIdMap.contains(tab->GetID()) == true);
		fIdMap.erase(tab->GetID());
	}

	void OnTabAdded(GTab* _tab, BView* panel) override
	{
		GTabID* tab = dynamic_cast<GTabID*>(_tab);
		ASSERT(tab != nullptr && fIdMap.contains(tab->GetID()) == false);
		fIdMap[tab->GetID()] = { tab, panel };
	}

	GTab* CreateTabView(GTab* clone) override
	{
		GTabID* tab = dynamic_cast<GTabID*>(clone);
		return new GTabID(tab->GetID(), tab->Label().String(), tab->GetPreviousOwner(), this);
	}

	void MessageReceived(BMessage* message) override
	{
		switch(message->what) {
			case GTabCloseButton::kTVCloseTab:
			{
				int32 fromIndex = message->GetInt32("index", -1);
				if (fromIndex > -1 && fromIndex < Container()->CountTabs()) {
					GTabID* tab = dynamic_cast<GTabID*>(Container()->TabAt(fromIndex));
					if (tab != nullptr) {
						PanelTabView* tabView = dynamic_cast<PanelTabView*>(fManager->GetPanelTabView(kTabViewHidden));
						if (tabView) {
							tab->SetPreviousOwner(Name());
							tabView->MoveTabs(tab, nullptr, Container());
						}
					}
				}
				break;
			}
			case kShowPanelMessage:
			{
				bool isSelected = message->GetBool("selected", false);

				if (IsHidden() || isSelected == false) {
					if (BString(Name()).Compare(kTabViewHidden) == 0) {
						GTabID*	tab = GetTab(message->GetInt32("id", -1));
						BString prevOwner = tab->GetPreviousOwner();
						if (prevOwner.IsEmpty())
							return;
						PanelTabView* tabView = dynamic_cast<PanelTabView*>(fManager->GetPanelTabView(prevOwner.String()));
						tab->SetPreviousOwner("");
						tabView->MoveTabs(tab, nullptr, Container());
					}
					fManager->ShowTab(message->GetInt32("id", -1));
				}

				//algo:
				/*
					1) Is the tab the selected one?
					   -> NO:
							-> ensure group is visible
							-> ensure tab is selected
					   -> YES:
					        -> close the
				*/
			}
			break;
			default:
				GTabView::MessageReceived(message);
				break;
			}
	}

private:
	TabIdMap fIdMap;
	PanelTabManager* fManager;
};


// PanelTabManager
PanelTabManager::PanelTabManager()
{
}


void
PanelTabManager::LoadConfiguration(const BMessage& config)
{
	fConfig = config;
}


void
PanelTabManager::SaveConfiguration(BMessage& config)
{
	for (const auto& info:fTVList) {
		PanelTabView* tabview = info.second;
		tabview->SaveTabsConfiguration(config);
	}
}


BView*
PanelTabManager::CreatePanelTabView(const char* tabview_name, orientation orientation)
{
	ASSERT(fTVList.contains(tabview_name) == false);

	PanelTabView* tabView = new PanelTabView(this, tabview_name, 'GPAF', orientation);
	fTVList[tabview_name] = tabView;

	//for (const auto& info:fTVList) {
		//printf("CreatePanel %s %p\n", tabview_name, tabView );
	//}

	return tabView;
}


BView*
PanelTabManager::GetPanelTabView(const char* name)
{
	return _GetPanelTabView(name);
}


void
PanelTabManager::AddPanelByConfig(BView* panel, tab_id id)
{
	BMessage tab;
	int32 i = 0;
	while (fConfig.FindMessage("tab", i++, &tab) == B_OK) {
		tab_id tabid = tab.GetInt32("id", 0);
		if (tabid == id) {
			const char* panelName = tab.GetString("panel_group", "");
			BString	prevOwner = tab.GetString("previous_owner", "");
			_AddPanel(panelName, panel, id, prevOwner, tab.GetInt32("index", -1), tab.GetBool("selected", false));
			return;
		}
	}
	BMessage defaults = DefaultConfig();
	i = 0;
	while (defaults.FindMessage("tab", i++, &tab) == B_OK) {
		tab_id tabid = tab.GetInt32("id", 0);
//				printf("Check %d vs %d\n", tabid, id);
		if (tabid == id) {
			const char* panelName = tab.GetString("panel_group", "");
			_AddPanel(panelName, panel, id, "", tab.GetInt32("index", -1), false);
			return;
		}
	}
	BString error("Cant! add a panel to tab! ");
	error << id;
	debugger(error.String());
}


void
PanelTabManager::_AddPanel(const char* tabview_name,
							BView* panel, tab_id id,
							BString prevOwner,
							int32 index, bool select)
{
	PanelTabView* tabview = _GetPanelTabView(tabview_name);
	ASSERT(tabview != nullptr);
	tabview->AddTab(panel, id, prevOwner, index, select);
}


status_t
PanelTabManager::FillPanelsMenu(BMenu* menu)
{
	for (const auto& panel:fTVList) {
		panel.second->FillPanelsMenu(menu);
	}
	return B_OK;
}


void
PanelTabManager::SelectTab(tab_id id)
{
	for (const auto& panel:fTVList) {
		if (panel.second->HasTab(id)) {
			panel.second->SelectTab(id);
			return;
		}
	}
}


void
PanelTabManager::ShowTab(tab_id id)
{
	for (const auto& panel:fTVList) {
		if (panel.second->HasTab(id)) {
			if (panel.second->IsHidden()) {
				if (panel.first.compare(kTabViewLeft) == 0) {
					gCFG["show_projects"] = true;
				} else if ( panel.first.compare(kTabViewRight) == 0) {
					gCFG["show_outline"] = true;
				} else if ( panel.first.compare(kTabViewBottom) == 0) {
					gCFG["show_output"] = true;
				}
				else {
					panel.second->Show();
				}
			}
			panel.second->SelectTab(id);
			return;
		}
	}
}


void
PanelTabManager::SetLabelForTab(tab_id id, const char* label)
{
	for (const auto& panel:fTVList) {
		if (panel.second->HasTab(id)) {
			panel.second->SetLabelForTab(id, label);
			return;
		}
	}
}


void
PanelTabManager::ShowPanelTabView(const char* tabview_name, bool show)
{
	PanelTabView* tabview = _GetPanelTabView(tabview_name);

	if (show && tabview->IsHidden())
		tabview->Show();
	if (!show && !tabview->IsHidden())
		tabview->Hide();
}


bool
PanelTabManager::IsPanelTabViewVisible(const char* tabview_name)
{
	return !_GetPanelTabView(tabview_name)->IsHidden();
}

bool
PanelTabManager::IsPanelClosed(tab_id id)
{
	return fTVList[kTabViewHidden]->HasTab(id);
}


PanelTabView*
PanelTabManager::_GetPanelTabView(const char* sname)
{
	ASSERT(fTVList.contains(sname) == true);
	return fTVList[sname];
}


BMessage
PanelTabManager::DefaultConfig()
{
	BMessage tabConfig;
	BMessage tab('TAB ');
	tab.AddInt32("id", kTabProblems);
	tab.AddString("panel_group", kTabViewBottom);
	tabConfig.AddMessage("tab", &tab);

	tab.ReplaceInt32("id", kTabBuildLog);
	tab.ReplaceString("panel_group", kTabViewBottom);
	tabConfig.AddMessage("tab", &tab);

	tab.ReplaceInt32("id", kTabOutputLog);
	tab.ReplaceString("panel_group", kTabViewBottom);
	tabConfig.AddMessage("tab", &tab);

	tab.ReplaceInt32("id", kTabSearchResult);
	tab.ReplaceString("panel_group", kTabViewBottom);
	tabConfig.AddMessage("tab", &tab);

	tab.ReplaceInt32("id", kTabTerminal);
	tab.ReplaceString("panel_group", kTabViewBottom);
	tabConfig.AddMessage("tab", &tab);

	tab.ReplaceInt32("id", kTabProjectBrowser);
	tab.ReplaceString("panel_group", kTabViewLeft);
	tabConfig.AddMessage("tab", &tab);

	tab.ReplaceInt32("id", kTabSourceControl);
	tab.ReplaceString("panel_group", kTabViewLeft);
	tabConfig.AddMessage("tab", &tab);

	tab.ReplaceInt32("id", kTabOutlineView);
	tab.ReplaceString("panel_group", kTabViewRight);
	tabConfig.AddMessage("tab", &tab);

	return tabConfig;
}
