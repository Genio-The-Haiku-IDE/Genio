/*
 * Copyright 2024, Andrea Anzani 
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once


#include <OptionPopUp.h>
#include <string>
#include <vector>

class OptionPopUpString : public BOptionPopUp {
public:
			OptionPopUpString(const char* name,
							  const char* label,
							  BMessage* message)
				:
				BOptionPopUp(name,label,message)
			{
			}

			void AddOption(const char* label, const char* value)
			{
				BOptionPopUp::AddOption(label, fValueList.size());
				fValueList.push_back(label);
			}

			void SetValue(int32 value)
			{
				BOptionPopUp::SetValue(value);
			}

			void SetValue(const char* value)
			{
				int i = 0;
				for (std::string v : fValueList) {
					if (v.compare(value) == 0) {
						BOptionPopUp::SetValue(i);
						return;
					}
					i++;
				}
			}
			const char* Value() const {
				int i = BOptionPopUp::Value();
				return fValueList[i].c_str();
			}
private:
			std::vector<std::string> fValueList;
};
