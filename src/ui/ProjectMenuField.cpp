/*
 * Copyright 2025, Stefano Ceccherini
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "ProjectMenuField.h"

#include <algorithm>

#include <Catalog.h>
#include <NaturalCompare.h>

#include "GenioWindow.h"
#include "GenioWindowMessages.h"
#include "ProjectBrowser.h"
#include "ProjectFolder.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ProjectMenuField"

using namespace Genio::UI;

ProjectMenuField::ProjectMenuField(const char* name, int32 what, uint32 flags)
	:
	OptionList(name, B_TRANSLATE("Project:"),
		B_TRANSLATE("Choose project" B_UTF8_ELLIPSIS), true, flags),
	fWhat(what)
{
}


/* virtual */
void
ProjectMenuField::AttachedToWindow()
{
	OptionList::AttachedToWindow();
	if (Window()->LockLooper()) {
		Window()->StartWatching(this, MSG_NOTIFY_PROJECT_LIST_CHANGED);
		Window()->StartWatching(this, MSG_NOTIFY_PROJECT_SET_ACTIVE);
		Window()->UnlockLooper();
	}
}


/* virtual */
void
ProjectMenuField::DetachedFromWindow()
{
	OptionList::DetachedFromWindow();
	if (Window()->LockLooper()) {
		Window()->StopWatching(this, MSG_NOTIFY_PROJECT_LIST_CHANGED);
		Window()->StopWatching(this, MSG_NOTIFY_PROJECT_SET_ACTIVE);
		Window()->UnlockLooper();
	}
}


/* virtual */
void
ProjectMenuField::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_OBSERVER_NOTICE_CHANGE:
		{
			int32 code;
			message->FindInt32(B_OBSERVE_WHAT_CHANGE, &code);
			switch (code) {
				case MSG_NOTIFY_PROJECT_LIST_CHANGED:
					_HandleProjectListChanged(message);
					break;
				case MSG_NOTIFY_PROJECT_SET_ACTIVE:
					_HandleActiveProjectChanged(message);
					break;
				default:
					OptionList::MessageReceived(message);
					break;
			}
			break;
		}
		default:
			OptionList::MessageReceived(message);
			break;
	}
}


void
ProjectMenuField::_HandleProjectListChanged(const BMessage* message)
{
	// The logic here: save the currently selected project, empty the list
	// then rebuild the list and try to reselect the previously selected project.
	// otherwise select the active project.
	BMenu* projectMenu = Menu();

	BString selectedProject;
	BMenuItem* item = projectMenu->FindMarked();
	if (item != nullptr) {
		selectedProject = item->Label();
	}

	Window()->BeginViewTransaction();

	MakeEmpty();

	ProjectBrowser* projectBrowser = gMainWindow->GetProjectBrowser();
	for (int32 p = 0; p < projectBrowser->CountProjects(); p++) {
		ProjectFolder* project = projectBrowser->ProjectAt(p);
		if (project == nullptr)
			break;
		bool marked = project->Name() == selectedProject;
		AddItem(project->Name(), project->Path(), fWhat, true, marked);
	}

	if (projectMenu->FindMarked() == nullptr) {
		const ProjectFolder* activeProject = gMainWindow->GetActiveProject();
		BMenuItem* item = nullptr;
		if (activeProject != nullptr)
			item = projectMenu->FindItem(activeProject->Name());
		else
			item = projectMenu->ItemAt(0);
		if (item != nullptr) {
			item->SetMarked(true);
			item->Messenger().SendMessage(item->Message());
		}
	}

	Window()->EndViewTransaction();
}


void
ProjectMenuField::_HandleActiveProjectChanged(const BMessage* message)
{
	BString selectedProjectName;
	BMenuItem* item = Menu()->FindMarked();
	if (item != nullptr) {
		selectedProjectName = item->Label();
	}

	BString activeProjectName = message->GetString("active_project_name");
	bool changed = false;
	if (::strcmp(activeProjectName, selectedProjectName) != 0) {
		item = Menu()->FindItem(activeProjectName);
		if (item != nullptr) {
			changed = !item->IsMarked();
			item->SetMarked(true);
		}
	}
	if (changed) {
		if (item != nullptr)
			item->Messenger().SendMessage(item->Message());
	}
}

