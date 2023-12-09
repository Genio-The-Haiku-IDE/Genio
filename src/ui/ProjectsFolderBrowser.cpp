/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * Copyright 2023, Nexus6 <nexus6.haiku@icloud.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "ProjectsFolderBrowser.h"

#include "ConfigManager.h"
#include "GenioApp.h"
#include "GenioWatchingFilter.h"
#include "GenioWindowMessages.h"
#include "GenioWindow.h"
#include "Log.h"
#include "ProjectFolder.h"
#include "ProjectItem.h"
#include "SwitchBranchMenu.h"
#include "TemplateManager.h"
#include "Utils.h"

#include <Alert.h>
#include <Catalog.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <Looper.h>
#include <MenuItem.h>
#include <Mime.h>
#include <NaturalCompare.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <Window.h>

#include <cstdio>
#include <functional>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ProjectsFolderBrowser"


ProjectsFolderBrowser::ProjectsFolderBrowser()
	:
	BOutlineListView("ProjectsFolderOutline", B_SINGLE_SELECTION_LIST)
{
	fGenioWatchingFilter = new GenioWatchingFilter();
	SetInvocationMessage(new BMessage(MSG_PROJECT_MENU_OPEN_FILE));
	BPrivate::BPathMonitor::SetWatchingInterface(fGenioWatchingFilter);
}


ProjectsFolderBrowser::~ProjectsFolderBrowser()
{
	BPrivate::BPathMonitor::SetWatchingInterface(nullptr);
	delete fGenioWatchingFilter;
}


ProjectItem*
ProjectsFolderBrowser::_CreateNewProjectItem(ProjectItem* parentItem, BPath path)
{
	ProjectFolder *projectFolder = parentItem->GetSourceItem()->GetProjectFolder();
	SourceItem *sourceItem = new SourceItem(path.Path());
	sourceItem->SetProjectFolder(projectFolder);
	return new ProjectItem(sourceItem);
}


ProjectItem*
ProjectsFolderBrowser::_CreatePath(BPath pathToCreate)
{
	LogTrace("Create path for %s", pathToCreate.Path());
	ProjectItem *item = GetProjectItemByPath(pathToCreate.Path());

	if (!item) {
		LogTrace("Can't find path %s", pathToCreate.Path());
		BPath parent;
		if (pathToCreate.GetParent(&parent) == B_OK) {
			ProjectItem* parentItem = _CreatePath(parent);
			LogTrace("Creating path %s", pathToCreate.Path());
			ProjectItem* newItem = _CreateNewProjectItem(parentItem, pathToCreate);

			if (AddUnder(newItem,parentItem)) {
				LogDebugF("AddUnder(%s,%s) (Parent %s)", newItem->Text(), parentItem->Text(), parent.Path());
				SortItemsUnder(parentItem, true, ProjectsFolderBrowser::_CompareProjectItems);
				Collapse(newItem);
			}
			return newItem;
		}
	}
	LogTrace("Found path [%s]", pathToCreate.Path());
	return item;
}


