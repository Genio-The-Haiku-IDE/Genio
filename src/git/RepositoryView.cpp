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

	if (Target()->LockLooper()) {
		Target()->StartWatching(this, MsgChangeProject);
		Target()->StartWatching(this, MsgSwitchBranch);
		Target()->UnlockLooper();
	}
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
		case B_OBSERVER_NOTICE_CHANGE: {
			int32 code;
			message->FindInt32(B_OBSERVE_WHAT_CHANGE, &code);
			switch (code) {
				case MsgChangeProject:
				{
					LogInfo("RepositoryView: MsgChangeProject");
					fSelectedProject = (ProjectFolder *)message->GetPointer("value");
					if (fSelectedProject!=nullptr)
						fRepositoryPath = fSelectedProject->Path().String();
					_UpdateRepository();
					break;
				}
				case MsgSwitchBranch:
				{
					LogInfo("RepositoryView: MsgSwitchBranch");
					fCurrentBranch = message->GetString("value");
					if (!fCurrentBranch.IsEmpty())
						_UpdateRepository();
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

				optionsMenu->AddItem(
					new BMenuItem(
						fmt << B_TRANSLATE("Switch to \"%selected_branch%\""),
						new GMessage{
							{"what", MsgSwitchBranch},
							{"selected_branch", selected_branch}
						}
					)
				);

				optionsMenu->AddItem(new BMenuItem(fmt << B_TRANSLATE("Rename \"%selected_branch%\""),
					new BMessage(2)));

				optionsMenu->AddItem(new BMenuItem(fmt << B_TRANSLATE("Delete \"%selected_branch%\""),
					new BMessage(2)));

				optionsMenu->AddSeparatorItem();

				optionsMenu->AddItem(
					new BMenuItem(
						fmt << B_TRANSLATE("Pull \"origin/%selected_branch%\""),
					new BMessage(MsgPull)));

				optionsMenu->AddItem(new BMenuItem(fmt << B_TRANSLATE("Push \"%selected_branch%\" to \"origin\\%selected_branch%\""),
					new BMessage(2)));

				optionsMenu->AddSeparatorItem();

				optionsMenu->AddItem(new BMenuItem(fmt << B_TRANSLATE("Create new branch from \"%selected_branch%\""),
					new BMessage(2)));

				optionsMenu->AddItem(new BMenuItem(fmt << B_TRANSLATE("Create new tag from \"%selected_branch%\""),
					new BMessage(2)));

				break;
			}
			case kRemoteBranch: {

				fmt.Substitutions["%current_branch%"] = fCurrentBranch;
				LogInfo("fmt.Substitutions[%current_branch%] = %s", fCurrentBranch.String());

				optionsMenu->AddItem(
					new BMenuItem(
						fmt << B_TRANSLATE("Checkout \"%selected_branch%\""),
						new GMessage{
							{"what", MsgSwitchBranch},
							{"selected_branch", selected_branch}
						}
					)
				);

				optionsMenu->AddItem(new BMenuItem(fmt << B_TRANSLATE("Delete \"%selected_branch%\""),
					new BMessage(2)));

				optionsMenu->AddSeparatorItem();

				// We don't allow to merge a local branch into its origin
				if (fCurrentBranch != selected_branch) {
					optionsMenu->AddItem(
						new BMenuItem(
							fmt << B_TRANSLATE("Merge \"%selected_branch%\" into \"%current_branch%\""),
							new GMessage{
								{"what", MsgPull},
								{"selected_branch", selected_branch},
								{"current_branch", fCurrentBranch}
							}
						)
					);

					optionsMenu->AddSeparatorItem();
				}

				optionsMenu->AddItem(new BMenuItem(fmt << B_TRANSLATE("Create new branch from \"%selected_branch%\""),
					new BMessage(2)));

				optionsMenu->AddItem(new BMenuItem(fmt << B_TRANSLATE("Create new tag from \"%selected_branch%\""),
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
	MakeEmpty();

	auto locals = new StyledItem(this, B_TRANSLATE("Local branches"));
	locals->SetPrimaryTextStyle(B_BOLD_FACE);
	AddItem(locals);
	// populate local branches
	if (fSelectedProject != nullptr) {
		Genio::Git::GitRepository repo(fSelectedProject->Path().String());
		auto branches = repo.GetBranches(GIT_BRANCH_LOCAL);
		auto current_branch = repo.GetCurrentBranch();
		for(auto &branch : branches) {
			auto item = new StyledItem(this, branch.String(), kLocalBranch);
			AddUnder(item, locals);
		}
	}

	auto remotes = new StyledItem(this, B_TRANSLATE("Remotes"));
	remotes->SetPrimaryTextStyle(B_BOLD_FACE);
	AddItem(remotes);
	// populate remote branches
	if (fSelectedProject != nullptr) {
		Genio::Git::GitRepository repo(fSelectedProject->Path().String());
		auto branches = repo.GetBranches(GIT_BRANCH_REMOTE);
		auto current_branch = repo.GetCurrentBranch();
		for(auto &branch : branches) {
			auto item = new StyledItem(this, branch.String(), kRemoteBranch);
			AddUnder(item, remotes);
		}
	}

	auto tags = new StyledItem(this, B_TRANSLATE("Tags"));
	tags->SetPrimaryTextStyle(B_BOLD_FACE);
	AddItem(tags);
	// populate tags
	if (fSelectedProject != nullptr) {
		Genio::Git::GitRepository repo(fSelectedProject->Path().String());
		auto all_tags = repo.GetTags();
		auto current_branch = repo.GetCurrentBranch();
		for(auto &tag : all_tags) {
			auto item = new StyledItem(this, tag.String(), kRemoteBranch);
			AddUnder(item, tags);
		}
	}
}