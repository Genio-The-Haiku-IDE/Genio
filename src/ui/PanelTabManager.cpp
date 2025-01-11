/*
 * Copyright 2024, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "PanelTabManager.h"
#include "GTabView.h"
#include "GTab.h"
#include <cassert>



class GTabID : public GTab {
	public:
		GTabID(tab_id id, const char* label, TabsContainer* container):
			GTab(label, container), fId(id)
		{
		}
		tab_id	GetID() { return fId; }
	private:
		tab_id	fId;
};

struct tab_info {
	GTabID* tab;
	BView* view;
};
typedef std::map<tab_id, tab_info> TabIdMap;


class PanelTabView : public GTabView {
public:
	PanelTabView(PanelTabManager* manager, const char* name, tab_affinity affinity, orientation orientation):
		GTabView(name, affinity, orientation)
	{
	}

	void AddTab(BView* panel, tab_id id)
	{
		GTabID* tab = new GTabID(id, panel->Name(), Container());
		GTabView::AddTab(tab, panel);
	}

	bool HasTab(tab_id id) {
		return fIdMap.contains(id);
	}

	void SelectTab(tab_id id) {
		assert(fIdMap.contains(id) == true);
		GTabView::SelectTab(fIdMap[id].tab);
	}

	void	SetLabelForTab(tab_id id, const char* label) {
		assert(fIdMap.contains(id) == true);
		fIdMap[id].tab->SetLabel(label);
		fIdMap[id].tab->Invalidate();
	}



protected:
	virtual void OnTabRemoved(GTab* _tab) override
	{
		GTabID* tab = dynamic_cast<GTabID*>(_tab);
/*
		if (fIdMap.contains(tab->GetID()) == false)
		{
			for(const auto& info:fIdMap) {
				printf("R %s: %d [%s] (vs %d)\n", Name(), info.first, info.second.view->Name(), tab->GetID());
			}
		}
*/
		assert(tab != nullptr && fIdMap.contains(tab->GetID()) == true);
		fIdMap.erase(tab->GetID());
	}
	virtual void OnTabAdded(GTab* _tab, BView* panel) override
	{
		GTabID* tab = dynamic_cast<GTabID*>(_tab);
/*
		if (fIdMap.contains(tab->GetID()) == true)
		{
			for(const auto& info:fIdMap) {
				printf("A %s: %d [%s] (vs %d)\n", Name(), info.first, info.second.view->Name(), tab->GetID());
			}
		}
*/
		assert(tab != nullptr && fIdMap.contains(tab->GetID()) == false);
		fIdMap[tab->GetID()] = { tab, panel };
	}

	GTab*	CreateTabView(GTab* clone) override
	{
		GTabID* tab = dynamic_cast<GTabID*>(clone);
		return new GTabID(tab->GetID(), tab->Label().String(), Container());;
	}

private:
	TabIdMap			fIdMap;
};

PanelTabManager::PanelTabManager()
{
}

BView*
PanelTabManager::CreatePanelTabView(const char* tabview_name, orientation orientation)
{
	PanelTabView*	tabView = new PanelTabView(this, tabview_name, 'GPAF', orientation);
	fTVList[tabview_name] = { tabView };
/*
	for (const auto& info:fTVList) {
		printf("CreatePanel %s %p\n", info.second->Name(), tabView );
	}
*/
	return tabView;
}


void
PanelTabManager::AddPanel(const char* tabview_name, BView* panel, tab_id id)
{
	PanelTabView* tabview = GetPanelTabView(tabview_name);
	assert (tabview != nullptr);

	tabview->AddTab(panel, id);

}


void
PanelTabManager::SelectTab(tab_id id)
{
	for (const auto panel:fTVList) {
		if (panel.second->HasTab(id)) {
			panel.second->SelectTab(id);
			return;
		}
	}
}


void
PanelTabManager::ShowTab(tab_id id)
{
	for (const auto panel:fTVList) {
		if (panel.second->HasTab(id)) {
			if (panel.second->IsHidden())
				panel.second->Show();
			panel.second->SelectTab(id);
			return;
		}
	}
}


void
PanelTabManager::SetLabelForTab(tab_id id, const char* label)
{
	for (const auto panel:fTVList) {
		if (panel.second->HasTab(id)) {
			panel.second->SetLabelForTab(id, label);
			return;
		}
	}
}


void
PanelTabManager::ShowPanelTabView(const char* tabview_name, bool show)
{
	PanelTabView* tabview = GetPanelTabView(tabview_name);

	if (show == true &&  tabview->IsHidden())
		tabview->Show();
	if (show == false && !tabview->IsHidden())
		tabview->Hide();
}



bool
PanelTabManager::IsPanelTabViewVisible(const char* tabview_name)
{
	return !GetPanelTabView(tabview_name)->IsHidden();
}



PanelTabView*
PanelTabManager::GetPanelTabView(const char* sname)
{
	assert(fTVList.contains(sname) == true);
	return fTVList[sname];
}
