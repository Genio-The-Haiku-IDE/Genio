/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
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
#include <ScrollView.h>
#include <Spinner.h>
#include <StringView.h>
#include <TextControl.h>
#include <Window.h>

#include <vector>

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
			GMessage& gsm = *(GMessage*)(msg);
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


template<>
const char* GControl<BOptionPopUp, const char*>::RetrieveValue()
{
	   const char* label = nullptr;
	   BOptionPopUp::SelectedOption(&label, nullptr);
	   return label;
}


template<>
void GControl<BOptionPopUp, const char*>::LoadValue(const char* value)
{
	const char* label = nullptr;
	int32 intValue = 0;
	for (int32 i = 0; i < CountOptions(); i++) {
		if (GetOptionAt(i, &label, &intValue)) {
			if (strcmp(label, value) == 0)
				break;
		}
	}
	SetValue(intValue);
}


ConfigWindow::ConfigWindow(ConfigManager &configManager)
    : BWindow(BRect(100, 100, 700, 500), "Settings", B_TITLED_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL,
              B_ASYNCHRONOUS_CONTROLS | B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE),
      fConfigManager(configManager)
{
	fShowHidden = modifiers() & B_COMMAND_KEY;

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

	fGroupList->Select(0);

	be_app->StartWatching(this, fConfigManager.UpdateMessageWhat());

	fDefaultsButton->SetEnabled(!fConfigManager.HasAllDefaultValues());

	return theView;
}

bool
ConfigWindow::QuitRequested()
{
	// it could happen that the user changed a BTextControl
	// without 'committing' it (by changing focus or by pressing enter)
	// here we try detect this case and fix it
	if (CurrentFocus() && CurrentFocus()->Parent()) {
		BTextControl* control = dynamic_cast<BTextControl*>(CurrentFocus()->Parent());
		if (control) {
			// let's avoid code duplications on how to invoke a config change.
			// and let's do it syncronous to avoid messing up with the quit workflow!
			GMessage msg = { {"what",kOnNewValue}, {"key",control->Name()}};
			control->MessageReceived(&msg);
		}
	}
	return BWindow::QuitRequested();
}

void
ConfigWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kItemSelected:
		{
			int32 index = message->GetInt32("index", 0);
			if (index >= 0) {
				BStringItem* item = dynamic_cast<BStringItem*>(fGroupList->FullListItemAt(index));
				if (item == nullptr)
					break;
				BView* card = fCardView->FindView(item->Text());
				if (card != nullptr)
					fCardView->CardLayout()->SetVisibleItem(fCardView->CardLayout()->IndexOfView(card));
			}
			break;
		}
		case kDefaultPressed:
			fConfigManager.ResetToDefaults();
			break;
		case B_OBSERVER_NOTICE_CHANGE:
		{
			int32 code;
			if (message->FindInt32(B_OBSERVE_WHAT_CHANGE, &code) != B_OK)
				break;
			if (code == fConfigManager.UpdateMessageWhat()) {
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
	std::vector<GMessage> dividedByGroup;
	GMessage msg;
	int i = 0;
	while (fConfigManager.Configuration().FindMessage("config", i++, &msg) == B_OK)  {
		std::vector<GMessage>::iterator i = dividedByGroup.begin();
		while (i != dividedByGroup.end()) {
			if (strcmp((*i)["group"], (const char*)msg["group"]) == 0) {
				(*i).AddMessage("config", &msg);
				break;
			}
			i++;
		}

		if (i == dividedByGroup.end() && (fShowHidden || (strcmp((const char*)msg["group"], "Hidden") != 0)) ) {
			GMessage first = {{ {"group", (const char*)msg["group"]} }};
			first.AddMessage("config", &msg);
			dividedByGroup.push_back(first);
		}
	}

	std::vector<GMessage>::iterator iter = dividedByGroup.begin();
	while (iter != dividedByGroup.end())  {
		BView *groupView = MakeViewFor((const char*)(*iter)["group"], *iter);
		if (groupView != NULL) {
			groupView->SetName((const char*)(*iter)["group"]);
			fCardView->AddChild(groupView);
			BString groupName = (const char*)(*iter)["group"];
			int32 position = groupName.FindFirstChars("/", 0);
			if (position > 0) {
				BString leaf;
				groupName.CopyCharsInto(leaf,0,position);
				groupName.Remove(0, position + 1);
				for (int y = 0;y < fGroupList->FullListCountItems(); y++) {
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
	int i = 0;
	while (list.FindMessage("config", i++, &msg) == B_OK)  {
		BView *parameterView = MakeControlFor(msg);
		if (parameterView == NULL)
			return nullptr;

		settingLayout->AddView(parameterView);
		if (msg.Has("note"))
			settingLayout->AddView(MakeNoteView(msg));
	}

	settingLayout->AddItem(BSpaceLayoutItem::CreateHorizontalStrut(10));
	layout->AddView(settingView);
	layout->AddItem(BSpaceLayoutItem::CreateGlue());
	return view;
}


BView*
ConfigWindow::MakeNoteView(GMessage& config)
{
	BString name((const char*)config["key"]);
	name << "_note";
	BStringView* view = new BStringView(name.String(), config["note"]);
	return view;
}


BView*
ConfigWindow::MakeControlFor(GMessage& config)
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
					return _CreatePopUp<int32>(config);
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
			if (config.Has("mode")) {
				if (BString((const char*)config["mode"]).Compare("options") == 0){
					return _CreatePopUp<const char*>(config);
				}
			} else {
				GControl<BTextControl, const char*>* control = new GControl<BTextControl, const char*>(config, fConfigManager[config["key"]], fConfigManager);
				// TODO: Improve
				control->SetExplicitMinSize(BSize(350, B_SIZE_UNSET));
				return control;
			}
		}
		default:
		{
			BString label;
			label.SetToFormat("Can't create control for setting [%s]\n", (const char*)config["key"]);
			return new BStringView("view", label.String());
		}
		break;
	}
	return NULL;
}


template<typename T>
BOptionPopUp*
ConfigWindow::_CreatePopUp(GMessage& config)
{
	auto popUp = new GControl<BOptionPopUp, T>(config, fConfigManager[config["key"]], fConfigManager);
	int32 c = 1;
	while (true) {
		BString key("option_");
		key << c;
		if (!config.Has(key.String()))
			break;
		popUp->AddOption(config[key.String()]["label"], config[key.String()]["value"]);
		c++;
	}
	popUp->LoadValue(fConfigManager[config["key"]]);
	return popUp;
}
