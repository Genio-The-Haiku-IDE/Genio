/*
 * Copyright 2023, Andrea Anzani 
 * Copyright 2023, Nexus6 
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "ProjectBrowser.h"

#include "ActionManager.h"
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
#include <MessageRunner.h>
#include <Mime.h>
#include <NaturalCompare.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <StringList.h>
#include <Window.h>

#include <cassert>
#include <cstdio>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ProjectsFolderBrowser"

const uint32 kTick = 'tick';

static BMessageRunner* sAnimationTickRunner;

ProjectBrowser::ProjectBrowser()
	:
	BOutlineListView("ProjectsFolderOutline", B_SINGLE_SELECTION_LIST),
	fFileNewProjectMenuItem(nullptr),
	fIsBuilding(false)
{
	fGenioWatchingFilter = new GenioWatchingFilter();
	SetInvocationMessage(new BMessage(MSG_PROJECT_MENU_OPEN_FILE));
	BPrivate::BPathMonitor::SetWatchingInterface(fGenioWatchingFilter);
}


ProjectBrowser::~ProjectBrowser()
{
	BPrivate::BPathMonitor::SetWatchingInterface(nullptr);
	delete fGenioWatchingFilter;
}


ProjectItem*
ProjectBrowser::_CreateNewProjectItem(ProjectItem* parentItem, BPath path)
{
	ProjectFolder *projectFolder = parentItem->GetSourceItem()->GetProjectFolder();
	SourceItem *sourceItem = new SourceItem(path.Path());
	sourceItem->SetProjectFolder(projectFolder);
	return new ProjectItem(sourceItem);
}


ProjectItem*
ProjectBrowser::_CreatePath(BPath pathToCreate)
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
				SortItemsUnder(parentItem, true, ProjectBrowser::_CompareProjectItems);
				Collapse(newItem);
			}
			return newItem;
		}
	}
	LogTrace("Found path [%s]", pathToCreate.Path());
	return item;
}


void
ProjectBrowser::_UpdateNode(BMessage* message)
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
					BMessage closePrj(MSG_PROJECT_MENU_CLOSE);
					closePrj.AddPointer("project", item->GetSourceItem());
					Window()->PostMessage(&closePrj);

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
				SortItemsUnder(Superitem(item), true, ProjectBrowser::_CompareProjectItems);
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
							BMessage closePrj(MSG_PROJECT_MENU_CLOSE);
							closePrj.AddPointer("project", item->GetSourceItem());
							Window()->PostMessage(&closePrj);

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
								BMessage msg(MSG_PROJECT_FOLDER_OPEN);
								msg.AddRef("refs", &ref);
								Window()->PostMessage(&msg);
							}
							UnlockLooper();
						}
					} else {
						RemoveItem(item);
						SortItemsUnder(Superitem(item), true, ProjectBrowser::_CompareProjectItems);
					}
				}
			} else {
				BString oldName, newName;
				BString oldPath, newPath;
				if (message->GetBool("added")) {
					if (message->FindString("path", &newPath) == B_OK) {
						if (message->FindString("name", &newName) == B_OK) {
							const BPath destination(newPath);
							BEntry newPathEntry(newPath);
							if (newPathEntry.IsDirectory()) {
								// a new folder moved inside the project.
								//ensure we have a parent
								BPath parent;
								destination.GetParent(&parent);
								ProjectItem *parentItem = _CreatePath(parent);
								// recursive parsing!
								entry_ref entryRef;
								newPathEntry.GetRef(&entryRef);
								_ProjectFolderScan(parentItem, &entryRef, parentItem->GetSourceItem()->GetProjectFolder());
								SortItemsUnder(parentItem, false, ProjectBrowser::_CompareProjectItems);
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
									entry_ref newRef;
									if (get_ref_for_path(newPath, &newRef) == B_OK) {
										item->SetText(newName);
										item->GetSourceItem()->UpdateEntryRef(newRef);
										SortItemsUnder(Superitem(item), true, ProjectBrowser::_CompareProjectItems);
									} else {
										LogError("Can't find ref for newPath[%s]", newPath.String());
										return;
									}
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
										SortItemsUnder(Superitem(item), true, ProjectBrowser::_CompareProjectItems);

										ProjectItem *newItem = _CreateNewProjectItem(item, bp_newPath);
										status = AddUnder(newItem, destinationItem);
										if (status) {
											SortItemsUnder(destinationItem, true, ProjectBrowser::_CompareProjectItems);
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
ProjectBrowser::MessageReceived(BMessage* message)
{
	switch (message->what) {
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
			if (item == nullptr) {
				LogError("(MSG_PROJECT_MENU_OPEN_FILE) Can't find item at index %d", index);
				return;
			}
			if (item->GetSourceItem()->Type() != SourceItemType::FileItem) {
				ExpandOrCollapse(item, !item->IsExpanded());
				LogDebug("(MSG_PROJECT_MENU_OPEN_FILE) ExpandOrCollapse(%s)", item->GetSourceItem()->Name().String());
				return;
			}

			BMessage msg(B_REFS_RECEIVED);
			msg.AddRef("refs", item->GetSourceItem()->EntryRef());
			msg.AddBool("openWithPreferred", true);
			Window()->PostMessage(&msg);
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
		case kTick:
		{
			if (fIsBuilding) {
				// TODO: Only invalidate the project item
				ProjectItem::TickAnimation();
				Invalidate();
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
				default:
					BOutlineListView::MessageReceived(message);
					break;
			}
			break;
		}
		case MSG_BROWSER_SELECT_ITEM:
		{
			ProjectItem* item = (ProjectItem*)message->GetPointer("parent_item", nullptr);
			entry_ref ref;
			if (item != nullptr && message->FindRef("ref", &ref) == B_OK) {
				int32 howMany = BOutlineListView::CountItemsUnder(item, true);
				for (int32 i = 0; i < howMany; i++) {
					ProjectItem* subItem = (ProjectItem*)BOutlineListView::ItemUnderAt(item, true, i);
					if (*subItem->GetSourceItem()->EntryRef() == ref) {
						Select(IndexOf(subItem));
						ScrollToSelection();
						bool doRename = message->GetBool("rename", false);
						if (doRename) {
							InitRename(subItem);
						}
						break;
					}
				}
			}
			break;
		}
		default:
			BOutlineListView::MessageReceived(message);
			break;
	}
}


void
ProjectBrowser::_ShowProjectItemPopupMenu(BPoint where)
{
	ProjectItem* projectItem = GetSelectedProjectItem();
	if (projectItem == nullptr)
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
		BMessage* closePrj = new BMessage(MSG_PROJECT_MENU_CLOSE);
		closePrj->AddPointer("project", (void*)project);
		BMenuItem* closeProjectMenuItem = new BMenuItem(B_TRANSLATE("Close project"), closePrj);

		BMessage* setActive = new BMessage(MSG_PROJECT_MENU_SET_ACTIVE);
		setActive->AddPointer("project", (void*)project);
		BMenuItem* setActiveProjectMenuItem = new BMenuItem(B_TRANSLATE("Set active"), setActive);

		BMessage* projSettings = new BMessage(MSG_PROJECT_SETTINGS);
		projSettings->AddPointer("project", (void*)project);
		BMenuItem* projectSettingsMenuItem = new BMenuItem(B_TRANSLATE("Project settings" B_UTF8_ELLIPSIS),
			projSettings);

		projectMenu->AddItem(closeProjectMenuItem);
		projectMenu->AddItem(setActiveProjectMenuItem);
		projectMenu->AddItem(projectSettingsMenuItem);
		closeProjectMenuItem->SetEnabled(true);
		projectMenu->AddSeparatorItem();
		projectMenu->AddItem(new SwitchBranchMenu(Window(), B_TRANSLATE("Switch to branch"),
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
			buildMenuItem->SetEnabled(false);
			cleanMenuItem->SetEnabled(false);
		}
	}

	projectMenu->AddItem(fFileNewProjectMenuItem);
	fFileNewProjectMenuItem->SetViewMode(TemplatesMenu::ViewMode::SHOW_ALL_VIEW_MODE);
	projectMenu->AddSeparatorItem();

	SelectionChanged();

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

		projectMenu->AddItem(openFileProjectMenuItem);
		projectMenu->AddItem(deleteFileProjectMenuItem);
		projectMenu->AddItem(renameFileProjectMenuItem);
		deleteFileProjectMenuItem->SetEnabled(true);
		renameFileProjectMenuItem->SetEnabled(true);
		openFileProjectMenuItem->SetEnabled(isFile);
	}

	BMessage* refMessage = new BMessage();
	refMessage->AddRef("ref", projectItem->GetSourceItem()->EntryRef());
	ActionManager::AddItem(MSG_PROJECT_MENU_SHOW_IN_TRACKER, projectMenu, refMessage);

	BMessage* refMessage2 = new BMessage(*refMessage);
	ActionManager::AddItem(MSG_PROJECT_MENU_OPEN_TERMINAL, projectMenu, refMessage2);

	projectMenu->SetTargetForItems(Window());
	projectMenu->Go(ConvertToScreen(where), true);
}


// TODO:
// Optimize the search under a specific ProjectItem, tipically a
// superitem (ProjectFolder)
ProjectItem*
ProjectBrowser::GetProjectItemByPath(BString const& path) const
{
	entry_ref ref;
	status_t status = get_ref_for_path(path.String(), &ref);
	if (status != B_OK) {
		LogTraceF("invalid path %s (%s)", path.String(), strerror(status));
		return nullptr;
	}

	const int32 countItems = FullListCountItems();
	for (int32 i = 0; i < countItems; i++) {
		ProjectItem *item = dynamic_cast<ProjectItem*>(FullListItemAt(i));
		if (item != nullptr && *item->GetSourceItem()->EntryRef() == ref)
			return item;
	}

	return nullptr;
}


ProjectItem*
ProjectBrowser::GetSelectedProjectItem() const
{
	const int32 selection = CurrentSelection();
	if (selection < 0)
		return nullptr;

	return dynamic_cast<ProjectItem*>(ItemAt(selection));
}


ProjectItem*
ProjectBrowser::GetProjectItemForProject(ProjectFolder* folder)
{
	assert(fProjectProjectItemList.CountItems() == CountProjects());

	for (int32 i = 0; i < CountProjects(); i++) {
		if (ProjectAt(i) == folder)
			return fProjectProjectItemList.ItemAt(i);
	}
	return nullptr;
}


ProjectFolder*
ProjectBrowser::GetProjectFromSelectedItem() const
{
	return GetProjectFromItem(GetSelectedProjectItem());
}


ProjectFolder*
ProjectBrowser::GetProjectFromItem(ProjectItem* item) const
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


const entry_ref*
ProjectBrowser::GetSelectedProjectFileRef() const
{
	ProjectItem* selectedProjectItem = GetSelectedProjectItem();
	return selectedProjectItem->GetSourceItem()->EntryRef();
}


status_t
ProjectBrowser::_RenameCurrentSelectedFile(const BString& new_name)
{
	status_t status = B_NOT_INITIALIZED;
	ProjectItem *item = GetSelectedProjectItem();
	if (item != nullptr) {
		BEntry entry(item->GetSourceItem()->EntryRef());
		if (entry.Exists())
			status = entry.Rename(new_name, false);
	}
	return status;
}


/* virtual */
void
ProjectBrowser::AttachedToWindow()
{
	BOutlineListView::AttachedToWindow();
	BOutlineListView::SetTarget((BHandler*)this, Window());

	if (Window()->LockLooper()) {
		Window()->StartWatching(this, MSG_NOTIFY_EDITOR_FILE_OPENED);
		Window()->StartWatching(this, MSG_NOTIFY_EDITOR_FILE_CLOSED);
		Window()->StartWatching(this, MSG_NOTIFY_BUILDING_PHASE);
		Window()->StartWatching(this, MSG_NOTIFY_FILE_SAVE_STATUS_CHANGED);
		Window()->UnlockLooper();
	}

	ProjectItem::InitAnimationIcons();

	BMessage message(kTick);
	if (sAnimationTickRunner == nullptr)
		sAnimationTickRunner = new BMessageRunner(BMessenger(this), &message, bigtime_t(100000));

	if (CountItems() == 0)
		AddItem(new StyledItem(B_TRANSLATE("Drop folder here to add as new project")));
}


