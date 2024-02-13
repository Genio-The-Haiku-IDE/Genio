/*
 * Copyright 2024, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once


#include <SupportDefs.h>
#include <OptionPopUp.h>
#include <vector>
#include <string>

class OptionPopUpString : public BOptionPopUp {
public:
			OptionPopUpString(const char* name,
							  const char* label,
							  BMessage* message): BOptionPopUp(name,label,message){
			}

			void AddOption(const char* label, const char* value) {
				BOptionPopUp::AddOption(label, fValueList.size());
				fValueList.push_back(label);
			}
			void SetValue(const char* value)
			{
				int i=0;
				for(std::string v : fValueList) {
					if (v.compare(value) == 0) {
						BOptionPopUp::SetValue(i);
						return;
					}
					i++;
				}
			}
			const char* Value() {
				int i = BOptionPopUp::Value();
				return fValueList[i].c_str();
			}

private:
			std::vector<std::string>	fValueList;
};

