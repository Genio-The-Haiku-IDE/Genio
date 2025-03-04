/*
 * Copyright 2024, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "PanelTabManager.h"

#include <cassert>

#include "GTab.h"
#include "GTabView.h"


class GTabID : public GTab {
	public:

		GTabID(tab_id id, const char* label)
			:
			GTab(label),
			fId(id)
		{
		}

		tab_id GetID() const { return fId; }

		BSize	MinSize() override
		{
			return MaxSize();
		}

		BSize	MaxSize() override
		{
			BSize min;
			float w = StringWidth(Label().String());
			float spacing = be_control_look->DefaultLabelSpacing();
			min.width = w + spacing * 4;
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
		GTabView(name, affinity, orientation)
	{
	}

	void AddTab(BView* panel, tab_id id, int32 index=-1, bool select = false)
	{
		GTabID* tab = new GTabID(id, panel->Name());
		GTabView::AddTab(tab, panel, index);
		if (select) {
			GTabView::SelectTab(tab);
		}
	}

	bool HasTab(tab_id id) const
	{
		return fIdMap.contains(id);
	}

	void SelectTab(tab_id id)
	{
		assert(fIdMap.contains(id) == true);
		GTabView::SelectTab(fIdMap[id].tab);
	}

	void SetLabelForTab(tab_id id, const char* label)
	{
		assert(fIdMap.contains(id) == true);
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
				tab.AddInt32("index", i);
				tab.AddBool("selected", tabid->IsFront());
				config.AddMessage("tab", &tab);
			}
		}
	}

protected:
	void OnTabRemoved(GTab* _tab) override
	{
		GTabID* tab = dynamic_cast<GTabID*>(_tab);
		assert(tab != nullptr && fIdMap.contains(tab->GetID()) == true);
		fIdMap.erase(tab->GetID());
	}

	void OnTabAdded(GTab* _tab, BView* panel) override
	{
		GTabID* tab = dynamic_cast<GTabID*>(_tab);
		assert(tab != nullptr && fIdMap.contains(tab->GetID()) == false);
		fIdMap[tab->GetID()] = { tab, panel };
	}

	GTab* CreateTabView(GTab* clone) override
	{
		GTabID* tab = dynamic_cast<GTabID*>(clone);
		return new GTabID(tab->GetID(), tab->Label().String());;
	}

private:
	TabIdMap fIdMap;
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
	assert(fTVList.contains(tabview_name) == false);

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
			_AddPanel(panelName, panel, id, tab.GetInt32("index", -1), tab.GetBool("selected", false));
			return;
		}
	}

	debugger("Cant! add a panel tab!");
}


void
PanelTabManager::_AddPanel(const char* tabview_name, BView* panel, tab_id id, int32 index, bool select)
{
	PanelTabView* tabview = _GetPanelTabView(tabview_name);
	assert (tabview != nullptr);

	tabview->AddTab(panel, id, index, select);
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



PanelTabView*
PanelTabManager::_GetPanelTabView(const char* sname)
{
	assert(fTVList.contains(sname) == true);
	return fTVList[sname];
}
