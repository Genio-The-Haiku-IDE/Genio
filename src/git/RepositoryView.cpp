/*
 * Copyright 2023, Nexus6 <nexus6.haiku@icloud.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "RepositoryView.h"

#include "BranchItem.h"
#include "StringFormatter.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SourceControlPanel"



RepositoryView::RepositoryView()
	:
	BOutlineListView("RepositoryView", B_SINGLE_SELECTION_LIST),
	fRepositoryPath(nullptr),
	fSelectedProject(nullptr),
	fCurrentBranch(nullptr)
{
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
			StyledItem *item = dynamic_cast<StyledItem*>(ItemAt(index));
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
	SetTarget(this);
	if (Target()->LockLooper()) {
		Target()->StartWatching(this, MsgChangeProject);
		Target()->StartWatching(this, MsgSwitchBranch);
		Target()->UnlockLooper();
	}

	SetInvocationMessage(new BMessage(kInvocationMessage));
}

void
RepositoryView::DetachedFromWindow()
{
	BOutlineListView::DetachedFromWindow();

	if (Target()->LockLooper()) {
		Target()->StopWatching(this, MsgChangeProject);
		Target()->StopWatching(this, MsgSwitchBranch);
		Target()->UnlockLooper();
	}
}


void
RepositoryView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kInvocationMessage: {
			auto item = dynamic_cast<BranchItem*>(ItemAt(CurrentSelection()));
			if (item == nullptr)
				break;
			switch (item->BranchType()) {
				case kLocalBranch:
				case kRemoteBranch: {
					auto switch_message = new GMessage{
						{"what", MsgSwitchBranch},
						{"value", item->Text()},
						{"type", GIT_BRANCH_LOCAL},
						{"sender", kSenderRepositoryPopupMenu}};
						BMessenger messenger(Target());
						messenger.SendMessage(switch_message);
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
		auto item = dynamic_cast<BranchItem*>(ItemAt(index));
		auto item_type = item->BranchType();

		BString selected_branch(item->Text());
		selected_branch.RemoveLast("*");

		StringFormatter fmt;
		fmt.Substitutions["%selected_branch%"] = selected_branch;

		switch (item_type) {
			case kLocalBranch: {

				if (selected_branch != fCurrentBranch) {
					optionsMenu->AddItem(
						new BMenuItem(
							fmt << B_TRANSLATE("Switch to \"%selected_branch%\""),
							new GMessage{
								{"what", MsgSwitchBranch},
								{"value", selected_branch},
								{"type", GIT_BRANCH_LOCAL},
								{"sender", kSenderRepositoryPopupMenu}}));
				}

				optionsMenu->AddItem(
					new BMenuItem(
						fmt << B_TRANSLATE("Rename \"%selected_branch%\""),
						new GMessage{
							{"what", MsgRenameBranch},
							{"value", selected_branch},
							{"type", GIT_BRANCH_LOCAL}}));

				optionsMenu->AddItem(
					new BMenuItem(
						fmt << B_TRANSLATE("Delete \"%selected_branch%\""),
						new GMessage{
							{"what", MsgDeleteBranch},
							{"value", selected_branch},
							{"type", GIT_BRANCH_LOCAL}}));

				optionsMenu->AddSeparatorItem();

				optionsMenu->AddItem(
					new BMenuItem(
						fmt << B_TRANSLATE("Create new branch from \"%selected_branch%\""),
						new GMessage{
							{"what", MsgNewBranch},
							{"value", selected_branch},
							{"type", GIT_BRANCH_LOCAL}}));

				// optionsMenu->AddItem(
					// new BMenuItem(
						// fmt << B_TRANSLATE("Create new tag from \"%selected_branch%\""),
						// new GMessage{
							// {"what", MsgNewTag},
							// {"value", selected_branch}}));

				break;
			}
			case kRemoteBranch: {

				fmt.Substitutions["%current_branch%"] = fCurrentBranch;
				LogInfo("fmt.Substitutions[%current_branch%] = %s", fCurrentBranch.String());

				optionsMenu->AddItem(
					new BMenuItem(
						fmt << B_TRANSLATE("Switch to \"%selected_branch%\""),
						new GMessage{
							{"what", MsgSwitchBranch},
							{"value", selected_branch},
							{"type", GIT_BRANCH_REMOTE},
							{"sender", kSenderRepositoryPopupMenu}}));

				// Deleting a remote branch is disabled for now
				// the code in GitRepository deletes only the local ref to the remote branch and
				// git fetch --all brings the remote branch back again
				// TODO: A different approach is required to delete a remote branch using push
				// optionsMenu->AddItem(
					// new BMenuItem(
						// fmt << B_TRANSLATE("Delete \"%selected_branch%\""),
						// new GMessage{
							// {"what", MsgDeleteBranch},
							// {"value", selected_branch},
							// {"type", GIT_BRANCH_REMOTE}}));


				optionsMenu->AddSeparatorItem();

				// We don't allow to merge a local branch into its origin
				// if (!selected_branch.EndsWith(fCurrentBranch)) {
					// optionsMenu->AddItem(
						// new BMenuItem(
							// fmt << B_TRANSLATE("Merge \"%selected_branch%\" into \"%current_branch%\""),
							// new GMessage{
								// {"what", MsgMerge},
								// {"selected_branch", selected_branch},
								// {"current_branch", fCurrentBranch}}));
					// optionsMenu->AddSeparatorItem();
				// }

				optionsMenu->AddItem(
					new BMenuItem(
						fmt << B_TRANSLATE("Create new branch from \"%selected_branch%\""),
						new GMessage{
							{"what", MsgNewBranch},
							{"value", selected_branch},
							{"type", GIT_BRANCH_REMOTE}}));

				// optionsMenu->AddItem(
					// new BMenuItem(
						// fmt << B_TRANSLATE("Create new tag from \"%selected_branch%\""),
						// new GMessage{
							// {"what", MsgNewTag},
							// {"value", selected_branch}}));

				break;
			}
			case kTag: {
				// TODO
				break;
			}
			default: {
				break;
			}
		}
	}

	optionsMenu->SetTargetForItems(Target());
	optionsMenu->Go(ConvertToScreen(where), true);
	delete optionsMenu;
}


void
RepositoryView::UpdateRepository(ProjectFolder *selectedProject, const BString &currentBranch)
{
	fSelectedProject = selectedProject;
	fRepositoryPath = fSelectedProject->Path().String();
	fCurrentBranch = currentBranch;

	try {
		auto repo = fSelectedProject->GetRepository();
		auto current_branch = repo->GetCurrentBranch();

		MakeEmpty();

		auto locals = _InitEmptySuperItem(B_TRANSLATE("Local branches"));
		// populate local branches
		auto local_branches = repo->GetBranches(GIT_BRANCH_LOCAL);
		for(auto &branch : local_branches) {
			auto item = new StyledItem(branch.String(), kLocalBranch);
			if (branch == fCurrentBranch)
				item->SetText(BString(item->Text()) << "*");
			// item->SetToolTipText(item->Text());
			AddUnder(item, locals);
		}

		auto remotes = _InitEmptySuperItem(B_TRANSLATE("Remote branches"));
		// populate remote branches
		auto remote_branches = repo->GetBranches(GIT_BRANCH_REMOTE);
		for(auto &branch : remote_branches) {
			auto item = new BranchItem(branch.String(), kRemoteBranch);
			// item->SetToolTipText(item->Text());
			AddUnder(item, remotes);
		}

		auto tags = _InitEmptySuperItem(B_TRANSLATE("Tags"));
		// populate tags
		auto all_tags = repo->GetTags();
		for(auto &tag : all_tags) {
			auto item = new BranchItem(tag.String(), kRemoteBranch);
			// item->SetToolTipText(item->Text());
			AddUnder(item, tags);
		}
	} catch (const GitException &ex) {
		OKAlert("Git", ex.Message(), B_INFO_ALERT);
		MakeEmpty();
		_InitEmptySuperItem(B_TRANSLATE("Local branches"));
		_InitEmptySuperItem(B_TRANSLATE("Remotes"));
		_InitEmptySuperItem(B_TRANSLATE("Tags"));
	}
}


BranchItem*
RepositoryView::_InitEmptySuperItem(const BString &label)
{
	auto item = new BranchItem(label);
	AddItem(item);
	return item;
}


auto
RepositoryView::GetSelectedItem()
{
	BListItem *item = nullptr;
	const int32 selection = CurrentSelection();
	if (selection >= 0)
		item = ItemAt(selection);
	return item;
}