/* virtual */
void
ProjectBrowser::DetachedFromWindow()
{
	delete sAnimationTickRunner;
	sAnimationTickRunner = nullptr;

	ProjectItem::DisposeAnimationIcons();

	BOutlineListView::DetachedFromWindow();

	if (Window()->LockLooper()) {
		Window()->StopWatching(this, MSG_NOTIFY_EDITOR_FILE_OPENED);
		Window()->StopWatching(this, MSG_NOTIFY_EDITOR_FILE_CLOSED);
		Window()->StopWatching(this, MSG_NOTIFY_FILE_SAVE_STATUS_CHANGED);
		Window()->StopWatching(this, MSG_NOTIFY_BUILDING_PHASE);
		Window()->UnlockLooper();
	}
}


/* virtual */
void
ProjectBrowser::MouseDown(BPoint where)
{
	int32 buttons = -1;
	BMessage* message = Looper()->CurrentMessage();
	if (message != NULL)
		message->FindInt32("buttons", &buttons);

	BOutlineListView::MouseDown(where);
	if ( buttons == B_MOUSE_BUTTON(2))
		_ShowProjectItemPopupMenu(where);
}


void
ProjectBrowser::MouseMoved(BPoint point, uint32 transit, const BMessage* message)
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
	}
}


void
ProjectBrowser::ProjectFolderDepopulate(ProjectFolder* project)
{
	const BString projectPath = project->Path();
	status_t status = BPrivate::BPathMonitor::StopWatching(projectPath, BMessenger(this));
	if (status != B_OK) {
		LogErrorF("Can't StopWatching! path [%s] error[%s]", projectPath.String(), strerror(status));
	}
	ProjectItem* listItem = GetProjectItemForProject(project);
	if (listItem)
		RemoveItem(listItem);
	else
		LogErrorF("Can't find ProjectItem for path [%s]", projectPath.String());

	fProjectProjectItemList.RemoveItem(listItem);
	fProjectList.RemoveItem(project);

	if (CountItems() == 0)
		AddItem(new StyledItem(B_TRANSLATE("Drop folder here to add as new project")));

	Invalidate();
}


