/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * Copyright 2023, Nexus6 <nexus6.haiku@icloud.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "ProjectBrowser.h"

#include <cstdio>
#include <algorithm>

#include <Catalog.h>
#include <Debug.h>
#include <GroupLayoutBuilder.h>
#include <LayoutBuilder.h>
#include <MessageRunner.h>
#include <Mime.h>
#include <NaturalCompare.h>
#include <OutlineListView.h>
#include <PopUpMenu.h>
#include <ScrollView.h>
#include <StringList.h>
#include <StringView.h>

#include "ActionManager.h"
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
#include "TemplatesMenu.h"
#include "Utils.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ProjectsFolderBrowser"

const uint32 kTick = 'tick';

static BMessageRunner* sAnimationTickRunner;

class ProjectOutlineListView : public BOutlineListView {
public:
			ProjectOutlineListView();
	virtual ~ProjectOutlineListView();

	void MouseDown(BPoint where) override;
	void MouseMoved(BPoint point, uint32 transit, const BMessage* message) override;
	void AttachedToWindow() override;
	void DetachedFromWindow() override;
	void MessageReceived(BMessage* message) override;
	void KeyDown(const char* bytes, int32 numBytes) override;
	void SelectionChanged() override;

	ProjectItem* ProjectItemAt(int32 index) const;
	ProjectItem* GetSelectedProjectItem() const;

	static int CompareProjectItems(const BListItem* a, const BListItem* b);

private:
	void _ShowProjectItemPopupMenu(BPoint where);
};


const pattern kStripePattern = {
	0xcc, 0x66, 0x33, 0x99,
	0xcc, 0x66, 0x33, 0x99
};

class ProjectDropView : public BView {
public:
	ProjectDropView()
		:
		BView("ProjectDropView", B_WILL_DRAW|B_FRAME_EVENTS|B_FULL_UPDATE_ON_RESIZE)
	{
		BString dropLabel = B_TRANSLATE("Drop folder here");
		BStringView* stringView = new BStringView("drop", dropLabel.String());
		BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
			.AddGroup(B_VERTICAL, 3)
				.AddGlue()
				.AddStrut(1)
				.Add(stringView)
				.AddGlue()
			.End();

		stringView->SetAlignment(B_ALIGN_CENTER);

		BFont font;
		font.SetFace(B_CONDENSED_FACE);
		stringView->SetFont(&font, B_FONT_FACE);

		// TODO: These should not be needed, but without them the
		// splitview which separates editor from the left pane doesn't move at all
		SetExplicitMinSize(BSize(0, B_SIZE_UNSET));
		SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	}

	void Draw(BRect updateRect) override
	{
		SetDrawingMode(B_OP_ALPHA);
		SetLowColor(0, 0, 0);
		float tint = B_DARKEN_2_TINT;
		const int32 kBrightnessBreakValue = 126;
		const rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
		if (base.Brightness() >= kBrightnessBreakValue)
			tint = B_LIGHTEN_2_TINT;

		SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),	tint));
		BRect innerRect = Bounds().InsetByCopy(10, 10);
		FillRect(innerRect, B_SOLID_LOW);
		StrokeRect(innerRect);
		FillRect(innerRect.InsetBySelf(3, 3), kStripePattern);
	}
};


// ProjectBrowser
ProjectBrowser::ProjectBrowser()
	:
	BView("Project browser", B_WILL_DRAW|B_FRAME_EVENTS)
{
	fOutlineListView = new ProjectOutlineListView();
	ProjectDropView* projectDropView = new ProjectDropView();

	BScrollView* scrollView = new BScrollView("scrollview", fOutlineListView,
		B_FRAME_EVENTS | B_WILL_DRAW, true, true, B_FANCY_BORDER);

	BLayoutBuilder::Cards<>(this)
		.Add(scrollView)
		.Add(projectDropView)
		.SetVisibleItem(int32(0));

	fGenioWatchingFilter = new GenioWatchingFilter();
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

			if (fOutlineListView->AddUnder(newItem,parentItem)) {
				LogDebugF("AddUnder(%s,%s) (Parent %s)", newItem->Text(), parentItem->Text(), parent.Path());
				fOutlineListView->SortItemsUnder(parentItem, true,
					ProjectOutlineListView::CompareProjectItems);
				fOutlineListView->Collapse(newItem);
			}
			return newItem;
		}
	}
	LogTrace("Found path [%s]", pathToCreate.Path());
	return item;
}


