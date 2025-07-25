/*
 * Copyright 2023-2025, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "ConfigWindow.h"

#include <Box.h>
#include <Button.h>
#include <CardView.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <ColorControl.h>
#include <LayoutBuilder.h>
#include <OptionPopUp.h>
#include <OutlineListView.h>
#include <ScrollView.h>
#include <SeparatorView.h>
#include <Spinner.h>
#include <StringView.h>
#include <TextControl.h>

#include <vector>

#include "ConfigManager.h"
#include "OptionPopUpString.h"

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
		fConfigManager(cfg)
	{
		C::SetName(msg["key"]);
		C::SetLabel(msg["label"]);
		LoadValue(value);

		GMessage* invoke = new GMessage(kOnNewValue);
		(*invoke)["key"] = msg["key"];

		C::SetMessage(invoke);
	}

	void AttachedToWindow()
	{
		C::AttachedToWindow();
		C::SetTarget(this);
	}

	void MessageReceived(BMessage* msg)
	{
		GMessage& gsm = *reinterpret_cast<GMessage*>(msg);
		if (msg->what == kOnNewValue) {
			fConfigManager[gsm["key"]] = RetrieveValue();
		} else if (msg->what == kSetValueNoUpdate) {
			LoadValue(fConfigManager[gsm["key"]]);
		} else
			C::MessageReceived(msg);
	}

	T RetrieveValue() const
	{
		return C::Value();
	}

	void LoadValue(T value)
	{
		C::SetValue(value);
	}
private:
	ConfigManager&	fConfigManager;
};


template<>
const char* GControl<BTextControl, const char*>::RetrieveValue() const
{
	return BTextControl::Text();
}


template<>
void GControl<BTextControl, const char*>::LoadValue(const char* value)
{
	BTextControl::SetText(value);
}


template<>
const char* GControl<BOptionPopUp, const char*>::RetrieveValue() const
{
	const char* label = nullptr;
	BOptionPopUp::SelectedOption(&label, nullptr);
	return label;
}


template<>
void GControl<BOptionPopUp, const char*>::LoadValue(const char* value)
{
	int32 intValue = 0;
	for (int32 i = 0; i < CountOptions(); i++) {
		const char* label = nullptr;
		if (GetOptionAt(i, &label, &intValue)) {
			if (::strcmp(label, value) == 0)
				break;
		}
	}
	SetValue(intValue);
}


// BColorControl
template<>
GControl<BColorControl, rgb_color>::GControl(GMessage& msg, rgb_color value, ConfigManager& cfg)
	:
	BColorControl(B_ORIGIN,	B_CELLS_32x8, 8, ""),
	fConfigManager(cfg)
{
	BColorControl::SetName(msg["key"]);
	BColorControl::SetLabel(msg["label"]);
	LoadValue(value);

	GMessage* invoke = new GMessage(kOnNewValue);
	(*invoke)["key"] = msg["key"];
	BColorControl::SetMessage(invoke);
}


template<>
rgb_color GControl<BColorControl, rgb_color>::RetrieveValue() const
{
	// TODO: Why isn't BColorControl::ValueAsColor() const ?
	return const_cast<GControl*>(this)->BColorControl::ValueAsColor();
}


ConfigWindow::ConfigWindow(ConfigManager &configManager, bool showDefaultButton)
    :
	BWindow(BRect(), B_TRANSLATE("Settings"), B_TITLED_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS | B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE),
    fConfigManager(configManager),
	fDefaultsButton(nullptr)
{
	fShowHidden = modifiers() & B_COMMAND_KEY;

	SetLayout(new BGroupLayout(B_HORIZONTAL));
	AddChild(_Init(showDefaultButton));
	CenterOnScreen();
}


BView*
ConfigWindow::_Init(bool showDefaultButton)
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

	if (showDefaultButton) {
		fDefaultsButton = new BButton("defaults", B_TRANSLATE("Defaults"), new BMessage(kDefaultPressed));
		fDefaultsButton->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_VERTICAL_UNSET));
		fDefaultsButton->SetEnabled(true);
	}
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
		.End();

	fGroupList->MakeFocus();
	fGroupList->Select(0);

	be_app->StartWatching(this, fConfigManager.UpdateMessageWhat());

	if (fDefaultsButton != nullptr) {
		theView->GetLayout()->AddView(fDefaultsButton);
		fDefaultsButton->SetEnabled(!fConfigManager.HasAllDefaultValues());
	}
	return theView;
}


bool
ConfigWindow::QuitRequested()
{
	// it could happen that the user changed a BTextControl
	// without 'committing' it (by changing focus or by pressing enter)
	// here we try detect this case and fix it
	BView* focus = CurrentFocus();
	if (focus != nullptr &&
	    focus->Parent() != nullptr) {
		BTextControl* control = dynamic_cast<BTextControl*>(focus->Parent());
		if (control != nullptr && fConfigManager.HasKey(control->Name())) {
			// let's avoid code duplications on how to invoke a config change.
			// and let's do it syncronous to avoid messing up with the quit workflow!
			GMessage msg = { {"what", kOnNewValue}, {"key", control->Name()}};
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
			int32 index = fGroupList->FullListCurrentSelection();
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
					}
				}

				BString context = message->GetString("context", "");
				if (context.IsEmpty() || context.Compare("reset_to_defaults_end") == 0) {
					if (fDefaultsButton != nullptr)
						fDefaultsButton->SetEnabled(!fConfigManager.HasAllDefaultValues());
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
	// TODO: Cleanup
	std::vector<GMessage> dividedByGroup;
	GMessage msg;
	int32 index = 0;
	while (fConfigManager.FindConfigMessage("config", index++, &msg) == B_OK)  {
		std::vector<GMessage>::iterator i = dividedByGroup.begin();
		while (i != dividedByGroup.end()) {
			if (::strcmp((*i)["group"], msg["group"]) == 0) {
				(*i).AddMessage("config", &msg);
				break;
			}
			i++;
		}

		if (i == dividedByGroup.end() &&
			(fShowHidden || (::strcmp(msg["group"], "Hidden") != 0)) ) {
			const char* group = msg["group"];
			GMessage first = {{ {"group", group} }};
			first.AddMessage("config", &msg);
			dividedByGroup.push_back(first);
		}
	}

	std::vector<GMessage>::iterator iter = dividedByGroup.begin();
	while (iter != dividedByGroup.end())  {
		BString groupName = (*iter)["group"];
		BView *groupView = MakeViewFor(groupName.String(), *iter);
		if (groupView != NULL) {
			groupView->SetName(groupName.String());
			fCardView->AddChild(groupView);

			int32 position = groupName.FindFirstChars("/", 0);
			if (position > 0) {
				BString leaf;
				groupName.CopyCharsInto(leaf, 0, position);
				groupName.Remove(0, position + 1);
				for (int32 y = 0;y < fGroupList->FullListCountItems(); y++) {
					BStringItem* item = static_cast<BStringItem*>(fGroupList->FullListItemAt(y));
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
	BBox* box = new BBox(groupName);
	box->SetLabel(groupName);
	BGroupView *settingView = new BGroupView(groupName, B_VERTICAL,
		B_USE_HALF_ITEM_SPACING);
	BGroupLayout *settingLayout = settingView->GroupLayout();
	settingLayout->SetInsets(B_USE_HALF_ITEM_INSETS);

	GMessage msg;
	int32 i = 0;
	while (list.FindMessage("config", i++, &msg) == B_OK)  {
		BView *parameterView = MakeControlFor(msg);
		if (parameterView == NULL)
			return nullptr;
		BColorControl* colorControl = dynamic_cast<BColorControl*>(parameterView);
		if (colorControl != nullptr) {
			// BColorControls don't have a label so we add one ourselves
			settingLayout->AddView(new BStringView(colorControl->Label(), colorControl->Label()));
		}
		settingLayout->AddView(parameterView);
		if (msg.Has("note"))
			settingLayout->AddView(MakeNoteView(msg));
	}

	settingLayout->AddItem(BSpaceLayoutItem::CreateHorizontalStrut(10));
	box->AddChild(settingView);

	return box;
}


BView*
ConfigWindow::MakeNoteView(GMessage& config)
{
	BString name(config["key"]);
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
			BCheckBox* cb = new GControl<BCheckBox, bool>(config, fConfigManager[config["key"]],
				fConfigManager);
			return cb;
		}
		case B_INT32_TYPE:
		{
			if (config.Has("mode")) {
				if (BString(config["mode"]).Compare("options") == 0) {
					return _CreatePopUp<int32, BOptionPopUp>(config);
				}
			} else {
				BSpinner* sp = new GControl<BSpinner, int32>(config, fConfigManager[config["key"]],
					fConfigManager);
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
				if (BString(config["mode"]).Compare("options") == 0) {
					return _CreatePopUp<const char*, OptionPopUpString>(config);
				}
			} else {
				GControl<BTextControl, const char*>* control =
					new GControl<BTextControl, const char*>(config, fConfigManager[config["key"]],
						fConfigManager);
				// TODO: Improve
				control->SetExplicitMinSize(BSize(350, B_SIZE_UNSET));
				return control;
			}
		}
		case B_RGB_COLOR_TYPE:
		{
			GControl<BColorControl, rgb_color>* control =
				new GControl<BColorControl, rgb_color>(config, fConfigManager[config["key"]],
					fConfigManager);
			return control;
		}
		case B_OBJECT_TYPE:
		{
			BView* view = new BStringView(config["key"], config["label"]);
			BSeparatorView* separator = new BSeparatorView(view);
			BSize size = view->MinSize();
			size.width += size.width / 3.0;
			size.height *= 2.0;
			separator->SetExplicitMinSize(size);
			return separator;
		}
		default:
		{
			BString label;
			const char* setting = config["key"];
			label.SetToFormat("Can't create control for setting [%s]\n", setting);
			return new BStringView("view", label.String());
		}
	}
	return NULL;
}


template<typename T, typename POPUP>
BOptionPopUp*
ConfigWindow::_CreatePopUp(GMessage& config)
{
	auto popUp = new GControl<POPUP, T>(config, fConfigManager[config["key"]], fConfigManager);
	int32 c = 1;
	while (true) {
		BString key("option_");
		key << c;
		if (!config.Has(key.String()))
			break;
		popUp->AddOption(config[key.String()]["label"], (T)config[key.String()]["value"]);
		c++;
	}
	popUp->LoadValue(fConfigManager[config["key"]]);
	return popUp;
}