void
ProjectBrowser::ProjectFolderPopulate(ProjectFolder* project)
{
	if (CountItems() == 1 && dynamic_cast<ProjectItem*>(ItemAt(0)) == nullptr)
		RemoveItem(ItemAt(0));

	ProjectItem *projectItem = _ProjectFolderScan(nullptr, project->EntryRef(), project);
	// TODO: here we are ordering ALL the elements (maybe and option could prevent ordering the projects)
	SortItemsUnder(nullptr, false, ProjectBrowser::_CompareProjectItems);

	const BString projectPath = project->Path();
	update_mime_info(projectPath, true, false, B_UPDATE_MIME_INFO_NO_FORCE);

	assert(projectItem && project);

	fProjectList.AddItem(project);
	fProjectProjectItemList.AddItem(projectItem);

	Invalidate();
	status_t status = BPrivate::BPathMonitor::StartWatching(projectPath,
			B_WATCH_RECURSIVELY, BMessenger(this));
	if (status != B_OK)
		LogErrorF("Can't StartWatching! path [%s] error[%s]", projectPath.String(), ::strerror(status));
}


ProjectItem*
ProjectBrowser::_ProjectFolderScan(ProjectItem* item, const entry_ref* ref, ProjectFolder *projectFolder)
{
	ProjectItem *newItem;
	if (item != nullptr) {
		SourceItem *sourceItem = new SourceItem(*ref);
		sourceItem->SetProjectFolder(projectFolder);
		newItem = new ProjectItem(sourceItem);
		AddUnder(newItem, item);
		Collapse(newItem);
	} else {
		newItem = new ProjectItem(projectFolder);
		AddItem(newItem);
	}

	BEntry entry(ref);
	BEntry parent;
	BPath parentPath;
	if (entry.IsDirectory()) {
		BDirectory dir(&entry);
		entry_ref nextRef;
		while (dir.GetNextRef(&nextRef) != B_ENTRY_NOT_FOUND) {
			_ProjectFolderScan(newItem, &nextRef, projectFolder);
		}
	}

	return newItem;
}


