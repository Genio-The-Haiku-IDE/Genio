/*
 * Copyright 2023, Nexus6 
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once


#include <MenuField.h>
#include <MenuItem.h>
#include <Messenger.h>
#include <ObjectList.h>
#include <PopUpMenu.h>
#include <SupportDefs.h>

#include <iterator>

namespace Genio::UI {

	template <typename T>
	class OptionList : public BMenuField {
	public:
		OptionList(const char* name,
			const char* label,
			const char* text,
			bool fixed_size = true,
			uint32 flags = B_WILL_DRAW | B_NAVIGABLE)
			: BMenuField(name, label, fMenu = new BPopUpMenu(text), fixed_size, flags)
		{
			fMenu->SetLabelFromMarked(true);
		}


		void MakeEmpty()
		{
			for (int32 index = fMenu->CountItems() - 1; index > -1; index--)
				fMenu->RemoveItem(index);
		}


		void AddItem(BString &name, T value,
			uint32 command, bool marked = false)
		{
			// GMessage *message = new GMessage(command);
			// (*message)["value"] = (void*)value;
			// message->what = command;
			BMessage *message = new BMessage(command);
			message->AddPointer("value", (void*)value);
			// BeDC("Genio").DumpBMessage(message);
			auto menu_item = new BMenuItem(name, message);
			menu_item->SetMarked(marked);
			fMenu->AddItem(menu_item);
		}

		template <typename C>
		void AddList(C const& list,
							uint32 command,
							const auto& get_name_lambda,
							const auto& set_mark_lambda = nullptr)
							// std::function<BString(T *)> get_name_lambda,
							// std::function<bool(T *)> set_mark_lambda = nullptr)
		{
			auto count = list->CountItems();
			for (int index = 0; index < count; index++) {
				auto list_item = list->ItemAt(index);
				auto name = get_name_lambda(list_item);
				AddItem(name, list_item, command,
									// (set_mark_lambda != nullptr) ? set_mark_lambda(list_item) : false);
									set_mark_lambda(list_item));
			}
		}


		template <typename C>
		void Menu_AddContainer(C const& list,
								uint32 command,
								const auto& get_name_lambda,
								const auto& set_mark_lambda = nullptr)
								// std::function<BString(T&)> get_name_lambda,
								// std::function<bool(T&)> set_mark_lambda = nullptr)
		{
			 typename C::const_iterator sit;
			 for(auto& element : list)
			 {
				T list_item = element;
				auto name = get_name_lambda(list_item);
				AddItem<T>(name, &list_item, command,
									// (set_mark_lambda != nullptr) ? set_mark_lambda(list_item) : false);
									set_mark_lambda(list_item));
			 }
		}


		void Select(T element)
		{
			for (int32 index = 0; index < fMenu->CountItems(); index++) {
				auto item = fMenu->ItemAt(index);
				auto message = item->Message();
				message->PrintToStream();
				auto value = message->GetPointer("value");
				if ((value != nullptr) && (value == element))
					item->SetMarked(index);
			}
		}

		void SetTarget(BHandler *handler)
		{
			fMenu->SetTargetForItems(handler);
		}


		void SetTarget(BMessenger &messenger)
		{
			fMenu->SetTargetForItems(messenger);
		}

	private:
		BPopUpMenu*		fMenu;

		auto FindItem(const auto& match)
		{
			for (int32 index = 0; index < fMenu->CountItems(); index++) {
				auto item = fMenu->ItemAt(index);
				auto message = item->Message();
				T element = message->GetPointer("value");
				if ((element != nullptr) && (match(element) == element))
					item->SetMarked(index);
			}
		}
	};

}