void
ProjectBrowser::_RemovePath(BString spath)
{
	LogDebug("path %s", spath.String());
	ProjectItem *item = GetProjectItemByPath(spath);
	if (!item) {
		LogError("Can't find an item to remove [%s]", spath.String());
		return;
	}
	if (item->GetSourceItem()->Type() == SourceItemType::ProjectFolderItem) {
		if (LockLooper()) {
			fOutlineListView->Select(fOutlineListView->IndexOf(item));
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
		fOutlineListView->RemoveItem(item);
		fOutlineListView->SortItemsUnder(fOutlineListView->Superitem(item),
			true, ProjectOutlineListView::CompareProjectItems);
	}
}


void
ProjectBrowser::_HandleEntryMoved(BMessage* message)
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
					fOutlineListView->Select(fOutlineListView->IndexOf(item));
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
				fOutlineListView->RemoveItem(item);
				fOutlineListView->SortItemsUnder(fOutlineListView->Superitem(item),
					true, ProjectOutlineListView::CompareProjectItems);
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
						fOutlineListView->SortItemsUnder(parentItem, false,
								ProjectOutlineListView::CompareProjectItems);
					} else {
						//Plain file
						_CreatePath(destination);
					}
				}
			}
		} else if (message->FindString("from name", &oldName) == B_OK) {
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
								fOutlineListView->SortItemsUnder(fOutlineListView->Superitem(item),
									true, ProjectOutlineListView::CompareProjectItems);
								if (item->IsSelected())
									fOutlineListView->ScrollToSelection();
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
							if (!destinationItem) {
								LogError("Can't find an item to move newParent [%s]", bp_newParent.Path());
								return;
							}
							bool status = fOutlineListView->RemoveItem(item);
							if (status) {
								fOutlineListView->SortItemsUnder(
									fOutlineListView->Superitem(item), true,
										ProjectOutlineListView::CompareProjectItems);

								ProjectItem *newItem = _CreateNewProjectItem(item, bp_newPath);
								status = fOutlineListView->AddUnder(newItem, destinationItem);
								if (status) {
									fOutlineListView->SortItemsUnder(destinationItem, true,
										ProjectOutlineListView::CompareProjectItems);
								}
							}
						}
					}
				}
			}
		}
	}
}


