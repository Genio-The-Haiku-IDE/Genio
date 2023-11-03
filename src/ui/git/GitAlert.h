/*
 * Copyright 2023 Nexus6 
 * Parts taken from QuitAlert.cpp
 * Copyright 2016-2018 Kacper Kasper 
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#pragma once

#include <ColumnListView.h>
#include <ColumnTypes.h>
#include <ObjectList.h>
#include <Window.h>

#include <string>
#include <vector>

class BButton;
class BCheckBox;
class BStringView;
class BScrollView;

class EditorWindow;


class GitAlert : public BWindow {
public:
								GitAlert(const char *title, const char *message,
											const std::vector<std::string> &files);
								~GitAlert();

	void						MessageReceived(BMessage* message);
	virtual void				Show();
	void						Go();

private:
	const BString					fTitle;
	const BString					fMessage;
	const std::vector<std::string>	fFiles;
	BStringView*					fMessageString;
	BScrollView*					fScrollView;
	BButton*						fOK;
	std::vector<BStringView*>		fFileStringView;
	sem_id							fAlertSem;

	void							_InitInterface();
};