void
ProjectsFolderBrowser::_UpdateNode(BMessage* message)
{
	int32 opCode;
	if (message->FindInt32("opcode", &opCode) != B_OK)
		return;
	BString watchedPath;
	if (message->FindString("watched_path", &watchedPath) != B_OK)
		return;
	switch (opCode) {
		case B_ENTRY_CREATED:
		{
			BString spath;
			if (message->FindString("path", &spath) == B_OK) {
				BPath path(spath.String());
				_CreatePath(path);
			}
			break;
		}
		case B_ENTRY_REMOVED:
		{
			BString spath;
			if (message->FindString("path", &spath) != B_OK)
				break;

			LogDebug("path %s", spath.String());
			ProjectItem *item = GetProjectItemByPath(spath);
			if (!item) {
					LogError("Can't find an item to remove [%s]", spath.String());
					return;
			}
			if (item->GetSourceItem()->Type() == SourceItemType::ProjectFolderItem) {
				if (LockLooper()) {
					Select(IndexOf(item));
					Window()->PostMessage(MSG_PROJECT_MENU_CLOSE);

					// It seems not possible to track the project folder to the new
					// location outside of the watched path. So we close the project
					// and warn the user
					auto alert = new BAlert("ProjectFolderChanged", B_TRANSLATE(
						"The project folder has been deleted or moved to another location "
						"and it will be closed and unloaded from the workspace."),
						B_TRANSLATE("OK"), NULL, NULL,
						B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);
						alert->Go();

					UnlockLooper();
				}
			} else {
				RemoveItem(item);
				SortItemsUnder(Superitem(item), true, ProjectsFolderBrowser::_CompareProjectItems);
			}
			break;
		}
		case B_ENTRY_MOVED:
		{
			BString spath;
			// An item moved outside of the project folder
			if (message->GetBool("removed", false)) {
				if (message->FindString("from path", &spath) == B_OK) {
					LogDebug("from path %s",  spath.String());
					ProjectItem *item = GetProjectItemByPath(spath);
					if (!item) {
						LogError("Can't find an item to move [%s]", spath.String());
						return;
					}
					// the project folder is being renamed
					if (item->GetSourceItem()->Type() == SourceItemType::ProjectFolderItem) {
						if (LockLooper()) {
							Select(IndexOf(item));
							Window()->PostMessage(MSG_PROJECT_MENU_CLOSE);

							auto alert = new BAlert("ProjectFolderChanged",
							B_TRANSLATE("The project folder has been renamed. It will be closed and reopened automatically."),
								B_TRANSLATE("OK"), NULL, NULL,
								B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);
							alert->Go();

							// reopen project under the new name or location
							entry_ref ref;
							if (message->FindInt64("to directory", &ref.directory) == B_OK) {
								const char *name;
								message->FindInt32("device", &ref.device);
								message->FindString("name", &name);
								ref.set_name(name);
								auto msg = new BMessage(MSG_PROJECT_FOLDER_OPEN);
								msg->AddRef("refs", &ref);
								Window()->PostMessage(msg);
							}
							UnlockLooper();
						}
					} else {
						RemoveItem(item);
						SortItemsUnder(Superitem(item), true, ProjectsFolderBrowser::_CompareProjectItems);
					}
				}
			} else {
				BString oldName, newName;
				BString oldPath, newPath;
				if (message->GetBool("added")) {
					if (message->FindString("path", &newPath) == B_OK) {
						if (message->FindString("name", &newName) == B_OK) {
							const BPath destination(newPath);
							if (BEntry(newPath).IsDirectory()) {
								// a new folder moved inside the project.
								//ensure we have a parent
								BPath parent;
								destination.GetParent(&parent);
								ProjectItem *parentItem = _CreatePath(parent);
								// recursive parsing!
								_ProjectFolderScan(parentItem, newPath, parentItem->GetSourceItem()->GetProjectFolder());
								SortItemsUnder(parentItem, false, ProjectsFolderBrowser::_CompareProjectItems);
							} else {
								//Plain file
								_CreatePath(destination);
							}
						}
					}
				} else 	if (message->FindString("from name", &oldName) == B_OK) {
					if (message->FindString("name", &newName) == B_OK) {
						if (message->FindString("from path", &oldPath) == B_OK) {
							if (message->FindString("path", &newPath) == B_OK) {
								const BPath bp_oldPath(oldPath.String());
								BPath bp_oldParent;
								bp_oldPath.GetParent(&bp_oldParent);
								const BPath bp_newPath(newPath.String());
								BPath bp_newParent;
								bp_newPath.GetParent(&bp_newParent);

								// If the path remains the same except the leaf
								// then the item is being RENAMED
								// if the path changes then the item is being MOVED
								if (bp_oldParent == bp_newParent) {
									ProjectItem *item = GetProjectItemByPath(oldPath);
									if (!item) {
										LogError("Can't find an item to move oldPath[%s] -> newPath[%s]", oldPath.String(), newPath.String());
										return;
									}
									item->SetText(newName);
									item->GetSourceItem()->Rename(newPath);
									SortItemsUnder(Superitem(item), true, ProjectsFolderBrowser::_CompareProjectItems);
								} else {
									ProjectItem *item = GetProjectItemByPath(oldPath);
									if (!item) {
										LogError("Can't find an item to move oldPath [%s]", oldPath.String());
										return;
									}
									ProjectItem *destinationItem = GetProjectItemByPath(bp_newParent.Path());
									if (!item) {
										LogError("Can't find an item to move newParent [%s]", bp_newParent.Path());
										return;
									}
									bool status = RemoveItem(item);
									if (status) {
										SortItemsUnder(Superitem(item), true, ProjectsFolderBrowser::_CompareProjectItems);

										ProjectItem *newItem = _CreateNewProjectItem(item, bp_newPath);
										status = AddUnder(newItem, destinationItem);
										if (status) {
											SortItemsUnder(destinationItem, true, ProjectsFolderBrowser::_CompareProjectItems);
										}
									}
								}
							}
						}
					}
				}
			}
			break;
		}
		default:
			break;
	}
}


