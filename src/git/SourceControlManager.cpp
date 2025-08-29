/*
 * Copyright 2025, The Genio Team
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "SourceControlManager.h"

#include "GenioWindowMessages.h"
#include "GitRepository.h"
#include "ProjectFolder.h"

SourceControlManager::SourceControlManager(const char* fullPath)
	:
	fProject(nullptr)
{
	fGitRepository = new GitRepository(fullPath);
}


void
SourceControlManager::SwitchBranch(const char* branch)
{
	fGitRepository->SwitchBranch(branch);
	
	_BranchChanged(branch);
}


/* virtual */
void
SourceControlManager::MessageReceived(BMessage* message)
{
	switch (message->what) {
		//case B_PATH_MONITOR:
		//	break;
		default:
			BLooper::MessageReceived(message);
			break;
	}
}


void
SourceControlManager::_BranchChanged(const char* branch)
{
	BMessage message(MSG_NOTIFY_GIT_BRANCH_CHANGED);
	message.AddString("current_branch", branch);
	if (fProject != nullptr) {
		message.AddString("project_name", fProject->Name());
		message.AddString("project_path", fProject->Path());
	}
	SendNotices(MSG_NOTIFY_GIT_BRANCH_CHANGED, &message);
}

