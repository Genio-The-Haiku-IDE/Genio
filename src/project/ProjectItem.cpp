/*
 * Copyright 2023 Nexus6
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "ProjectItem.h"

#include <Bitmap.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <Font.h>
#include <MenuItem.h>
#include <NodeInfo.h>
#include <OutlineListView.h>
#include <PopUpMenu.h>
#include <StringItem.h>
#include <TextControl.h>
#include <Window.h>

#include "IconCache.h"
#include "GenioWindowMessages.h"
#include "GitRepository.h"
#include "ProjectBrowser.h"
#include "ProjectFolder.h"
#include "TemplateManager.h"
#include "TemplatesMenu.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ProjectsFolderBrowser"

class ProjectItem;

class TemporaryTextControl: public BTextControl {
	typedef	BTextControl _inherited;


public:
	ProjectItem *fProjectItem;

	TemporaryTextControl(BRect frame, const char* name, const char* label, const char* text,
							BMessage* message, ProjectItem *item, uint32 resizingMode = B_FOLLOW_LEFT|B_FOLLOW_TOP,
							uint32 flags = B_WILL_DRAW|B_NAVIGABLE)
		:
		BTextControl(frame, name, label, text, message, resizingMode, flags),
		fProjectItem(item)
	{
		SetEventMask(B_POINTER_EVENTS|B_KEYBOARD_EVENTS);
	}

	virtual void AllAttached()
	{
		TextView()->SelectAll();
	}

	virtual void MouseDown(BPoint point)
	{
		if (Bounds().Contains(point))
			_inherited::MouseDown(point);
		else {
			fProjectItem->AbortRename();
		}
		Invoke();
	}

	virtual void KeyDown(const char* bytes, int32 numBytes)
	{
		if (numBytes == 1 && *bytes == B_ESCAPE) {
			fProjectItem->AbortRename();
		}
		if (numBytes == 1 && *bytes == B_RETURN) {
			BMessage message(*Message());
			message.AddString("_value", Text());
			BMessenger(Parent()).SendMessage(&message);
			fProjectItem->CommitRename();
		}
	}
};


ProjectItem::ProjectItem(SourceItem *sourceItem)
	:
	StyledItem(sourceItem->Name()),
	fSourceItem(sourceItem),
	fNeedsSave(false),
	fOpenedInEditor(false),
	fTextControl(nullptr)
{
}


/* virtual */
ProjectItem::~ProjectItem()
{
	delete fSourceItem;
	delete fTextControl;
}