/* virtual */
void
ProjectsFolderBrowser::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_COPY:
		case B_CUT:
		case B_PASTE:
		case B_SELECT_ALL:
			//to avoid crash! (WIP)
			break;
		case B_PATH_MONITOR:
		{
			if (Logger::IsDebugEnabled())
				message->PrintToStream();
			_UpdateNode(message);
			SendNotices(B_PATH_MONITOR, message);
			break;
		}
		case MSG_PROJECT_MENU_OPEN_FILE:
		{
			int32 index = -1;
			if (message->FindInt32("index", &index) != B_OK) {
				LogError("(MSG_PROJECT_MENU_OPEN_FILE) Can't find index!");
				return;
			}
			ProjectItem* item = dynamic_cast<ProjectItem*>(ItemAt(index));
			if (!item) {
				LogError("(MSG_PROJECT_MENU_OPEN_FILE) Can't find item at index %d", index);
				return;
			}
			if (item->GetSourceItem()->Type() != SourceItemType::FileItem) {
				ExpandOrCollapse(item, !item->IsExpanded());
				LogDebug("(MSG_PROJECT_MENU_OPEN_FILE) ExpandOrCollapse(%s)", item->GetSourceItem()->Name().String());
				return;
			}
			BEntry entry(item->GetSourceItem()->Path());
			entry_ref ref;
			if (entry.GetRef(&ref) != B_OK) {
				LogError("(MSG_PROJECT_MENU_OPEN_FILE) Invalid entry_ref for [%s]", item->GetSourceItem()->Path().String());
				return;
			}
			BMessage msg(B_REFS_RECEIVED);
			msg.AddRef("refs", &ref);
			msg.AddBool("openWithPreferred", true);
			Window()->PostMessage(&msg);
			return;
			break;
		}
		case MSG_PROJECT_MENU_DO_RENAME_FILE:
		{
			BString newName;
			if (message->FindString("_value", &newName) == B_OK) {
				if (_RenameCurrentSelectedFile(newName) != B_OK) {
					OKAlert("Rename",
							BString(B_TRANSLATE("An error occurred attempting to rename file ")) <<
								newName, B_WARNING_ALERT);
				}
			}
			break;
		}
		case B_OBSERVER_NOTICE_CHANGE:
		{
			int32 code;
			message->FindInt32(B_OBSERVE_WHAT_CHANGE, &code);
			switch (code) {
				case MSG_NOTIFY_EDITOR_FILE_OPENED:
				case MSG_NOTIFY_EDITOR_FILE_CLOSED:
				{
					bool open = (code == MSG_NOTIFY_EDITOR_FILE_OPENED);
					BString fileName;
					message->FindString("file_name", &fileName);
					ProjectItem* item = GetProjectItemByPath(fileName);
					if (item != nullptr) {
						item->SetOpenedInEditor(open);
						Invalidate();
					}
					break;
				}
				case MSG_NOTIFY_FILE_SAVE_STATUS_CHANGED:
				{
					bool needsSave = false;
					BString fileName;
					message->FindBool("needs_save", &needsSave);
					message->FindString("file_name", &fileName);
					ProjectItem* item = GetProjectItemByPath(fileName);
					if (item != nullptr) {
						item->SetNeedsSave(needsSave);
						Invalidate();
					}
					break;
				}
				case MSG_NOTIFY_BUILDING_PHASE:
				{
					bool building = false;
					message->FindBool("building", &building);
					fIsBuilding = building;
					break;
				}
				case MSG_NOTIFY_PROJECT_SET_ACTIVE:
				{
					// Expand active project, collapse other
					if (gCFG["auto_expand_collapse_projects"]) {
						ProjectFolder* activeProject = nullptr;
						if (message->FindPointer("active_project",
							reinterpret_cast<void**>(&activeProject)) != B_OK)
							break;
						Expand(GetProjectItem(activeProject->Name()));
					}
					break;
				}
				default:
					BOutlineListView::MessageReceived(message);
					break;
			}
		}
		default:
			BOutlineListView::MessageReceived(message);
			break;
	}
}


