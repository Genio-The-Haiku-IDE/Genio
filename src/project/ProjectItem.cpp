/*
 * Copyright 2023 Nexus6
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "ProjectItem.h"

#include <Bitmap.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <Font.h>
#include <NodeInfo.h>
#include <OutlineListView.h>
#include <StringItem.h>
#include <TextControl.h>
#include <Window.h>

#include "IconCache.h"
#include "GitRepository.h"
#include "ProjectFolder.h"

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

	if (GetSourceItem()->Type() == SourceItemType::ProjectFolderItem) {
		ProjectFolder *projectFolder = static_cast<ProjectFolder*>(GetSourceItem());
		if (projectFolder->Active())
			SetTextFontFace(B_BOLD_FACE);
		else
			SetTextFontFace(B_REGULAR_FACE);
		BString projectName = Text();
		BString projectPath = projectFolder->Path();
		BString branchName;
		try {
			Genio::Git::GitRepository repo(projectPath.String());
			branchName = repo.GetCurrentBranch();
			BString extraText;
			extraText << "  [" << branchName << "]";
			SetExtraText(extraText);
		} catch (const Genio::Git::GitException &ex) {
		}

		BString toolTipText;
		toolTipText.SetToFormat("%s: %s\n%s: %s\n%s: %s",
									B_TRANSLATE("Project"), Text(),
									B_TRANSLATE("Path"), projectPath.String(),
									B_TRANSLATE("Current branch"), branchName.String());
		SetToolTipText(toolTipText);
	} else if (fOpenedInEditor)
		SetTextFontFace(B_ITALIC_FACE);
	else
		SetTextFontFace(B_REGULAR_FACE);

	const BBitmap* icon = IconCache::GetIcon(GetSourceItem()->Path());
	float iconSize = be_control_look->ComposeIconSize(B_MINI_ICON).Height();
	BRect iconRect = DrawIcon(owner, bounds, icon, iconSize);

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
		BString text = Text();
		if (fNeedsSave)
			text.Append("*");
		text.Append(ExtraText());

		DrawText(owner, text, textPoint);

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


void
ProjectItem::_DestroyTextWidget()
{
	if (fTextControl != nullptr) {
		fTextControl->RemoveSelf();
		delete fTextControl;
		fTextControl = nullptr;
	}
}