/* virtual */
void
ProjectItem::DrawItem(BView* owner, BRect bounds, bool complete)
{
	// TODO: this part is duplicated between StyledItem and here:
	// Move to a common method so inherited classes don't have
	// to duplicate
	if (Text() == NULL)
		return;

	if (IsSelected() || complete) {
		rgb_color oldLowColor = owner->LowColor();
		rgb_color color;
		if (IsSelected())
			color = ui_color(B_LIST_SELECTED_BACKGROUND_COLOR);
		else
			color = owner->ViewColor();
		owner->SetLowColor(color);
		owner->FillRect(bounds, B_SOLID_LOW);
		owner->SetLowColor(oldLowColor);
	}

	owner->SetFont(be_plain_font);

	if (IsSelected())
		owner->SetHighColor(ui_color(B_LIST_SELECTED_ITEM_TEXT_COLOR));
	else
		owner->SetHighColor(ui_color(B_LIST_ITEM_TEXT_COLOR));

	// TODO: until here... (see comment above)

	bool isProject = GetSourceItem()->Type() == SourceItemType::ProjectFolderItem;
	if (isProject) {
		ProjectFolder *projectFolder = static_cast<ProjectFolder*>(GetSourceItem());
		if (projectFolder->Active())
			SetTextFontFace(B_BOLD_FACE);
		else
			SetTextFontFace(B_REGULAR_FACE);

		// TODO: this part is quite computationally intensive
		// and shoud be moved away from the DrawItem.

		BString projectName = Text();
		BString projectPath = projectFolder->Path();
		BString branchName;
		try {
			if (projectFolder->GetRepository()) {
				branchName = projectFolder->GetRepository()->GetCurrentBranch();
				BString extraText;
				extraText << "  [" << branchName << "]";
				SetExtraText(extraText);
			}
		} catch (const Genio::Git::GitException &ex) {
		}

		BString toolTipText;
		toolTipText.SetToFormat("%s: %s\n%s: %s\n%s: %s",
									B_TRANSLATE("Project"), projectName.String(),
									B_TRANSLATE("Path"), projectPath.String(),
									B_TRANSLATE("Current branch"), branchName.String());
		SetToolTipText(toolTipText);
	} else if (fOpenedInEditor)
		SetTextFontFace(B_ITALIC_FACE);
	else
		SetTextFontFace(B_REGULAR_FACE);

	float iconSize = be_control_look->ComposeIconSize(B_MINI_ICON).Height();
	BRect iconRect = DrawIcon(owner, bounds, iconSize);

	// There's a TextControl for renaming
	if (fTextControl != nullptr) {
		BRect textRect;
		textRect.top = bounds.top - 0.5f;
		textRect.left = iconRect.right + be_control_look->DefaultLabelSpacing();
		textRect.bottom = bounds.bottom - 1;
		textRect.right = bounds.right;
		// TODO: Don't move it every time
		fTextControl->MoveTo(textRect.LeftTop());
	} else {
		BPoint textPoint(iconRect.right + be_control_look->DefaultLabelSpacing(),
						bounds.top + BaselineOffset());

		// TODO: Apply any style change here (i.e. bold, italic)
		if (!isProject) {
			if (fNeedsSave)
				SetExtraText("*");
			else
				SetExtraText("");
		}

		if (isProject) {
			// Fill background to the project color
			ProjectFolder *projectFolder = static_cast<ProjectFolder*>(GetSourceItem());
			const rgb_color oldColor = owner->HighColor();
			owner->SetHighColor(projectFolder->Color());

			// Set the font face here, otherwise StringWidth() won't return
			// the correct width
			BFont font;
			owner->GetFont(&font);
			font.SetFace(TextFontFace());
			owner->SetFont(&font);

			BRect circleRect;
			circleRect.top = textPoint.y - BaselineOffset() + 2.5f;
			circleRect.left = textPoint.x - 3;
			circleRect.bottom = textPoint.y + 6;
			circleRect.right = circleRect.left + owner->StringWidth(Text()) + 5;
			owner->FillRoundRect(circleRect, 9, 10);
			owner->SetHighColor(oldColor);
		}
		DrawText(owner, Text(), ExtraText(), textPoint);

		owner->Sync();
	}
}


void
ProjectItem::SetNeedsSave(bool needs)
{
	fNeedsSave = needs;
}


void
ProjectItem::SetOpenedInEditor(bool open)
{
	fOpenedInEditor = open;
}


/* virtual */
void
ProjectItem::Update(BView* owner, const BFont* font)
{
	StyledItem::Update(owner, be_bold_font);
}


void
ProjectItem::InitRename(BView* owner, BMessage* message)
{
	if (fTextControl == nullptr) {
		BOutlineListView* listView = static_cast<BOutlineListView*>(owner);
		const int32 index = listView->IndexOf(this);
		BRect itemRect = listView->ItemFrame(index);
		fTextControl = new TemporaryTextControl(itemRect, "RenameTextWidget",
											"", Text(), message, this,
											B_FOLLOW_NONE);
		if (owner->LockLooper()) {
			owner->AddChild(fTextControl);
			owner->UnlockLooper();
		}
		fTextControl->TextView()->SetAlignment(B_ALIGN_LEFT);
		fTextControl->SetDivider(0);
		fTextControl->TextView()->SelectAll();
		fTextControl->TextView()->ResizeBy(0, -3);
	}
	fTextControl->MakeFocus();
}


void
ProjectItem::AbortRename()
{
	if (fTextControl != nullptr)
		_DestroyTextWidget();
}


void
ProjectItem::CommitRename()
{
	if (fTextControl != nullptr) {
		_DestroyTextWidget();
	}
}


