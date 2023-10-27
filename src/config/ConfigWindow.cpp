/*
 * Copyright 2023, Andrea Anzani 
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "ConfigWindow.h"

#include <Button.h>
#include <CardView.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <LayoutBuilder.h>
#include <OptionPopUp.h>
#include <OutlineListView.h>
#include <Spinner.h>
#include <ScrollView.h>
#include <TextControl.h>
#include <Window.h>

#include "ConfigManager.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ConfigWindow"

const int32 kOnNewValue = 'ONVA';
const int32 kSetValueNoUpdate = 'SVNU';
const int32 kItemSelected = 'ITSE';
const int32 kDefaultPressed = 'DEFA';

template<class C, typename T>
class GControl : public C {
public:
		GControl(GMessage& msg, T value, ConfigManager& cfg)
			:
			C("", "", nullptr),
			fConfigManager(cfg) {
				C::SetName(msg["key"]);
				C::SetLabel(msg["label"]);
				LoadValue(value);

				GMessage* invoke = new GMessage(kOnNewValue);
				(*invoke)["key"] = msg["key"];

				C::SetMessage(invoke);
			}

		void AttachedToWindow() {
			C::AttachedToWindow();
			C::SetTarget(this);
		}
		void MessageReceived(BMessage* msg) {
			GMessage& gsm = *((GMessage*)msg);
			if (msg->what == kOnNewValue) {
				fConfigManager[gsm["key"]] = RetrieveValue();
			} else if (msg->what == kSetValueNoUpdate) {
				LoadValue(fConfigManager[gsm["key"]]);
			} else
				C::MessageReceived(msg);
		}

		T RetrieveValue() {
			return (T)C::Value();
		}

		void LoadValue(T value) {
			C::SetValue(value);
		}

private:
		ConfigManager&	fConfigManager;
};


template<>
const char* GControl<BTextControl, const char*>::RetrieveValue()
{
       return BTextControl::Text();
}


template<>
void GControl<BTextControl, const char*>::LoadValue(const char* value)
{
       BTextControl::SetText(value);
}


ConfigWindow::ConfigWindow(ConfigManager &configManager)
    : BWindow(BRect(100, 100, 700, 500), "Settings", B_TITLED_WINDOW,
              B_ASYNCHRONOUS_CONTROLS | B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS),
      fConfigManager(configManager)
{
	CenterOnScreen();
	SetLayout(new BGroupLayout(B_HORIZONTAL));
	AddChild(_Init());
}


BView*
ConfigWindow::_Init()
{
	BView* theView = new BView("theView", B_WILL_DRAW);
	// Add the list view
	fGroupList = new BOutlineListView("Groups");
	fGroupList->SetSelectionMessage(new BMessage(kItemSelected));
	// TODO: Improve: don't use fixed size
	fGroupList->SetExplicitMinSize(BSize(180, B_SIZE_UNSET));

	BScrollView* scrollView = new BScrollView("scroll_trans",
		fGroupList, B_WILL_DRAW | B_FRAME_EVENTS, false,
		true, B_FANCY_BORDER);

	fDefaultsButton = new BButton("defaults", B_TRANSLATE("Defaults"), new BMessage(kDefaultPressed));
	fDefaultsButton->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_VERTICAL_UNSET));

	// TODO: Find a way to handle enabling/disabling
	fDefaultsButton->SetEnabled(true);

	// Box around the config and info panels
	fCardView = new BCardView("Right_Side");
	fCardView->SetExplicitAlignment(BAlignment(B_ALIGN_USE_FULL_WIDTH,
		B_ALIGN_USE_FULL_HEIGHT));

	// Populate the list view
	_PopulateListView();

	// Build the layout
	BLayoutBuilder::Group<>(theView, B_VERTICAL, 10)
		.SetInsets(B_USE_WINDOW_SPACING)
		.AddGroup(B_HORIZONTAL)
			.Add(scrollView, 1)
			.Add(fCardView, 3)
		.End()
		.Add(fDefaultsButton);

	fGroupList->MakeFocus();
	// fCardView->CardLayout()->SetVisibleItem(0);

	fGroupList->Select(0);

	be_app->StartWatching(this, MSG_NOTIFY_CONFIGURATION_UPDATED);

	fDefaultsButton->SetEnabled(!fConfigManager.HasAllDefaultValues());

	return theView;
}


void
ConfigWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kItemSelected:
		{
			int32 index = message->GetInt32("index", 0);
//			message->PrintToStream();
			if (index >= 0) {
				BStringItem* item = (BStringItem*)fGroupList->FullListItemAt(index);
				BView* card = fCardView->FindView(item->Text());
				fCardView->CardLayout()->SetVisibleItem(fCardView->CardLayout()->IndexOfView(card));
			}
			break;
		}
		case kDefaultPressed:
			gCFG.ResetToDefaults();
			break;
		case B_OBSERVER_NOTICE_CHANGE:
		{
			int32 code;
			if (message->FindInt32(B_OBSERVE_WHAT_CHANGE, &code) != B_OK)
				break;
			switch (code) {
				case MSG_NOTIFY_CONFIGURATION_UPDATED:
				{
					// TODO: Review this!
					BString key;
					if (message->FindString("key", &key) == B_OK) {
						BView* control = FindView(key.String());
						if (control != nullptr) {
							GMessage m(kSetValueNoUpdate);
							m["key"] = key.String();
							control->MessageReceived(&m);
							fDefaultsButton->SetEnabled(!fConfigManager.HasAllDefaultValues());
						}
					}
					break;
				}
				default:
					break;
			}
			break;
		}
		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void
ConfigWindow::_PopulateListView()
{
	std::vector<GMessage> divededByGroup;
	GMessage msg;
	int i=0;
	while(fConfigManager.Configuration().FindMessage("config", i++, &msg) == B_OK)  {
		//printf("Adding for %s -> %s\n", (const char*)msg["group"], (const char*)msg["label"]);
		std::vector<GMessage>::iterator i = divededByGroup.begin();
		while (i != divededByGroup.end()) {
			if (strcmp((*i)["group"], (const char*)msg["group"]) == 0) {
				(*i).AddMessage("config", &msg);
				break;
			}
			i++;
		}
		if (i == divededByGroup.end() && strcmp((const char*)msg["group"], "Hidden") != 0 ) {
			GMessage first = {{ {"group",(const char*)msg["group"]} }};
			first.AddMessage("config", &msg);
			divededByGroup.push_back(first);
		}
	}

	std::vector<GMessage>::iterator iter = divededByGroup.begin();
	while(iter != divededByGroup.end())  {
		// printf("Working for %s ", (const char*)(*iter)["group"]);
		// (*iter).PrintToStream();
		BView *groupView = MakeViewFor((const char*)(*iter)["group"], *iter);
		if (groupView != NULL) {
			groupView->SetName((const char*)(*iter)["group"]);
			fCardView->AddChild(groupView);
			BString groupName = (const char*)(*iter)["group"];
			int position = groupName.FindFirstChars("/", 0);
			if (position > 0) {
				BString leaf;
				groupName.CopyCharsInto(leaf,0,position);
				groupName.Remove(0, position + 1);
				for(int y=0;y<fGroupList->FullListCountItems();y++) {
					BStringItem* item = (BStringItem*)fGroupList->FullListItemAt(y);
					if (leaf.Compare(item->Text()) == 0) {
						int32 count = fGroupList->CountItemsUnder(item, false);
						count = count + fGroupList->FullListIndexOf(item) + 1;
						BStringItem *newItem = new BStringItem(groupName);
						newItem->SetOutlineLevel(item->OutlineLevel() + 1);
						fGroupList->AddItem(newItem, count);

						groupView->SetName(groupName);
						break;
					}
				}
			} else {
				fGroupList->AddItem(new BStringItem(groupName));
			}
		}
		iter++;
	}
}


BView*
ConfigWindow::MakeViewFor(const char* groupName, GMessage& list)
{
	// printf("Making for %s\n", (const char*)config["key"]);
	// Create and add the setting views
	BGroupView *view = new BGroupView(groupName, B_HORIZONTAL,
		B_USE_HALF_ITEM_SPACING);
	BGroupLayout *layout = view->GroupLayout();
	layout->SetInsets(B_USE_HALF_ITEM_INSETS);

	BGroupView *settingView = new BGroupView(groupName, B_VERTICAL,
		B_USE_HALF_ITEM_SPACING);
	BGroupLayout *settingLayout = settingView->GroupLayout();
	settingLayout->SetInsets(B_USE_HALF_ITEM_INSETS);

	GMessage msg;
	int i=0;
	while(list.FindMessage("config", i++, &msg) == B_OK)  {
//		msg.PrintToStream();
		BView *parameterView = MakeSelfHostingViewFor(msg);
		if (parameterView == NULL)
			return nullptr;

		settingLayout->AddView(parameterView);

	}

	settingLayout->AddItem(BSpaceLayoutItem::CreateHorizontalStrut(10));
	layout->AddView(settingView);

	// Add the sub-group views
/*	for (int32 i = 0; i < group.size(); i++) {
		SettingGroup *subGroup = group.at(i);
		if (subGroup == NULL)
			continue;

		BView *groupView = MakeViewFor(*subGroup);
		if (groupView == NULL)
			continue;

		if (i > 0)
			layout->AddView(new SeparatorView(B_VERTICAL));

		layout->AddView(groupView);
	}*/

	layout->AddItem(BSpaceLayoutItem::CreateGlue());
	return view;
}