int
ProjectBrowser::_CompareProjectItems(const BListItem* a, const BListItem* b)
{
	if (a == b)
		return 0;

	const ProjectItem* A = dynamic_cast<const ProjectItem*>(a);
	const ProjectItem* B = dynamic_cast<const ProjectItem*>(b);
	const char* nameA = A->Text();
	SourceItem *itemA = A->GetSourceItem();
	const char* nameB = B->Text();
	SourceItem *itemB = B->GetSourceItem();

	if (itemA->Type() == SourceItemType::FolderItem && itemB->Type() == SourceItemType::FileItem) {
		return -1;
	}

	if (itemA->Type() == SourceItemType::FileItem && itemB->Type() == SourceItemType::FolderItem) {
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
ProjectBrowser::SelectionChanged()
{
	ProjectItem* selected = GetSelectedProjectItem();
	if (selected != nullptr) {
		GenioWindow *window = (GenioWindow*)Window();
		BEntry entry(selected->GetSourceItem()->EntryRef());
		if (entry.IsFile()) {
			// If this is a file, get the parent directory
			entry.GetParent(&entry);
		}
		entry_ref newRef;
		entry.GetRef(&newRef);
		window->UpdateMenu(selected, &newRef);

		if (fFileNewProjectMenuItem != nullptr)
			fFileNewProjectMenuItem->SetSender(selected, &newRef);
	}
}


void
ProjectBrowser::InitRename(ProjectItem *item)
{
	//ensure the item is visible!
	if (Superitem(item)->IsExpanded() == false)
		Expand(Superitem(item));

	Select(IndexOf(item));
	ScrollToSelection();

	item->InitRename(this, new BMessage(MSG_PROJECT_MENU_DO_RENAME_FILE));
	Invalidate();
}


int32
ProjectBrowser::CountProjects() const
{
	return fProjectList.CountItems();
}


ProjectFolder*
ProjectBrowser::ProjectAt(int32 index) const
{
	return fProjectList.ItemAt(index);
}


ProjectFolder*
ProjectBrowser::ProjectByPath(const BString& fullPath) const
{
	for (int32 i = 0; i < CountProjects(); i++) {
		ProjectFolder* project = ProjectAt(i);
		if (project != nullptr && project->Path() == fullPath)
			return project;
	}
	return nullptr;
}

void
ProjectBrowser::SelectProjectAndScroll(ProjectFolder* projectFolder)
{
	ProjectItem* item = GetProjectItemForProject(projectFolder);
	if (item != nullptr) {
		Select(IndexOf(item));
		ScrollToSelection();
	}
}

void
ProjectBrowser::SelectNewItemAndScrollDelayed(ProjectItem* parent, const entry_ref ref)
{
	// Let's select the new created file.
	// just send a message to the ProjectBrowser with the new ref
	// .. after some milliseconds..
	BMessage selectMessage(MSG_BROWSER_SELECT_ITEM);
	selectMessage.AddPointer("parent_item", parent);
	selectMessage.AddRef("ref", &ref);
	selectMessage.AddBool("rename", true);
	BMessageRunner::StartSending(BMessenger(this), &selectMessage, 300, 1);
}


/*
	This method implements an 'optimized' version for searching with a ref on the browser.
	The basic implementaion parses all the items.. (could be useful in PathMonitor procedures)
*/
void
ProjectBrowser::SelectItemByRef(ProjectFolder* project, const entry_ref& ref)
{
	ProjectItem* projectItem = GetProjectItemForProject(project);
	if (projectItem == nullptr)
		return;

	BPath path(&ref);
	BString fullpath = path.Path();
	if (fullpath.StartsWith(project->Path().String())) {
		fullpath = fullpath.RemoveFirst(project->Path().String());

		bool found = false;
		BStringList list;
		fullpath.Split("/", true, list);
		for (int32 i = 0; i < list.CountStrings(); i++) {
			for (int32 j = 0; j < CountItemsUnder(projectItem, true); j++) {
				ProjectItem* pItem = (ProjectItem*)ItemUnderAt(projectItem, true, j);
				if (pItem->GetSourceItem()->Name().Compare(list.StringAt(i)) == 0) {
					Expand(projectItem);

					projectItem = pItem;
					found = (i == list.CountStrings() - 1);
					break;
				}
			}
		}
		if (found) {
			LogInfoF("Found ProjectItem! %s", projectItem->GetSourceItem()->Name().String());
			Select(IndexOf(projectItem));
			ScrollToSelection();
		}
	}
}


const BObjectList<ProjectFolder>*
ProjectBrowser::GetProjectList() const
{
	return &fProjectList;
}
