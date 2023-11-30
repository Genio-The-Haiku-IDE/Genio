/*
 * Copyright 2023, Nexus6 <nexus6.haiku@icloud.com>
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

#include "GException.h"
#include "GMessage.h"
#include "Log.h"
#include "Utils.h"

namespace Genio::UI {

	template <typename T>
	  struct is_iterator {
	  static char test(...);

	  template <typename U,
		typename=typename std::iterator_traits<U>::difference_type,
		typename=typename std::iterator_traits<U>::pointer,
		typename=typename std::iterator_traits<U>::reference,
		typename=typename std::iterator_traits<U>::value_type,
		typename=typename std::iterator_traits<U>::iterator_category
	  > static long test(U&&);

	  constexpr static bool value = std::is_same<decltype(test(std::declval<T>())),long>::value;
	};

	template <typename T>
	class OptionList : public BMenuField {
	public:
		OptionList(const char* name,
			const char* label,
			const char* text,
			bool fixed_size = true,
			uint32 flags = B_WILL_DRAW | B_NAVIGABLE)
			:
			BMenuField(name, label, fMenu = new BPopUpMenu(text), fixed_size, flags),
			fMessenger(nullptr)
		{
			fMenu->SetLabelFromMarked(true);
		}

		~OptionList()
		{
			delete fMessenger;
		}

		void MakeEmpty()
		{
			for (int32 index = fMenu->CountItems() - 1; index > -1; index--)
				fMenu->RemoveItem(index);
		}


		void AddItem(BString &name, T value, uint32 command,
			bool invokeItemMessage = true, bool marked = false)
		{
			status_t status;
			auto message = new BMessage(command);
			if constexpr (std::is_pointer<T>::value) {
				status = message->AddPointer("value", value);
			} else if constexpr (std::is_same<BString, T>::value) {
				status = message->AddString("value", (BString)value);
			} else {
				status = message->AddData("value", B_ANY_TYPE, &value, sizeof(value), true);
			}
			if (status != B_OK)
				throw GException(status, strerror(status));
			LogInfo("item name: %s type: %s", name.String(), typeid(value).name());
			// message->PrintToStream();
			auto menu_item = new BMenuItem(name.String(), message);
			menu_item->SetMarked(marked);
			if ((fMessenger != nullptr) && (fMessenger->IsValid())) {
				menu_item->SetTarget(*fMessenger);
				if (marked && invokeItemMessage) {
					LogInfo("Item %s marked as selected. Message sent out", name.String());
					fMessenger->SendMessage(message);
				}
			}
			fMenu->AddItem(menu_item);
		}

		template <class C>
		void AddIterator(C const& list,
							uint32 command,
							const auto& get_name_lambda,
							bool invokeItemMessage = true,
							const auto& set_mark_lambda = nullptr)
		{
			LogInfo("AddIterator command:%d", command);
			typename C::const_iterator sit;
			for(T element : list)
			{
				auto name = get_name_lambda(element);
				if constexpr (std::is_same<BString, T>::value) {
					LogInfo("AddIterator name:%s value:%s", name.String(), ((BString)element).String());
				} else {
					LogInfo("AddIterator name:%s value: N/A", name.String());
				}
				AddItem(name, element, command, invokeItemMessage, set_mark_lambda(element));
			}
		}

		template <class C>
		void AddList(C const& list,
							uint32 command,
							const auto& get_name_lambda,
							bool invokeItemMessage = true,
							const auto& set_mark_lambda = nullptr)
		{
			LogInfo("AddList command:%d", command);
			auto count = list->CountItems();
			for (int index = 0; index < count; index++) {
				T element = list->ItemAt(index);
				auto name = get_name_lambda(element);
				if constexpr (std::is_same<BString, T>::value) {
					LogInfo("AddList name:%s value:%s", name.String(), ((BString)element).String());
				} else {
					LogInfo("AddList name:%s value: N/A", name.String());
				}
				AddItem(name, element, command, invokeItemMessage, set_mark_lambda(element));
			}
		}

		void SetTarget(BHandler *handler)
		{
			fMenu->SetTargetForItems(handler);
			fMessenger = new BMessenger(handler);
		}


		void SetTarget(BMessenger messenger)
		{
			fMenu->SetTargetForItems(messenger);
			fMessenger = new BMessenger(messenger);
		}

	private:
		BPopUpMenu *fMenu;
		BMessenger *fMessenger;
	};

}