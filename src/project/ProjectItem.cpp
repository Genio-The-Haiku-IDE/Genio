/*
 * Copyright 2023 Nexus6
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "ProjectItem.h"

#include <Application.h>
#include <Bitmap.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <Font.h>
#include <NodeInfo.h>
#include <OutlineListView.h>
#include <Resources.h>
#include <StringItem.h>
#include <TextControl.h>
#include <TranslationUtils.h>
#include <Window.h>

#include "GitRepository.h"
#include "IconCache.h"
#include "ProjectFolder.h"
#include "Utils.h"


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
		if (numBytes == 1) {
			if (*bytes == B_ESCAPE) {
				fProjectItem->AbortRename();
			} else if (*bytes == B_RETURN) {
				BMessage message(*Message());
				message.AddString("_value", Text());
				BMessenger(Parent()).SendMessage(&message);
				fProjectItem->CommitRename();
			}
		}
	}
};


int32 ProjectItem::sBuildAnimationIndex = 0;
std::vector<BBitmap*> ProjectItem::sBuildAnimationFrames;


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

		if (isProject && static_cast<ProjectFolder*>(GetSourceItem())->IsBuilding()) {
			_DrawBuildIndicator(owner, bounds);
		}
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


/* static */
status_t
ProjectItem::InitAnimationIcons()
{
	// TODO: icon names are "waiting-N" where N is the index
	// 1 to 6
	BResources* resources = BApplication::AppResources();
	for (int32 i = 1; i < 7; i++) {
		BString name("waiting-");
		const int32 kBrightnessBreakValue = 126;
		rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
		if (base.Brightness() >= kBrightnessBreakValue)
			name.Append("light-");
		else
			name.Append("dark-");
		name << i;
		size_t size;
		const uint8* rawData = (const uint8*)resources->LoadResource(
			B_RAW_TYPE, name.String(), &size);
		if (rawData == nullptr) {
			LogError("InitAnimationIcons: Cannot load resource");
			break;
		}
		BMemoryIO mem(rawData, size);
		BBitmap* frame = BTranslationUtils::GetBitmap(&mem);

		sBuildAnimationFrames.push_back(frame);
	}
	return B_OK;
}


/* static */
status_t
ProjectItem::DisposeAnimationIcons()
{
	for (std::vector<BBitmap*>::iterator i = sBuildAnimationFrames.begin();
		i != sBuildAnimationFrames.end(); i++) {
		delete *i;
	}
	sBuildAnimationFrames.clear();

	return B_OK;
}


/* static */
void
ProjectItem::TickAnimation()
{
	if (++ProjectItem::sBuildAnimationIndex >= (int32)sBuildAnimationFrames.size())
		ProjectItem::sBuildAnimationIndex = 0;
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
ProjectItem::_DrawBuildIndicator(BView* owner, BRect bounds)
{
	try {
		const BBitmap* frame = sBuildAnimationFrames.at(sBuildAnimationIndex);
		if (frame != nullptr) {
			owner->SetDrawingMode(B_OP_ALPHA);
			bounds.left = bounds.right - bounds.Height();
			bounds.OffsetBy(-1, 1);
			owner->DrawBitmap(frame, frame->Bounds(), bounds);
		}
	} catch (...) {
		// nothing to do
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

