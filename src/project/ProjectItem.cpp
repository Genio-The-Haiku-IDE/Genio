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
				Parent()->MakeFocus();
				fProjectItem->CommitRename();
			}
		}
	}
};


BTextControl* ProjectItem::sTextControl = nullptr;

ProjectItem::ProjectItem(SourceItem *sourceItem)
	:
	StyledItem(sourceItem->Name()),
	fSourceItem(sourceItem),
	fNeedsSave(false),
	fOpenedInEditor(false),
	fRenaming(false)
{
}


/* virtual */
ProjectItem::~ProjectItem()
{
	delete fSourceItem;
}


/* virtual */
void
ProjectItem::DrawItem(BView* owner, BRect bounds, bool complete)
{
	DrawItemPrepare(owner, bounds, complete);

	const float iconSize = be_control_look->ComposeIconSize(B_MINI_ICON).Height();
	BRect iconRect = DrawIcon(owner, bounds, iconSize);

	if (fOpenedInEditor)
		SetTextFontFace(B_ITALIC_FACE);
	else
		SetTextFontFace(B_REGULAR_FACE);

	// There's a TextControl for renaming
	if (fRenaming && sTextControl != nullptr) {
		// TODO: We do this hide/show to workaround a weird bug where if we rename the last
		// visible list item, DrawItem() isn't called (probably because the item is hidden by the
		// textbox, but weird it only happens when the item is the last visible one.
		if (sTextControl->IsHidden())
			sTextControl->Show();
		BRect textRect;
		textRect.top = bounds.top - 0.5f;
		textRect.left = iconRect.right + be_control_look->DefaultLabelSpacing();
		textRect.bottom = bounds.bottom - 1;
		textRect.right = bounds.right;
		// TODO: We sould position the textbox on InitRename, but unfortunately
		// BOutlineListView::ItemRect() doesn't give us the item indentation.
		// It's only available here in DrawItem()
		sTextControl->MoveTo(textRect.LeftTop());
	} else {
		BPoint textPoint(iconRect.right + be_control_look->DefaultLabelSpacing(),
						bounds.top + BaselineOffset());

		// TODO: Apply any style change here (i.e. bold, italic)
		if (fNeedsSave)
			SetExtraText("*");
		else
			SetExtraText("");

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
	StyledItem::Update(owner, font);
}


void
ProjectItem::InitRename(BView* owner, BMessage* message)
{
	if (sTextControl == nullptr) {
		fRenaming = true;
		BOutlineListView* listView = static_cast<BOutlineListView*>(owner);
		const int32 index = listView->IndexOf(this);
		BRect itemRect = listView->ItemFrame(index);

		sTextControl = new TemporaryTextControl(itemRect, "RenameTextWidget",
											"", Text(), message, this,
											B_FOLLOW_NONE);
		if (owner->LockLooper()) {
			owner->AddChild(sTextControl);
			// TODO: See the note in DrawItem()
			sTextControl->Hide();
			owner->UnlockLooper();
		}
		sTextControl->TextView()->SetAlignment(B_ALIGN_LEFT);
		sTextControl->SetDivider(0);
		sTextControl->TextView()->SelectAll();
		sTextControl->TextView()->ResizeBy(0, -3);
	}
	sTextControl->MakeFocus();
}


void
ProjectItem::AbortRename()
{
	_DestroyTextWidget();
}


void
ProjectItem::CommitRename()
{
	_DestroyTextWidget();
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
	if (sTextControl != nullptr) {
		sTextControl->RemoveSelf();
		delete sTextControl;
		sTextControl = nullptr;
		fRenaming = false;
	}
}


// ProjectTitleItem

int32 ProjectTitleItem::sBuildAnimationIndex = 0;
std::vector<BBitmap*> ProjectTitleItem::sBuildAnimationFrames;


ProjectTitleItem::ProjectTitleItem(SourceItem *sourceFile)
	:
	ProjectItem(sourceFile)
{
}


/* virtual */
ProjectTitleItem::~ProjectTitleItem()
{
}


/* virtual */
void
ProjectTitleItem::DrawItem(BView* owner, BRect bounds, bool complete)
{
	DrawItemPrepare(owner, bounds, complete);

	float iconSize = be_control_look->ComposeIconSize(B_MINI_ICON).Height();
	BRect iconRect = DrawIcon(owner, bounds, iconSize);

	ProjectFolder *projectFolder = static_cast<ProjectFolder*>(GetSourceItem());
	if (projectFolder->Active())
		SetTextFontFace(B_BOLD_FACE);
	else
		SetTextFontFace(B_REGULAR_FACE);

	BPoint textPoint(iconRect.right + be_control_look->DefaultLabelSpacing(),
					bounds.top + BaselineOffset());

	// Set the font face here, otherwise StringWidth() won't return
	// the correct width
	BFont font;
	font.SetFace(TextFontFace());
	owner->SetFont(&font, B_FONT_FACE);

	const rgb_color oldColor = owner->HighColor();

	// "highlight" the project title item with the project color
	owner->SetHighColor(projectFolder->Color());
	BRect circleRect;
	circleRect.top = textPoint.y - BaselineOffset() + 2.5f;
	circleRect.left = textPoint.x - 3;
	circleRect.bottom = textPoint.y + 6;
	circleRect.right = circleRect.left + owner->StringWidth(Text()) + 5;
	owner->FillRoundRect(circleRect, 9, 10);

	owner->SetHighColor(oldColor);

	// TODO: this part is quite computationally intensive
	// and shoud be moved away from the DrawItem.
	const BString projectName = Text();
	const BString projectPath = projectFolder->Path();
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

	DrawText(owner, Text(), ExtraText(), textPoint);

	if (projectFolder->IsBuilding()) {
		_DrawBuildIndicator(owner, bounds);
	}
	owner->Sync();

	BString toolTipText;
	toolTipText.SetToFormat("%s: %s\n%s: %s\n%s: %s",
								B_TRANSLATE("Project"), projectName.String(),
								B_TRANSLATE("Path"), projectPath.String(),
								B_TRANSLATE("Current branch"), branchName.String());
	SetToolTipText(toolTipText);
}


/* static */
status_t
ProjectTitleItem::InitAnimationIcons()
{
	// TODO: icon names are "waiting-N" where N is the index
	// 1 to 6
	BResources* resources = BApplication::AppResources();
	for (int32 i = 1; i < 7; i++) {
		BString name("waiting-");
		const int32 kBrightnessBreakValue = 126;
		const rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
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
ProjectTitleItem::DisposeAnimationIcons()
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
ProjectTitleItem::TickAnimation()
{
	if (++ProjectTitleItem::sBuildAnimationIndex >= (int32)sBuildAnimationFrames.size())
		ProjectTitleItem::sBuildAnimationIndex = 0;
}


/* virtual */
BRect
ProjectTitleItem::DrawIcon(BView* owner, const BRect& itemBounds,
							const float& iconSize)
{
	// TODO: We could draw a bigger icon here,
	// but IconCache needs to be reworked
	return ProjectItem::DrawIcon(owner, itemBounds, iconSize);
}


void
ProjectTitleItem::_DrawBuildIndicator(BView* owner, BRect bounds)
{
	try {
		const BBitmap* frame = sBuildAnimationFrames.at(sBuildAnimationIndex);
		if (frame != nullptr) {
			owner->SetDrawingMode(B_OP_ALPHA);
			bounds.left = owner->PenLocation().x + 5;
			bounds.right = bounds.left + bounds.Height();
			bounds.OffsetBy(-1, 1);
			owner->DrawBitmap(frame, frame->Bounds(), bounds);
		}
	} catch (...) {
		// nothing to do
	}
}