void
ProjectsFolderBrowser::_ShowProjectItemPopupMenu(BPoint where)
{
	ProjectItem*  projectItem = GetSelectedProjectItem();
	if (!projectItem)
		return;

	ProjectFolder *project = GetProjectFromItem(projectItem);
	if (project == nullptr)
		return;

	BPopUpMenu* projectMenu = new BPopUpMenu("ProjectMenu", false, false);

	fFileNewProjectMenuItem = new TemplatesMenu(this, B_TRANSLATE("New"),
		new BMessage(MSG_PROJECT_MENU_NEW_FILE), new BMessage(MSG_SHOW_TEMPLATE_USER_FOLDER),
		TemplateManager::GetDefaultTemplateDirectory(),
		TemplateManager::GetUserTemplateDirectory(),
		TemplatesMenu::SHOW_ALL_VIEW_MODE,	true);

	fFileNewProjectMenuItem->SetEnabled(true);

	if (projectItem->GetSourceItem()->Type() == SourceItemType::ProjectFolderItem) {
		BMenuItem* closeProjectMenuItem = new BMenuItem(B_TRANSLATE("Close project"),
			new BMessage(MSG_PROJECT_MENU_CLOSE));
		BMenuItem* setActiveProjectMenuItem = new BMenuItem(B_TRANSLATE("Set active"),
			new BMessage(MSG_PROJECT_MENU_SET_ACTIVE));
		BMenuItem* projectSettingsMenuItem = new BMenuItem(B_TRANSLATE("Project settings" B_UTF8_ELLIPSIS),
			new BMessage(MSG_PROJECT_SETTINGS));
		projectMenu->AddItem(closeProjectMenuItem);
		projectMenu->AddItem(setActiveProjectMenuItem);
		projectMenu->AddItem(projectSettingsMenuItem);
		closeProjectMenuItem->SetEnabled(true);
		projectMenu->AddSeparatorItem();
		projectMenu->AddItem(new SwitchBranchMenu(this->Window(), B_TRANSLATE("Switch to branch"),
													new BMessage(MSG_GIT_SWITCH_BRANCH), project->Path()));
		projectMenu->AddSeparatorItem();
		BMenuItem* buildMenuItem = new BMenuItem(B_TRANSLATE("Build project"),
			new BMessage(MSG_BUILD_PROJECT));
		BMenuItem* cleanMenuItem = new BMenuItem(B_TRANSLATE("Clean project"),
			new BMessage(MSG_CLEAN_PROJECT));
		projectMenu->AddItem(buildMenuItem);
		projectMenu->AddItem(cleanMenuItem);
		setActiveProjectMenuItem->SetEnabled(!project->Active());
		if (!project->Active())
			setActiveProjectMenuItem->SetEnabled(true);
		if (fIsBuilding || !project->Active()) {
			projectSettingsMenuItem->SetEnabled(false);
			buildMenuItem->SetEnabled(false);
			cleanMenuItem->SetEnabled(false);
		}
	}

	projectMenu->AddItem(fFileNewProjectMenuItem);
	fFileNewProjectMenuItem->SetViewMode(TemplatesMenu::ViewMode::SHOW_ALL_VIEW_MODE);
	projectMenu->AddSeparatorItem();

	bool isFolder = projectItem->GetSourceItem()->Type() == SourceItemType::FolderItem;
	bool isFile = projectItem->GetSourceItem()->Type() == SourceItemType::FileItem;
	if (isFolder || isFile) {
		BMenuItem* deleteFileProjectMenuItem = new BMenuItem(
			isFile ? B_TRANSLATE("Delete file") : B_TRANSLATE("Delete folder"),
			new BMessage(MSG_PROJECT_MENU_DELETE_FILE));
		BMenuItem* openFileProjectMenuItem = new BMenuItem(B_TRANSLATE("Open file"),
			new BMessage(MSG_PROJECT_MENU_OPEN_FILE));
		BMenuItem* renameFileProjectMenuItem = new BMenuItem(
			isFile ? B_TRANSLATE("Rename file") : B_TRANSLATE("Rename folder"),
			new BMessage(MSG_PROJECT_MENU_RENAME_FILE));
		if (isFile)
			fFileNewProjectMenuItem->SetViewMode(TemplatesMenu::ViewMode::DISABLE_FILES_VIEW_MODE, false);
		projectMenu->AddItem(openFileProjectMenuItem);
		projectMenu->AddItem(deleteFileProjectMenuItem);
		projectMenu->AddItem(renameFileProjectMenuItem);
		deleteFileProjectMenuItem->SetEnabled(true);
		renameFileProjectMenuItem->SetEnabled(true);
		openFileProjectMenuItem->SetEnabled(isFile);
	}

	BMenuItem* showInTrackerProjectMenuItem = new BMenuItem(B_TRANSLATE("Show in Tracker"),
		new BMessage(MSG_PROJECT_MENU_SHOW_IN_TRACKER));
	BMenuItem* openTerminalProjectMenuItem = new BMenuItem(B_TRANSLATE("Open Terminal"),
		new BMessage(MSG_PROJECT_MENU_OPEN_TERMINAL));
	showInTrackerProjectMenuItem->SetEnabled(true);
	openTerminalProjectMenuItem->SetEnabled(true);
	projectMenu->AddItem(showInTrackerProjectMenuItem);
	projectMenu->AddItem(openTerminalProjectMenuItem);

	projectMenu->SetTargetForItems(Window());
	projectMenu->Go(ConvertToScreen(where), true);
}


