/*
 * Copyright 2023, Nexus6 <nexus6.haiku@icloud.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "RepositoryView.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SourceControlPanel"

RepositoryView::RepositoryView()
	:
	BOutlineListView("RepositoryView", B_SINGLE_SELECTION_LIST),
	fRepositoryPath(nullptr),
	fSelectedProject(nullptr),
	fCurrentBranch(nullptr)
{
	SetInvocationMessage(new BMessage('strt'));
}

RepositoryView::RepositoryView(BString &repository_path)
	:
	BOutlineListView("RepositoryView", B_SINGLE_SELECTION_LIST),
	fRepositoryPath(repository_path),
	fSelectedProject(nullptr),
	fCurrentBranch(nullptr)
{
	SetInvocationMessage(new BMessage('strt'));
}


RepositoryView::RepositoryView(const char *repository_path)
	:
	BOutlineListView("RepositoryView", B_SINGLE_SELECTION_LIST),
	fRepositoryPath(repository_path),
	fSelectedProject(nullptr),
	fCurrentBranch(nullptr)
{
	// SetInvocationMessage(new BMessage('strt'));
	// SetSelectionMessage(new BMessage('strt'));
}


RepositoryView::~RepositoryView()
{
}


void
RepositoryView::MouseDown(BPoint where)
{
	BOutlineListView::MouseDown(where);
	if (IndexOf(where) >= 0) {
		int32 buttons = -1;

		BMessage* message = Looper()->CurrentMessage();
		 if (message != NULL)
			 message->FindInt32("buttons", &buttons);

		if (buttons == B_MOUSE_BUTTON(2)) {
			_ShowPopupMenu(where);
		}
	}
}


void
RepositoryView::MouseMoved(BPoint point, uint32 transit, const BMessage* message)
{
	if ((transit == B_ENTERED_VIEW) || (transit == B_INSIDE_VIEW)) {
		auto index = IndexOf(point);
		if (index >= 0) {
			StyledItem *item = reinterpret_cast<StyledItem*>(ItemAt(index));
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
RepositoryView::AttachedToWindow()
{
	BOutlineListView::AttachedToWindow();

	if (Parent()->LockLooper()) {
		Parent()->StartWatching(this, MsgChangeProject);
		Parent()->StartWatching(this, MsgSwitchBranch);
		Parent()->UnlockLooper();
	}
}

void
RepositoryView::DetachedFromWindow()
{
	BOutlineListView::DetachedFromWindow();

	if (Parent()->LockLooper()) {
		Parent()->StopWatching(this, MsgChangeProject);
		Parent()->StopWatching(this, MsgSwitchBranch);
		Parent()->UnlockLooper();
	}
}


void
RepositoryView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_OBSERVER_NOTICE_CHANGE: {
			int32 code;
			message->FindInt32(B_OBSERVE_WHAT_CHANGE, &code);
			switch (code) {
				case MsgChangeProject:
				{
					LogInfo("MsgChangeProject");
					fSelectedProject = (ProjectFolder *)message->GetPointer("value");
					if (fSelectedProject!=nullptr)
						fRepositoryPath = fSelectedProject->Path().String();
					_UpdateRepository();
					break;
				}
				case MsgSwitchBranch:
				{
					LogInfo("MsgSwitchBranch");
					fCurrentBranch = message->GetString("value");
					if (!fCurrentBranch.IsEmpty())
						_UpdateRepository();
					OKAlert("", "MsgSwitchBranch", B_INFO_ALERT);
					break;
				}
				default:
					break;
			}
			break;
		}
		default:
			break;
	}
	BOutlineListView::MessageReceived(message);
}


void
RepositoryView::SelectionChanged()
{
}


void
RepositoryView::_ShowPopupMenu(BPoint where)
{
	auto optionsMenu = new BPopUpMenu("Options", false, false);

	auto index = IndexOf(where);
	if (index >= 0) {
		StyledItem *item = reinterpret_cast<StyledItem*>(ItemAt(index));
		auto item_type = item->GetType();

		auto selected_branch = item->Text();

		StringFormatter fmt;
		fmt.Substitutions["%selected_branch%"] = selected_branch;

		switch (item_type) {
			case kLocalBranch: {

				optionsMenu->AddItem(new BMenuItem(fmt << B_TRANSLATE("Switch to \"%branch%\""),
					new BMessage(MsgSwitchBranch)));

				optionsMenu->AddItem(new BMenuItem(fmt << B_TRANSLATE("Pull origin/\"%branch%\""),
					new BMessage(MsgPull)));

				optionsMenu->AddItem(new BMenuItem(fmt << B_TRANSLATE("Push \"%branch%\" to \"origin\""),
					new BMessage(2)));

				optionsMenu->AddSeparatorItem();

				optionsMenu->AddItem(new BMenuItem(fmt << B_TRANSLATE("Create new branch from \"%branch%\""),
					new BMessage(2)));

				optionsMenu->AddItem(new BMenuItem(fmt << B_TRANSLATE("Create new tag from \"%branch%\""),
					new BMessage(2)));

				optionsMenu->AddSeparatorItem();

				optionsMenu->AddItem(new BMenuItem(fmt << B_TRANSLATE("Rename \"%branch%\""),
					new BMessage(2)));

				optionsMenu->AddItem(new BMenuItem(fmt << B_TRANSLATE("Delete \"%branch%\""),
					new BMessage(2)));

				break;
			}
			case kRemoteBranch: {

				fmt.Substitutions["%current_branch%"] = fCurrentBranch;

				optionsMenu->AddItem(
					new BMenuItem(
						fmt << B_TRANSLATE("Switch to \"%branch%\""),
						new GMessage{
							{"what", MsgSwitchBranch},
							{"selected_branch", selected_branch}
						}
					)
				);

				optionsMenu->AddItem(
					new BMenuItem(
						fmt << B_TRANSLATE("Merge \"%branch%\" into \"%current_branch%\""),
						new GMessage{
							{"what", MsgPull},
							{"selected_branch", selected_branch},
							{"current_branch", fCurrentBranch}
						}
					)
				);

				optionsMenu->AddItem(
					new BMenuItem(
						fmt << B_TRANSLATE("Push \"%branch%\" to \"origin\%branch%\""),
						new GMessage{
							{"what", MsgPush},
							{"selected_branch", selected_branch}
						}
					)
				);

				optionsMenu->AddSeparatorItem();

				optionsMenu->AddItem(new BMenuItem(fmt << B_TRANSLATE("Create new branch from \"%branch%\""),
					new BMessage(2)));

				optionsMenu->AddItem(new BMenuItem(fmt << B_TRANSLATE("Create new tag from \"%branch%\""),
					new BMessage(2)));

				optionsMenu->AddSeparatorItem();

				optionsMenu->AddItem(new BMenuItem(fmt << B_TRANSLATE("Rename \"%branch%\""),
					new BMessage(2)));

				optionsMenu->AddItem(new BMenuItem(fmt << B_TRANSLATE("Delete \"%branch%\""),
					new BMessage(2)));

				break;
			}
			case kTag: {
				break;
			}
			default: {
				break;
			}
		}
	}

	// optionsMenu->AddItem(new BMenuItem(B_TRANSLATE("Fetch"), new BMessage(MsgFetch)));
	// optionsMenu->AddItem(new BMenuItem(B_TRANSLATE("Fetch prune"),	new BMessage(MsgFetchPrune)));
	// optionsMenu->AddItem(new BMenuItem(B_TRANSLATE("Pull"), new BMessage(MsgPull)));
	// optionsMenu->AddItem(new BMenuItem(B_TRANSLATE("Pull rebase"),	new BMessage(MsgPullRebase)));
	// optionsMenu->AddItem(new BMenuItem(B_TRANSLATE("Merge"), new BMessage(MsgMerge)));
	// optionsMenu->AddItem(new BMenuItem(B_TRANSLATE("Push"), new BMessage(MsgPush)));
	// optionsMenu->AddSeparatorItem();
	// optionsMenu->AddItem(new BMenuItem(B_TRANSLATE("Stash changes"), new BMessage(MsgStashSave)));
	// optionsMenu->AddItem(new BMenuItem(B_TRANSLATE("Stash pop changes"), new BMessage(MsgStashPop)));
	// optionsMenu->AddItem(new BMenuItem(B_TRANSLATE("Stash apply changes"), new BMessage(MsgStashApply)));


	optionsMenu->SetTargetForItems(this);
	optionsMenu->Go(ConvertToScreen(where), true);
	delete optionsMenu;
}


void
RepositoryView::_UpdateRepository()
{
}