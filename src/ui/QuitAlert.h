/*
 * Copyright 2016-2018 Kacper Kasper 
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef QUITALERT_H
#define QUITALERT_H


#include <ObjectList.h>
#include <Window.h>

#include <string>
#include <vector>


class BButton;
class BCheckBox;
class BStringView;
class BScrollView;

class EditorWindow;


class QuitAlert : public BWindow {
public:
								QuitAlert(const std::vector<std::string> &unsavedFiles);
								~QuitAlert();

	void						MessageReceived(BMessage* message);
	virtual void				Show();
	std::vector<bool>			Go();
private:
	enum Actions {
		SAVE_ALL		= 'sval',
		SAVE_SELECTED	= 'svsl',
		DONT_SAVE		= 'dnsv'
	};
	const std::vector<std::string>	fUnsavedFiles;
	BScrollView*					fScrollView;
	BStringView*					fMessageString;
	BButton*						fSaveAll;
	BButton*						fSaveSelected;
	BButton*						fDontSave;
	BButton*						fCancel;
	std::vector<BCheckBox*>			fCheckboxes;
	std::vector<bool>				fAlertValue;
	sem_id							fAlertSem;

	void							_InitInterface();
};


#endif // QUITALERT_H