ProjectItem*
ProjectsFolderBrowser::GetProjectItem(const BString& projectName) const
{
	const int32 countItems = CountItems();
	for (int32 i = 0; i < countItems; i++) {
		ProjectItem *item = dynamic_cast<ProjectItem*>(ItemAt(i));
		if (item != nullptr && item->Text() == projectName)
			return item;
	}

	return nullptr;
}


ProjectItem*
ProjectsFolderBrowser::GetProjectItemAt(const int32& index) const
{
	const int32 countItems = CountItems();
	if (index < 0 || index >= countItems)
		return nullptr;

	return dynamic_cast<ProjectItem*>(ItemAt(index));
}


// TODO:
// Optimize the search under a specific ProjectItem, tipically a
// superitem (ProjectFolder)
ProjectItem*
ProjectsFolderBrowser::GetProjectItemByPath(BString const& path) const
{
	const int32 countItems = FullListCountItems();
	for (int32 i = 0; i < countItems; i++) {
		ProjectItem *item = dynamic_cast<ProjectItem*>(FullListItemAt(i));
		if (item != nullptr && item->GetSourceItem()->Path() == path)
			return item;
	}

	return nullptr;
}


ProjectItem*
ProjectsFolderBrowser::GetSelectedProjectItem() const
{
	const int32 selection = CurrentSelection();
	if (selection < 0)
		return nullptr;

	return dynamic_cast<ProjectItem*>(ItemAt(selection));
}


ProjectFolder*
ProjectsFolderBrowser::GetProjectFromSelectedItem() const
{
	return GetProjectFromItem(GetSelectedProjectItem());
}


ProjectFolder*
ProjectsFolderBrowser::GetProjectFromItem(ProjectItem* item) const
{
	if (item == nullptr)
		return nullptr;

	ProjectFolder *project;
	if (item->GetSourceItem()->Type() == SourceItemType::ProjectFolderItem) {
		project = static_cast<ProjectFolder*>(item->GetSourceItem());
	} else {
		project = static_cast<ProjectFolder*>(item->GetSourceItem()->GetProjectFolder());
	}

	return project;
}


BString const
ProjectsFolderBrowser::GetSelectedProjectFileFullPath() const
{
	ProjectItem* selectedProjectItem = GetSelectedProjectItem();
	// if (selectedProjectItem->GetSourceItem()->Type() == SourceItemType::FileItem)
		return selectedProjectItem->GetSourceItem()->Path();
	// else
		// return "";
}


