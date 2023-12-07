/*
 * Copyright 2023, Nexus6 
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
			BranchItem *item = dynamic_cast<BranchItem*>(ItemAt(index));
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


/* virtual */
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


/* virtual */
void
RepositoryView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kInvocationMessage: {
			auto item = dynamic_cast<BranchItem*>(ItemAt(CurrentSelection()));
			if (item == nullptr)
				break;
			if (item->BranchName() == fCurrentBranch)
				break;
			switch (item->BranchType()) {
				case kLocalBranch: {
					auto switch_message = new GMessage{
						{"what", MsgSwitchBranch},
						{"value", item->BranchName()},
						{"type", GIT_BRANCH_LOCAL},
						{"sender", kSenderRepositoryPopupMenu}};
						BMessenger messenger(Target());
						messenger.SendMessage(switch_message);
					break;
				}
				case kRemoteBranch: {
					auto switch_message = new GMessage{
						{"what", MsgSwitchBranch},
						{"value", item->BranchName()},
						{"type", GIT_BRANCH_REMOTE},
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


/* virtual */
void
RepositoryView::SelectionChanged()
{
}


BListItem*
RepositoryView::GetSelectedItem()
{
	BListItem *item = nullptr;
	const int32 selection = CurrentSelection();
	if (selection >= 0)
		item = ItemAt(selection);
	return item;
}


void
RepositoryView::UpdateRepository(ProjectFolder *selectedProject, const BString &currentBranch)
{
	fSelectedProject = selectedProject;
	fRepositoryPath = fSelectedProject->Path().String();
	fCurrentBranch = currentBranch;

	auto const NullLambda = [](const auto& val){ return false; };

	try {
		auto repo = fSelectedProject->GetRepository();
		auto current_branch = repo->GetCurrentBranch();

		MakeEmpty();

		// populate local branches
		auto locals = _InitEmptySuperItem(B_TRANSLATE("Local branches"));
		auto local_branches = repo->GetBranches(GIT_BRANCH_LOCAL);
		for(auto &branch : local_branches) {
			_BuildBranchTree(branch, locals, kLocalBranch,
				[&](const auto &branchname) {
					return (branchname == fCurrentBranch);
				});
		}

		// populate remote branches
		auto remotes = _InitEmptySuperItem(B_TRANSLATE("Remote branches"));
		auto remote_branches = repo->GetBranches(GIT_BRANCH_REMOTE);
		for(auto &branch : remote_branches) {
			_BuildBranchTree(branch, remotes, kRemoteBranch, NullLambda);
		}

		// populate tags
		auto tags = _InitEmptySuperItem(B_TRANSLATE("Tags"));
		auto all_tags = repo->GetTags();
		for(auto &tag : all_tags) {
			_BuildBranchTree(tag, tags, kTag, NullLambda);
		}
	} catch (const GitException &ex) {
		OKAlert("Git", ex.Message(), B_INFO_ALERT);
		MakeEmpty();
		_InitEmptySuperItem(B_TRANSLATE("Local branches"));
		_InitEmptySuperItem(B_TRANSLATE("Remotes"));
		_InitEmptySuperItem(B_TRANSLATE("Tags"));
	}
}


void
RepositoryView::_BuildBranchTree(const BString &branch, BranchItem *rootItem, uint32 branchType,
									const auto& checker)
{
	// Do not show an outline
	if (!gCFG["repository_outline"]) {
		StyledItem* item = new BranchItem(branch.String(), branch.String(), branchType);
		if (checker(branch))
			item->SetTextFontFace(B_UNDERSCORE_FACE);
		AddUnder(item, rootItem);
		return;
	}

	// show the outline
	BranchItem *parentitem = rootItem;
	std::filesystem::path path = branch.String();
	vector<std::string> parts(path.begin(), path.end());
	for (uint32 i = 0; i < parts.size(); i++) {
		uint32 lastIndex = parts.size();

		BString partName = parts.at(i).c_str();
		auto partItem = FindItem<BranchItem>(partName, rootItem, false, i+1);
		if (partItem != nullptr) {
			parentitem = partItem;
		} else {
			if (i == (lastIndex-1)) {
				auto newItem = new BranchItem(branch.String(), partName, branchType, i);
				if (checker(branch))
					newItem->SetTextFontFace(B_UNDERSCORE_FACE);
				AddUnder(newItem, parentitem);
				parentitem = rootItem;
			} else {
				auto newItem = new BranchItem(branch.String(), partName, kHeader, i);
				if (AddUnder(newItem, parentitem))
					parentitem = newItem;
			}
		}
	}
}


template <typename T>
T*
RepositoryView::FindItem(const BString& name, T* startItem, bool oneLevelOnly, uint32 outlinelevel)
{
	const int32 countItems = FullListCountItems();
	const int32 startIndex = 0;
	// LogInfo("FindItem: item '%s' under '%s' with countItems = %d, startIndex = %d",
		// name.String(), startItem->Text(), countItems, startIndex);
	for (int32 i = startIndex; i< countItems; i++) {
		// LogInfo("FindItem: for i = %d to %d", i, countItems);
		T *item = dynamic_cast<T*>(ItemUnderAt(startItem, oneLevelOnly, i));
		// LogInfo("FindItem: current item '%s' at %d", item->Text(), i);
		if (item != nullptr &&
			name.ICompare(item->Text()) == 0 &&
			item->OutlineLevel() == outlinelevel) {
			// LogInfo("FindItem: *** found item '%s' at level", item->Text(), item->OutlineLevel());
			return item;
		}
	}

	return nullptr;
}


void
RepositoryView::_ShowPopupMenu(BPoint where)
{
	auto optionsMenu = new BPopUpMenu("Options", false, false);
	auto index = IndexOf(where);
	if (index >= 0) {
		auto item = dynamic_cast<BranchItem*>(ItemAt(index));
		if (item == nullptr) {
			delete optionsMenu;
			return;
		}
		auto itemType = item->BranchType();
		BString selectedBranch(item->BranchName());
		selectedBranch.RemoveLast("*");

		StringFormatter fmt;
		fmt.Substitutions["%selected_branch%"] = selectedBranch;

		switch (itemType) {
			case kLocalBranch: {

				if (selectedBranch != fCurrentBranch) {
					optionsMenu->AddItem(
						new BMenuItem(
							fmt << B_TRANSLATE("Switch to \"%selected_branch%\""),
							new GMessage{
								{"what", MsgSwitchBranch},
								{"value", selectedBranch},
								{"type", GIT_BRANCH_LOCAL},
								{"sender", kSenderRepositoryPopupMenu}}));
				}

				optionsMenu->AddItem(
					new BMenuItem(
						fmt << B_TRANSLATE("Rename \"%selected_branch%\""),
						new GMessage{
							{"what", MsgRenameBranch},
							{"value", selectedBranch},
							{"type", GIT_BRANCH_LOCAL}}));

				optionsMenu->AddItem(
					new BMenuItem(
						fmt << B_TRANSLATE("Delete \"%selected_branch%\""),
						new GMessage{
							{"what", MsgDeleteBranch},
							{"value", selectedBranch},
							{"type", GIT_BRANCH_LOCAL}}));

				optionsMenu->AddSeparatorItem();

				optionsMenu->AddItem(
					new BMenuItem(
						fmt << B_TRANSLATE("Create new branch from \"%selected_branch%\""),
						new GMessage{
							{"what", MsgNewBranch},
							{"value", selectedBranch},
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
							{"value", selectedBranch},
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
							{"value", selectedBranch},
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
				delete optionsMenu;
				return;
			}
		}

		optionsMenu->AddItem(
			new BMenuItem(
				fmt << B_TRANSLATE("Copy name"),
				new GMessage{
					{"what", MsgCopyRefName},
					{"value", selectedBranch}}));
	}

	optionsMenu->SetTargetForItems(Target());
	optionsMenu->Go(ConvertToScreen(where), true);
	delete optionsMenu;
}


BranchItem*
RepositoryView::_InitEmptySuperItem(const BString &label)
{
	auto item = new BranchItem(label, label, kHeader);
	item->SetTextFontFace(B_BOLD_FACE);
	AddItem(item);
	return item;
}