BView*
ConfigWindow::MakeSelfHostingViewFor(GMessage& config)
{
	/*if (parameter.Flags() & B_HIDDEN_PARAMETER
		|| parameter_should_be_hidden(parameter))
		return NULL;
*/
	BView *view = MakeViewFor(config);
	if (view == NULL) {
		// The MakeViewFor() method above returns a BControl - which we
		// don't need for a null parameter; that's why it returns NULL.
		// But we want to see something anyway, so we add a string view
		// here.
		/*if (parameter.Type() == Setting::B_NULL_PARAMETER) {
			if (parameter.Group()->ParameterAt(0) == &parameter) {
				// this is the first parameter in this group, so
				// let's use a nice title view
				return new TitleView(parameter.Name());
			}
			BStringView *stringView = new BStringView(parameter.Name(),
				parameter.Name());
			stringView->SetAlignment(B_ALIGN_CENTER);

			return stringView;
		}*/

		return NULL;
	}

	/*MessageFilter *filter = MessageFilter::FilterFor(view, setting);
	if (filter != NULL)
		view->AddFilter(filter);*/

	return view;
}


BView*
ConfigWindow::MakeViewFor(GMessage& config)
{
	type_code type = config["type_code"];
	switch (type) {
		case B_BOOL_TYPE:
		{
			BCheckBox* cb = new GControl<BCheckBox, bool>(config, fConfigManager[config["key"]], fConfigManager);
			return cb;
		}
		case B_INT32_TYPE:
		{
			if (config.Has("mode")) {
				if (BString((const char*)config["mode"]).Compare("options") == 0){

					BOptionPopUp* popUp = new GControl<BOptionPopUp, int32>(config, fConfigManager[config["key"]], fConfigManager);
					int32 c=1;
					while(true) {
						BString key("option_");
						key << c;
						if (!config.Has(key.String()))
							break;
						popUp->AddOption(config[key.String()]["label"], config[key.String()]["value"]);
						c++;
					}
					return popUp;
				}
			} else {
				BSpinner* sp = new GControl<BSpinner, int32>(config, fConfigManager[config["key"]], fConfigManager);
				sp->SetValue(fConfigManager[config["key"]]);
				if (config.Has("max"))
					sp->SetMaxValue(config["max"]);
				if (config.Has("min"))
					sp->SetMinValue(config["min"]);
				return sp;
			}
		}

		case B_STRING_TYPE:
		{
			//TODO: handle the 'mode'
			GControl<BTextControl, const char*>* control = new GControl<BTextControl, const char*>(config, fConfigManager[config["key"]], fConfigManager);
			// TODO: Improve
			control->SetExplicitMinSize(BSize(350, B_SIZE_UNSET));
			return control;
		}
/*
		default:
		{
			BString errorString;
			errorString.SetToFormat("Setting: Don't know setting type: 0x%x\n", setting->Type());
			debugger(errorString.String());
		}*/
	}
	return NULL;
}