status_t
ProjectsFolderBrowser::_RenameCurrentSelectedFile(const BString& new_name)
{
	status_t status = B_NOT_INITIALIZED;
	ProjectItem *item = GetSelectedProjectItem();
	if (item) {
		BEntry entry(item->GetSourceItem()->Path());
		if (entry.Exists())
			status = entry.Rename(new_name, false);
	}
	return status;
}


/* virtual */
void
ProjectsFolderBrowser::AttachedToWindow()
{
	BOutlineListView::AttachedToWindow();
	BOutlineListView::SetTarget((BHandler*)this, Window());

	if (Window()->LockLooper()) {
		Window()->StartWatching(this, MSG_NOTIFY_EDITOR_FILE_OPENED);
		Window()->StartWatching(this, MSG_NOTIFY_EDITOR_FILE_CLOSED);
		Window()->StartWatching(this, MSG_NOTIFY_BUILDING_PHASE);
		Window()->StartWatching(this, MSG_NOTIFY_FILE_SAVE_STATUS_CHANGED);
		Window()->StartWatching(this, MSG_NOTIFY_PROJECT_SET_ACTIVE);
		Window()->UnlockLooper();
	}
}


/* virtual */
void
ProjectsFolderBrowser::DetachedFromWindow()
{
	BOutlineListView::DetachedFromWindow();

	if (Window()->LockLooper()) {
		Window()->StopWatching(this, MSG_NOTIFY_EDITOR_FILE_OPENED);
		Window()->StopWatching(this, MSG_NOTIFY_EDITOR_FILE_CLOSED);
		Window()->StopWatching(this, MSG_NOTIFY_FILE_SAVE_STATUS_CHANGED);
		Window()->StopWatching(this, MSG_NOTIFY_BUILDING_PHASE);
		Window()->StopWatching(this, MSG_NOTIFY_PROJECT_SET_ACTIVE);
		Window()->UnlockLooper();
	}
}


/* virtual */
void
ProjectsFolderBrowser::MouseDown(BPoint where)
{
	int32 buttons = -1;

	BMessage* message = Looper()->CurrentMessage();
	 if (message != NULL)
		 message->FindInt32("buttons", &buttons);

	if (buttons == B_MOUSE_BUTTON(1)) {
		return BOutlineListView::MouseDown(where);
	} else 	if ( buttons == B_MOUSE_BUTTON(2)) {
		int32 index = IndexOf(where);
		if (index >= 0) {
			Select(index);
			_ShowProjectItemPopupMenu(where);
		}
	}
}


void
ProjectsFolderBrowser::MouseMoved(BPoint point, uint32 transit, const BMessage* message)
{
	if ((transit == B_ENTERED_VIEW) || (transit == B_INSIDE_VIEW)) {
		auto index = IndexOf(point);
		if (index >= 0) {
			ProjectItem *item = reinterpret_cast<ProjectItem*>(ItemAt(index));
			if (item->HasToolTip()) {
				SetToolTip(item->GetToolTipText());
			} else {
				SetToolTip("");
			}
		}
	} else {
	}
}


void
ProjectsFolderBrowser::ProjectFolderDepopulate(ProjectFolder* project)
{
	status_t status = BPrivate::BPathMonitor::StopWatching(project->Path(), BMessenger(this));
	if ( status != B_OK ){
		LogErrorF("Can't StopWatching! path [%s] error[%s]", project->Path(), strerror(status));
	}
	ProjectItem* listItem = GetProjectItemByPath(project->Path());
	if (listItem)
		RemoveItem(listItem);
	else
		LogErrorF("Can't find ProjectItem for path [%s]", project->Path().String());
	Invalidate();
}


void
ProjectsFolderBrowser::ProjectFolderPopulate(ProjectFolder* project)
{
	ProjectItem *projectItem = NULL;
	_ProjectFolderScan(projectItem, project->Path(), project);

	LockLooper();
	SortItemsUnder(projectItem, false, ProjectsFolderBrowser::_CompareProjectItems);
	UnlockLooper();
	status_t status = BPrivate::BPathMonitor::StartWatching(project->Path(),
			B_WATCH_RECURSIVELY, BMessenger(this));
	if (status != B_OK ) {
		LogErrorF("Can't StartWatching! path [%s] error[%s]", project->Path(), strerror(status));
	}

	update_mime_info(project->Path(), true, false, B_UPDATE_MIME_INFO_NO_FORCE);
}


