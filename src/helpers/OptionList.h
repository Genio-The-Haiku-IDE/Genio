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
			: BMenuField(name, label, fMenu = new BPopUpMenu(text), fixed_size, flags)
		{
			fMenu->SetLabelFromMarked(true);
		}


		void MakeEmpty()
		{
			for (int32 index = fMenu->CountItems() - 1; index > -1; index--)
				fMenu->RemoveItem(index);
		}


		void AddItem(BString &name, T value, uint32 command, bool marked = false)
		{
			auto message = new BMessage(command);
			if constexpr (std::is_pointer<T>::value) {
				message->AddPointer("value", value);
			} if constexpr (std::is_same<BString, T>::value) {
				message->AddString("value", value);
			} else {
				message->AddData("value", B_ANY_TYPE, &value, sizeof(value), true);
			}
			printf("*OPTIONLIST*\n");
			printf(name.String());
			printf("\n");
			printf(typeid(value).name());
			printf("\n");
			message->PrintToStream();
			auto menu_item = new BMenuItem(name.String(), message);
			menu_item->SetMarked(marked);
			fMenu->AddItem(menu_item);
		}

		template <class C>
		void AddIterator(C const& list,
							uint32 command,
							const auto& get_name_lambda,
							const auto& set_mark_lambda = nullptr)
		{
			LogInfo("AddIterator command:%d", command);
			typename C::const_iterator sit;
			for(T element : list)
			{
				auto name = get_name_lambda(element);
				LogInfo("AddIterator name:%s value:%s", name.String(), element);
				AddItem(name, element, command, set_mark_lambda(element));
			}
		}

		template <class C>
		void AddList(C const& list,
							uint32 command,
							const auto& get_name_lambda,
							const auto& set_mark_lambda = nullptr)
		{
			LogInfo("AddList command:%d", command);
			auto count = list->CountItems();
			for (int index = 0; index < count; index++) {
				T element = list->ItemAt(index);
				auto name = get_name_lambda(element);
				LogInfo("AddList: name:%s value:%s", name.String(), element);
				AddItem(name, element, command, set_mark_lambda(element));
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
		BPopUpMenu *fMenu;
	};

}