BPopUpMenu*
ProjectItem::BuildPopupMenu()
{
	BPopUpMenu* projectMenu = new BPopUpMenu("ProjectMenu", false, false);

	TemplatesMenu *fileNewProjectMenuItem = new TemplatesMenu(nullptr, B_TRANSLATE("New"),
		new BMessage(MSG_PROJECT_MENU_NEW_FILE), new BMessage(MSG_SHOW_TEMPLATE_USER_FOLDER),
		TemplateManager::GetDefaultTemplateDirectory(),
		TemplateManager::GetUserTemplateDirectory(),
		TemplatesMenu::SHOW_ALL_VIEW_MODE,	true);

	fileNewProjectMenuItem->SetEnabled(true);

	if (GetSourceItem()->Type() == SourceItemType::ProjectFolderItem) {
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
//		projectMenu->AddItem(new SwitchBranchMenu(Window(), B_TRANSLATE("Switch to branch"),
	//												new BMessage(MSG_GIT_SWITCH_BRANCH), project->Path()));
		projectMenu->AddSeparatorItem();
		BMenuItem* buildMenuItem = new BMenuItem(B_TRANSLATE("Build project"),
			new BMessage(MSG_BUILD_PROJECT));
		BMenuItem* cleanMenuItem = new BMenuItem(B_TRANSLATE("Clean project"),
			new BMessage(MSG_CLEAN_PROJECT));
		projectMenu->AddItem(buildMenuItem);
		projectMenu->AddItem(cleanMenuItem);
//		setActiveProjectMenuItem->SetEnabled(!project->Active());
		/*if (!project->Active())
			setActiveProjectMenuItem->SetEnabled(true);
		if (fIsBuilding || !project->Active()) {
			projectSettingsMenuItem->SetEnabled(false);
			buildMenuItem->SetEnabled(false);
			cleanMenuItem->SetEnabled(false);
		}*/
	}

	projectMenu->AddItem(fileNewProjectMenuItem);
	fileNewProjectMenuItem->SetViewMode(TemplatesMenu::ViewMode::SHOW_ALL_VIEW_MODE);
	projectMenu->AddSeparatorItem();

	//SelectionChanged();
	//GenioWindow *window = (GenioWindow*)Window();
	BEntry entry(GetSourceItem()->EntryRef());
	if (entry.IsFile()) {
		// If this is a file, get the parent directory
		entry.GetParent(&entry);
	}
	entry_ref newRef;
	entry.GetRef(&newRef);
	//window->UpdateMenu(selected, &newRef);

	if (fileNewProjectMenuItem != nullptr)
		fileNewProjectMenuItem->SetSender(this, &newRef);

	bool isFolder = GetSourceItem()->Type() == SourceItemType::FolderItem;
	bool isFile = GetSourceItem()->Type() == SourceItemType::FileItem;
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

	BMenuItem* showInTrackerProjectMenuItem = new BMenuItem(B_TRANSLATE("Show in Tracker"),
		new BMessage(MSG_PROJECT_MENU_SHOW_IN_TRACKER));
	BMenuItem* openTerminalProjectMenuItem = new BMenuItem(B_TRANSLATE("Open in Terminal"),
		new BMessage(MSG_PROJECT_MENU_OPEN_TERMINAL));
	showInTrackerProjectMenuItem->SetEnabled(true);
	openTerminalProjectMenuItem->SetEnabled(true);
	projectMenu->AddItem(showInTrackerProjectMenuItem);
	projectMenu->AddItem(openTerminalProjectMenuItem);

//	projectMenu->SetTargetForItems(Window());
//	projectMenu->Go(ConvertToScreen(where), true);

	return projectMenu;
}


/* virtual */
BRect
ProjectItem::DrawIcon(BView* owner, const BRect& itemBounds,
							const float& iconSize)
{
	BPoint iconStartingPoint(itemBounds.left + 4.0f,
		itemBounds.top + (itemBounds.Height() - iconSize) / 2.0f);
	const BBitmap* icon = IconCache::GetIcon(GetSourceItem()->EntryRef());
	if (icon != nullptr) {
		owner->SetDrawingMode(B_OP_ALPHA);
		owner->DrawBitmapAsync(icon, iconStartingPoint);
	}

	return BRect(iconStartingPoint, BSize(iconSize, iconSize));
}


void
ProjectItem::_DestroyTextWidget()
{
	if (fTextControl != nullptr) {
		fTextControl->RemoveSelf();
		delete fTextControl;
		fTextControl = nullptr;
	}
}