void
ProjectBrowser::_UpdateNode(BMessage* message)
{
	int32 opCode;
	if (message->FindInt32("opcode", &opCode) != B_OK)
		return;

	if (!message->HasString("watched_path"))
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
			if (message->FindString("path", &spath) == B_OK) {
				_RemovePath(spath);
			}
			break;
		}
		case B_ENTRY_MOVED:
			_HandleEntryMoved(message);
			break;
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
			ProjectTitleItem::TickAnimation();
			for(ProjectItem* titleItem: fProjectProjectItemList) {
				if (titleItem->GetSourceItem()->GetProjectFolder()->IsBuilding()) {
					int32 itemIndex = fOutlineListView->IndexOf(titleItem);
					fOutlineListView->InvalidateItem(itemIndex);
				}
			}
			break;
		}
		case B_OBSERVER_NOTICE_CHANGE:
		{
			int32 code;
			message->FindInt32(B_OBSERVE_WHAT_CHANGE, &code);
			switch (code) {
				case MSG_NOTIFY_PROJECT_SET_ACTIVE:
					if (gCFG["auto_expand_collapse_projects"]) {
						// Expand active project, collapse other
						BString activeProject = message->GetString("active_project_name", nullptr);
						ExpandProjectCollapseOther(activeProject);
					}
					fOutlineListView->Invalidate();
					break;
				case MSG_NOTIFY_EDITOR_FILE_OPENED:
				case MSG_NOTIFY_EDITOR_FILE_CLOSED:
				{
					bool open = (code == MSG_NOTIFY_EDITOR_FILE_OPENED);
					BString fileName;
					message->FindString("file_name", &fileName);
					ProjectItem* item = GetProjectItemByPath(fileName);
					if (item != nullptr) {
						item->SetOpenedInEditor(open);
						fOutlineListView->Invalidate();
					}
					break;
				}
				case MSG_NOTIFY_EDITOR_FILE_SELECTED:
				{
					entry_ref ref;
					if (message->FindRef("ref", &ref) != B_OK)
						break;
					const ProjectFolder* project =
						reinterpret_cast<const ProjectFolder*>(message->GetPointer("project", nullptr));
					SelectItemByRef(project, ref);
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
						fOutlineListView->Invalidate();
					}
					break;
				}
				case MSG_NOTIFY_BUILDING_PHASE:
				{
					// TODO: no longer needed
					bool building = false;
					message->FindBool("building", &building);
					fOutlineListView->Invalidate();
					break;
				}
				case kMsgProjectSettingsUpdated:
				{
					const ProjectFolder* project
						= reinterpret_cast<const ProjectFolder*>(message->GetPointer("project_folder", nullptr));
					if (project == nullptr) {
						LogError("ProjectBrowser: Update project configuration message without a project folder pointer!");
						if (Logger::IsDebugEnabled()) {
							message->PrintToStream();
						}
						break;
					}
					// Save project settings
					const_cast<ProjectFolder*>(project)->SaveSettings();

					BString key(message->GetString("key", ""));
					if (key.IsEmpty())
						break;
					if (key == "color") {
						fOutlineListView->Invalidate();
					}
					break;
				}
				default:
					BView::MessageReceived(message);
					break;
			}
			break;
		}
		case MSG_BROWSER_SELECT_ITEM:
		{
			const BListItem* item = reinterpret_cast<const BListItem*>
				(message->GetPointer("parent_item", nullptr));
			entry_ref ref;
			if (item != nullptr && message->FindRef("ref", &ref) == B_OK) {
				int32 howMany = fOutlineListView->CountItemsUnder(const_cast<BListItem*>(item), true);
				for (int32 i = 0; i < howMany; i++) {
					ProjectItem* subItem = static_cast<ProjectItem*>
						(fOutlineListView->ItemUnderAt(const_cast<BListItem*>(item), true, i));
					if (*subItem->GetSourceItem()->EntryRef() == ref) {
						fOutlineListView->Select(fOutlineListView->IndexOf(subItem));
						fOutlineListView->ScrollToSelection();
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
		case B_SIMPLE_DATA:
		{
			entry_ref ref;
			int32 refsCount = 0;
			while (message->FindRef("refs", refsCount++, &ref) == B_OK) {
				if (BEntry(&ref, true).IsDirectory()) {
					BMessage openProjectMessage(MSG_PROJECT_FOLDER_OPEN);
					openProjectMessage.AddRef("refs", &ref);
					Window()->PostMessage(&openProjectMessage);
				}
			}
			// TODO: is falling-through correct ?
		}
		default:
			BView::MessageReceived(message);
			break;
	}
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

	const int32 countItems = fOutlineListView->FullListCountItems();
	for (int32 i = 0; i < countItems; i++) {
		ProjectItem *item = dynamic_cast<ProjectItem*>(fOutlineListView->FullListItemAt(i));
		if (item != nullptr && *item->GetSourceItem()->EntryRef() == ref)
			return item;
	}

	return nullptr;
}


ProjectItem*
ProjectBrowser::GetSelectedProjectItem() const
{
	return fOutlineListView->GetSelectedProjectItem();
}


ProjectItem*
ProjectBrowser::GetProjectItemForProject(const ProjectFolder* folder) const
{
	ASSERT(fProjectProjectItemList.size() == (size_t)CountProjects());

	for (int32 i = 0; i < CountProjects(); i++) {
		if (ProjectAt(i) == folder)
			return fProjectProjectItemList[i];
	}
	return nullptr;
}


ProjectFolder*
ProjectBrowser::GetProjectFromSelectedItem() const
{
	return GetProjectFromItem(GetSelectedProjectItem());
}


ProjectFolder*
ProjectBrowser::GetProjectFromItem(const ProjectItem* item) const
{
	if (item == nullptr)
		return nullptr;

	return item->GetSourceItem()->GetProjectFolder();
}


const entry_ref*
ProjectBrowser::GetSelectedProjectFileRef() const
{
	ProjectItem* selectedProjectItem = GetSelectedProjectItem();
	if (selectedProjectItem == nullptr)
		return nullptr;

	return selectedProjectItem->GetSourceItem()->EntryRef();
}


ProjectItem*
ProjectBrowser::GetItemByRef(const ProjectFolder* project, const entry_ref& ref) const
{
	ProjectItem* projectItem = GetProjectItemForProject(project);
	if (projectItem == nullptr)
		return nullptr;

	bool found = false;
	BPath path(&ref);
	BString fullpath = path.Path();
	if (fullpath.StartsWith(project->Path().String())) {
		fullpath = fullpath.RemoveFirst(project->Path().String());

		BStringList list;
		fullpath.Split("/", true, list);
		for (int32 i = 0; i < list.CountStrings(); i++) {
			for (int32 j = 0; j < fOutlineListView->CountItemsUnder(projectItem, true); j++) {
				ProjectItem* pItem = (ProjectItem*)fOutlineListView->ItemUnderAt(projectItem, true, j);
				if (pItem->GetSourceItem()->Name().Compare(list.StringAt(i)) == 0) {
					fOutlineListView->Expand(projectItem);
					projectItem = pItem;
					found = (i == list.CountStrings() - 1);
					break;
				}
			}
		}
	}
	if (!found)
		return nullptr;

	return projectItem;
}


status_t
ProjectBrowser::_RenameCurrentSelectedFile(const BString& newName)
{
	status_t status = B_NOT_INITIALIZED;
	ProjectItem *item = GetSelectedProjectItem();
	if (item != nullptr) {
		BEntry entry(item->GetSourceItem()->EntryRef());
		if (entry.Exists()) {
			status = entry.Rename(newName, false);
		}
	}
	return status;
}


/* virtual */
void
ProjectBrowser::AttachedToWindow()
{
	BView::AttachedToWindow();

	if (Window()->LockLooper()) {
		Window()->StartWatching(this, MSG_NOTIFY_EDITOR_FILE_OPENED);
		Window()->StartWatching(this, MSG_NOTIFY_EDITOR_FILE_CLOSED);
		Window()->StartWatching(this, MSG_NOTIFY_EDITOR_FILE_SELECTED);
		Window()->StartWatching(this, MSG_NOTIFY_BUILDING_PHASE);
		Window()->StartWatching(this, MSG_NOTIFY_FILE_SAVE_STATUS_CHANGED);
		Window()->StartWatching(this, MSG_NOTIFY_PROJECT_SET_ACTIVE);
		be_app->StartWatching(this, kMsgProjectSettingsUpdated);
		Window()->UnlockLooper();
	}

	ProjectTitleItem::InitAnimationIcons();

	BMessage message(kTick);
	if (sAnimationTickRunner == nullptr)
		sAnimationTickRunner = new BMessageRunner(BMessenger(this), &message, bigtime_t(100000));

	if (fOutlineListView->CountItems() == 0)
		static_cast<BCardLayout*>(GetLayout())->SetVisibleItem(int32(1));
}


/* virtual */
void
ProjectBrowser::DetachedFromWindow()
{
	delete sAnimationTickRunner;
	sAnimationTickRunner = nullptr;

	ProjectTitleItem::DisposeAnimationIcons();

	if (Window()->LockLooper()) {
		Window()->StopWatching(this, MSG_NOTIFY_EDITOR_FILE_OPENED);
		Window()->StopWatching(this, MSG_NOTIFY_EDITOR_FILE_CLOSED);
		Window()->StopWatching(this, MSG_NOTIFY_EDITOR_FILE_SELECTED);
		Window()->StopWatching(this, MSG_NOTIFY_FILE_SAVE_STATUS_CHANGED);
		Window()->StopWatching(this, MSG_NOTIFY_BUILDING_PHASE);
		Window()->StopWatching(this, MSG_NOTIFY_PROJECT_SET_ACTIVE);
		be_app->StopWatching(this, kMsgProjectSettingsUpdated);
		Window()->UnlockLooper();
	}
	BView::DetachedFromWindow();
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
		fOutlineListView->RemoveItem(listItem);
	else
		LogErrorF("Can't find ProjectItem for path [%s]", projectPath.String());

	auto it = std::find(fProjectProjectItemList.begin(), fProjectProjectItemList.end(), listItem);
	if (it != fProjectProjectItemList.end()) {
		fProjectProjectItemList.erase(it);
	}

	auto it2 = std::find(fProjectList.begin(), fProjectList.end(), project);
	if (it2 != fProjectList.end()) {
		fProjectList.erase(it2);
	}

	if (fOutlineListView->CountItems() == 0)
		static_cast<BCardLayout*>(GetLayout())->SetVisibleItem(int32(1));

	Invalidate();
}


void
ProjectBrowser::ProjectFolderPopulate(ProjectFolder* project)
{
	ASSERT(project != nullptr);

	if (fOutlineListView->CountItems() == 0)
		static_cast<BCardLayout*>(GetLayout())->SetVisibleItem(int32(0));

	ProjectItem *projectItem = _ProjectFolderScan(nullptr, project->EntryRef(), project);
	// TODO: here we are ordering ALL the elements (maybe and option could prevent ordering the projects)
	fOutlineListView->SortItemsUnder(nullptr, false, ProjectOutlineListView::CompareProjectItems);

	const BString projectPath = project->Path();
	update_mime_info(projectPath, true, false, B_UPDATE_MIME_INFO_NO_FORCE);

	ASSERT(projectItem != nullptr);

	fProjectList.push_back(project);
	fProjectProjectItemList.push_back(projectItem);

	Invalidate();
	status_t status = BPrivate::BPathMonitor::StartWatching(projectPath,
			B_WATCH_RECURSIVELY, BMessenger(this));
	if (status != B_OK)
		LogErrorF("Can't StartWatching! path [%s] error[%s]", projectPath.String(), ::strerror(status));
}


void
ProjectBrowser::ExpandProjectCollapseOther(const BString& project)
{
	for (int32 i = 0; i < CountProjects(); i++) {
		ProjectFolder* prj = ProjectAt(i);
		ProjectItem* item = GetProjectItemForProject(prj);
		if (prj->Name() == project)
			fOutlineListView->Expand(item);
		else
			fOutlineListView->Collapse(item);
	}
}


ProjectItem*
ProjectBrowser::_ProjectFolderScan(ProjectItem* item, const entry_ref* ref, ProjectFolder *projectFolder)
{
	ProjectItem *newItem;
	if (item != nullptr) {
		SourceItem *sourceItem = new SourceItem(*ref);
		sourceItem->SetProjectFolder(projectFolder);
		newItem = new ProjectItem(sourceItem);
		fOutlineListView->AddUnder(newItem, item);
		fOutlineListView->Collapse(newItem);
	} else {
		// Add project title
		newItem = new ProjectTitleItem(projectFolder);
		fOutlineListView->AddItem(newItem);
	}

	BEntry entry(ref);
	if (entry.IsDirectory()) {
		BDirectory dir(&entry);
		entry_ref nextRef;
		while (dir.GetNextRef(&nextRef) != B_ENTRY_NOT_FOUND) {
			_ProjectFolderScan(newItem, &nextRef, projectFolder);
		}
	}

	return newItem;
}


void
ProjectBrowser::InitRename(ProjectItem *item)
{
	//ensure the item is visible!
	if (fOutlineListView->Superitem(item)->IsExpanded() == false)
		fOutlineListView->Expand(fOutlineListView->Superitem(item));

	fOutlineListView->Select(fOutlineListView->IndexOf(item));
	fOutlineListView->ScrollToSelection();

	item->InitRename(fOutlineListView, new BMessage(MSG_PROJECT_MENU_DO_RENAME_FILE));
	Invalidate();
}


int32
ProjectBrowser::CountProjects() const
{
	return fProjectList.size();
}


ProjectFolder*
ProjectBrowser::ProjectAt(int32 index) const
{
	try {
		return fProjectList.at(index);
	} catch (...) {
		LogError("ProjectBrowser::ProjectAt() called with invalid index %ld!", index);
		return nullptr;
	}
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
ProjectBrowser::SelectProjectAndScroll(const ProjectFolder* projectFolder)
{
	ProjectItem* item = GetProjectItemForProject(projectFolder);
	if (item != nullptr) {
		fOutlineListView->Select(fOutlineListView->IndexOf(item));
		fOutlineListView->ScrollToSelection();
	}
}


void
ProjectBrowser::SelectNewItemAndScrollDelayed(const ProjectItem* parent, const entry_ref ref)
{
	// Let's select the new created file.
	// just send a message to the ProjectBrowser with the new ref
	// .. after some milliseconds..

	// the selected item initiating this is not a folder or project but a file.
	if (parent && parent->GetSourceItem() && parent->GetSourceItem()->Type() == FileItem) {
		parent = static_cast<ProjectItem*>(fOutlineListView->Superitem(parent));
	}

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
ProjectBrowser::SelectItemByRef(const ProjectFolder* project, const entry_ref& ref)
{
	ProjectItem* projectItem = GetItemByRef(project, ref);

	if (projectItem != nullptr) {
		LogInfoF("Found ProjectItem! %s", projectItem->GetSourceItem()->Name().String());
		fOutlineListView->Select(fOutlineListView->IndexOf(projectItem));
		fOutlineListView->ScrollToSelection();
	}
}


const ProjectFolderList*
ProjectBrowser::GetProjectList() const
{
	return &fProjectList;
}


// ProjectOutlineListView
ProjectOutlineListView::ProjectOutlineListView()
	:
	BOutlineListView("ProjectBrowserOutline", B_SINGLE_SELECTION_LIST)
{
}


/* virtual */
ProjectOutlineListView::~ProjectOutlineListView()
{
}


/* virtual */
void
ProjectOutlineListView::MouseDown(BPoint where)
{
	int32 buttons = -1;
	BMessage* message = Looper()->CurrentMessage();
	if (message != NULL)
		message->FindInt32("buttons", &buttons);

	BOutlineListView::MouseDown(where);
	if (buttons == B_MOUSE_BUTTON(2))
		_ShowProjectItemPopupMenu(where);
}


/* virtual */
void
ProjectOutlineListView::MouseMoved(BPoint point, uint32 transit, const BMessage* message)
{
	BOutlineListView::MouseMoved(point, transit, message);

	if ((transit == B_ENTERED_VIEW) || (transit == B_INSIDE_VIEW)) {
		auto index = IndexOf(point);
		if (index >= 0) {
			ProjectItem *item = ProjectItemAt(index);
			if (item->HasToolTip()) {
				SetToolTip(item->GetToolTipText());
			} else {
				SetToolTip("");
			}
		}
	}
}


/* virtual */
void
ProjectOutlineListView::AttachedToWindow()
{
	BOutlineListView::AttachedToWindow();
	BOutlineListView::SetTarget(Window());
}


/* virtual */
void
ProjectOutlineListView::DetachedFromWindow()
{
	BOutlineListView::DetachedFromWindow();
}


/* virtual */
void
ProjectOutlineListView::MessageReceived(BMessage* message)
{
	BOutlineListView::MessageReceived(message);
}


/* virtual */
void
ProjectOutlineListView::KeyDown(const char* bytes, int32 numBytes)
{
	if (bytes[0] == B_DELETE) {
		BMessage deleteFile(MSG_PROJECT_MENU_DELETE_FILE);
		ProjectItem* selected = GetSelectedProjectItem();
		if (selected != nullptr) {
			const entry_ref* ref = selected->GetSourceItem()->EntryRef();
			deleteFile.AddRef("ref", ref);
			Window()->PostMessage(&deleteFile);
		}
	}
	BOutlineListView::KeyDown(bytes, numBytes);
}


/* virtual */
void
ProjectOutlineListView::SelectionChanged()
{
	BOutlineListView::SelectionChanged();
	ProjectItem* selected = GetSelectedProjectItem();
	if (selected != nullptr) {
		GenioWindow *window = gMainWindow;
		BEntry entry(selected->GetSourceItem()->EntryRef());
		if (entry.IsFile()) {
			entry_ref fileRef;
			entry.GetRef(&fileRef);
			BMessage* invocationMessage = new BMessage(MSG_PROJECT_MENU_OPEN_FILE);
			invocationMessage->AddRef("refs", &fileRef);
			invocationMessage->AddBool("openWithPreferred", true);
			SetInvocationMessage(invocationMessage);

			// If this is a file, get the parent directory
			entry.GetParent(&entry);
		} else
			SetInvocationMessage(nullptr);

		entry_ref newRef;
		entry.GetRef(&newRef);
		window->UpdateMenu(selected, &newRef);
	}
}


ProjectItem*
ProjectOutlineListView::ProjectItemAt(int32 index) const
{
	return static_cast<ProjectItem*>(ItemAt(index));
}


ProjectItem*
ProjectOutlineListView::GetSelectedProjectItem() const
{
	const int32 selection = CurrentSelection();
	if (selection < 0)
		return nullptr;

	return ProjectItemAt(selection);
}


int
ProjectOutlineListView::CompareProjectItems(const BListItem* a, const BListItem* b)
{
	if (a == b)
		return 0;

	const ProjectItem* A = static_cast<const ProjectItem*>(a);
	const ProjectItem* B = static_cast<const ProjectItem*>(b);

	const char* nameA = A->Text();
	const auto itemAType = A->GetSourceItem()->Type();

	const char* nameB = B->Text();
	const auto itemBType = B->GetSourceItem()->Type();

	if (itemAType == SourceItemType::FolderItem && itemBType == SourceItemType::FileItem) {
		return -1;
	}

	if (itemAType == SourceItemType::FileItem && itemBType == SourceItemType::FolderItem) {
		return 1;
	}

	if (nameA == nullptr) {
		return 1;
	}

	if (nameB == nullptr) {
		return -1;
	}

	// Natural order sort
	return BPrivate::NaturalCompare(nameA, nameB);
}


void
ProjectOutlineListView::_ShowProjectItemPopupMenu(BPoint where)
{
	// TODO: This duplicates some code in ProjectBrowser and in GenioWindow.
	// Refactor!
	ProjectItem* projectItem = GetSelectedProjectItem();
	ProjectFolder* project = projectItem->GetSourceItem()->GetProjectFolder();

	BPopUpMenu* projectMenu = new BPopUpMenu("ProjectMenu", false, false);

	TemplatesMenu* fileNewProjectMenuItem = new TemplatesMenu(this, B_TRANSLATE("New"),
		new BMessage(MSG_PROJECT_MENU_NEW_FILE), new BMessage(MSG_SHOW_TEMPLATE_USER_FOLDER),
		TemplateManager::GetDefaultTemplateDirectory(),
		TemplateManager::GetUserTemplateDirectory(),
		TemplatesMenu::SHOW_ALL_VIEW_MODE,	true);

	fileNewProjectMenuItem->SetEnabled(true);

	if (projectItem->GetSourceItem()->Type() == SourceItemType::ProjectFolderItem) {
		BMessage* closePrj = new BMessage(MSG_PROJECT_MENU_CLOSE);
		closePrj->AddPointer("project", project);
		BMenuItem* closeProjectMenuItem = new BMenuItem(B_TRANSLATE("Close project"), closePrj);

		BMessage* setActive = new BMessage(MSG_PROJECT_MENU_SET_ACTIVE);
		setActive->AddPointer("project", project);
		BMenuItem* setActiveProjectMenuItem = new BMenuItem(B_TRANSLATE("Set active"), setActive);

		BMessage* projSettings = new BMessage(MSG_PROJECT_SETTINGS);
		projSettings->AddPointer("project", project);
		BMenuItem* projectSettingsMenuItem = new BMenuItem(B_TRANSLATE("Project settings" B_UTF8_ELLIPSIS),
			projSettings);

		projectMenu->AddItem(closeProjectMenuItem);
		projectMenu->AddItem(setActiveProjectMenuItem);
		projectMenu->AddItem(projectSettingsMenuItem);
		closeProjectMenuItem->SetEnabled(true);
		projectMenu->AddSeparatorItem();
		BMenu* switchBranchMenu = new SwitchBranchMenu(Window(), B_TRANSLATE("Switch to branch"),
												new BMessage(MSG_GIT_SWITCH_BRANCH),
												project->Path());
		projectMenu->AddItem(switchBranchMenu);
		projectMenu->AddSeparatorItem();
		BMenuItem* buildMenuItem = new BMenuItem(B_TRANSLATE("Build project"),
			new BMessage(MSG_BUILD_PROJECT));
		BMenuItem* cleanMenuItem = new BMenuItem(B_TRANSLATE("Clean project"),
			new BMessage(MSG_CLEAN_PROJECT));
		projectMenu->AddItem(buildMenuItem);
		projectMenu->AddItem(cleanMenuItem);

		projectMenu->AddSeparatorItem();

		BMenu* buildModeItem = new BMenu(B_TRANSLATE("Build mode"));
		buildModeItem->SetRadioMode(true);
		BMenuItem* release = new BMenuItem(B_TRANSLATE("Release"), new BMessage(MSG_BUILD_MODE_RELEASE));
		BMenuItem* debug   = new BMenuItem(B_TRANSLATE("Debug"), new BMessage(MSG_BUILD_MODE_DEBUG));

		buildModeItem->AddItem(release);
		buildModeItem->AddItem(debug);

		buildModeItem->SetTargetForItems(Window());

		projectMenu->AddItem(buildModeItem);

		const bool releaseMode = project->GetBuildMode() == BuildMode::ReleaseMode;
		release->SetMarked(releaseMode);
		debug->SetMarked(!releaseMode);

		projectMenu->AddSeparatorItem();

		ProjectFolder* activeProject = dynamic_cast<GenioWindow*>(Window())->GetActiveProject();
		setActiveProjectMenuItem->SetEnabled(!project->Active() && !activeProject->IsBuilding());

		if (project->IsBuilding())
			switchBranchMenu->SetEnabled(false);

		if (project->IsBuilding() || !project->Active()) {
			buildMenuItem->SetEnabled(false);
			cleanMenuItem->SetEnabled(false);
			buildModeItem->SetEnabled(false);
		}
	}

	SelectionChanged();

	const entry_ref* itemRef = projectItem->GetSourceItem()->EntryRef();

	projectMenu->AddItem(fileNewProjectMenuItem);

	BEntry entry(itemRef);
	if (entry.IsFile()) {
		entry.GetParent(&entry);
		entry_ref ref;
		entry.GetRef(&ref);
		fileNewProjectMenuItem->SetSender(projectItem, &ref);
	} else
		fileNewProjectMenuItem->SetSender(projectItem, itemRef);

	fileNewProjectMenuItem->SetViewMode(TemplatesMenu::ViewMode::SHOW_ALL_VIEW_MODE);
	projectMenu->AddSeparatorItem();

	bool isFolder = projectItem->GetSourceItem()->Type() == SourceItemType::FolderItem;
	bool isFile = projectItem->GetSourceItem()->Type() == SourceItemType::FileItem;
	if (isFolder || isFile) {
		BMenuItem* deleteFileProjectMenuItem = new BMenuItem(
			isFile ? B_TRANSLATE("Delete file") : B_TRANSLATE("Delete folder"),
			new BMessage(MSG_PROJECT_MENU_DELETE_FILE));
		deleteFileProjectMenuItem->Message()->AddPointer("project", project);
		deleteFileProjectMenuItem->Message()->AddRef("ref", itemRef);

		BMenuItem* openFileProjectMenuItem = new BMenuItem(B_TRANSLATE("Open file"),
			new BMessage(MSG_PROJECT_MENU_OPEN_FILE));
		openFileProjectMenuItem->Message()->AddRef("refs", itemRef);
		openFileProjectMenuItem->Message()->AddBool("openWithPreferred", true);

		BMenuItem* renameFileProjectMenuItem = new BMenuItem(
			isFile ? B_TRANSLATE("Rename file") : B_TRANSLATE("Rename folder"),
			new BMessage(MSG_PROJECT_MENU_RENAME_FILE));
		renameFileProjectMenuItem->Message()->AddPointer("project", project);
		renameFileProjectMenuItem->Message()->AddRef("ref", itemRef);

		projectMenu->AddItem(openFileProjectMenuItem);
		projectMenu->AddItem(deleteFileProjectMenuItem);
		projectMenu->AddItem(renameFileProjectMenuItem);
		deleteFileProjectMenuItem->SetEnabled(true);
		renameFileProjectMenuItem->SetEnabled(true);
		openFileProjectMenuItem->SetEnabled(isFile);
	}

	BMessage* refMessage = new BMessage();
	refMessage->AddRef("ref", itemRef);
	ActionManager::AddItem(MSG_PROJECT_MENU_SHOW_IN_TRACKER, projectMenu, refMessage);

	BMessage* refMessage2 = new BMessage(*refMessage);
	ActionManager::AddItem(MSG_PROJECT_MENU_OPEN_TERMINAL, projectMenu, refMessage2);

	projectMenu->SetTargetForItems(Window());

	projectMenu->SetAsyncAutoDestruct(true);

	// Open menu slightly off wrt the click, so it doesn't open right under the mouse
	BPoint menuPoint = ConvertToScreen(where);
	menuPoint.x += 1;
	menuPoint.y += 1;
	projectMenu->Go(menuPoint, true, false, true);
}