void
ProjectsFolderBrowser::_ProjectFolderScan(ProjectItem* item, BString const& path, ProjectFolder *projectFolder)
{
	LockLooper();

	ProjectItem *newItem;
	if (item != nullptr) {
		SourceItem *sourceItem = new SourceItem(path);
		sourceItem->SetProjectFolder(projectFolder);
		newItem = new ProjectItem(sourceItem);
		AddUnder(newItem, item);
		Collapse(newItem);
	} else {
		newItem = item = new ProjectItem(projectFolder);
		AddItem(newItem);
	}

	BEntry entry(path);
	BEntry parent;
	BPath parentPath;
	// Check if there's a Jamfile or makefile in the root path
	// of the project
	if (entry.IsFile() && entry.GetParent(&parent) == B_OK
		&& parent.GetPath(&parentPath) == B_OK
		&& projectFolder->Path().Compare(parentPath.Path()) == 0) {
		// guess builder type
		// TODO: do it for real: set a flag or setting in project
		// TODO: move this away from here, into a specialized class
		// and maybe into plugins
		if (strcasecmp(entry.Name(), "makefile") == 0) {
			// builder: make
			projectFolder->SetGuessedBuilder("make");
			LogInfo("Guessed builder: make");
		} else if (strcasecmp(entry.Name(), "jamfile") == 0) {
			// builder: jam
			projectFolder->SetGuessedBuilder("jam");
			LogInfo("Guessed builder: jam");
		}
	} else if (entry.IsDirectory()) {
		BPath _currentPath;
		BDirectory dir(&entry);
		BEntry nextEntry;
		while (dir.GetNextEntry(&nextEntry, false) != B_ENTRY_NOT_FOUND) {
			nextEntry.GetPath(&_currentPath);
			UnlockLooper();
			_ProjectFolderScan(newItem, _currentPath.Path(), projectFolder);
			LockLooper();
		}
	}
	UnlockLooper();
}


int
ProjectsFolderBrowser::_CompareProjectItems(const BListItem* a, const BListItem* b)
{
	if (a == b)
		return 0;

	const ProjectItem* A = dynamic_cast<const ProjectItem*>(a);
	const ProjectItem* B = dynamic_cast<const ProjectItem*>(b);
	const char* nameA = A->Text();
	SourceItem *itemA = A->GetSourceItem();
	const char* nameB = B->Text();
	SourceItem *itemB = B->GetSourceItem();

	if (itemA->Type()==SourceItemType::FolderItem && itemB->Type()==SourceItemType::FileItem) {
		return -1;
	}

	if (itemA->Type()==SourceItemType::FileItem && itemB->Type()==SourceItemType::FolderItem) {
		return 1;
	}

	if (nameA == NULL) {
		return 1;
	}

	if (nameB == NULL) {
		return -1;
	}

	// Natural order sort
	if (nameA != NULL && nameB != NULL)
		return BPrivate::NaturalCompare(nameA, nameB);

	return 0;
}


void
ProjectsFolderBrowser::ProjectFolderPopulateThread(ProjectFolder* item)
{
	BString taskName;
	taskName << item->Name() << "PopulateProject (" << item->Name() << ") task";
	shared_ptr<Genio::Task::Task<status_t>> task = make_shared<Genio::Task::Task<status_t>>
	(
		taskName.String(),
		BMessenger(this),
		std::bind
		(
			&ProjectsFolderBrowser::ProjectFolderPopulate,
			this,
			item
		)
	);

	item->AssignTask(task);
	task->Run();
}


void
ProjectsFolderBrowser::SelectionChanged()
{
	GenioWindow *window = (GenioWindow*)this->Window();
	window->UpdateMenu();
}


void
ProjectsFolderBrowser::InitRename(ProjectItem *item)
{
	item->InitRename(this, new BMessage(MSG_PROJECT_MENU_DO_RENAME_FILE));
	Invalidate();
}
