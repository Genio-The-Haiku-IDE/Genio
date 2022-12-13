/*
 * Copyright 2017..2018  A. Mosca 
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "Project.h"

#include <stdexcept>

#include "GenioNamespace.h"

Project::Project(BString const& name)
	:
	fExtensionedName(name)
{
}

Project::~Project()
{
	delete fProjectTitle;
}

void
Project::Activate()
{
	isActive = true;
	fProjectTitle->Activate();
}

BString  const
Project::BuildCommand()
{
	BString command("");
	TPreferences prefs(fExtensionedName, GenioNames::kApplicationName, 'LOPR');
	prefs.FindString("project_build_command", &command);

	return command;
}

BString const
Project::CleanCommand()
{
	BString command("");
	TPreferences prefs(fExtensionedName, GenioNames::kApplicationName, 'LOPR');
	prefs.FindString("project_clean_command", &command);

	return command;
}

void
Project::Deactivate()
{
	isActive = false;
	fProjectTitle->Deactivate();
}

std::vector<BString> const
Project::FilesList()
{
	fFilesList.clear();

	TPreferences prefs(fExtensionedName, GenioNames::kApplicationName, 'LOPR');
	BString file;
	int count = 0;

	while (prefs.FindString("project_file", count++, &file) == B_OK)
		fFilesList.push_back(file);

	return fFilesList;
}

status_t
Project::Open(bool activate)
{
	if (fExtensionedName.IsEmpty())
		throw std::logic_error("Empty name");

	TPreferences idmproFile(fExtensionedName, GenioNames::kApplicationName, 'LOPR');

	isActive = activate;

	fProjectTitle = new ProjectTitleItem(fExtensionedName.String(), activate);

	// name without extension
	if (idmproFile.FindString("project_name", &fName) != B_OK)
		throw std::logic_error("Empty name");

	if (idmproFile.FindString("project_directory", &fProjectDirectory) != B_OK)
		fProjectDirectory = "";

	if (idmproFile.FindString("project_type", &fType) != B_OK)
		fType = "";

	if (idmproFile.FindBool("run_in_terminal", &fRunInTerminal) != B_OK)
		fRunInTerminal = false;

	return B_OK;
}

bool
Project::ReleaseModeEnabled()
{
	bool releaseMode = false;
	TPreferences prefs(fExtensionedName, GenioNames::kApplicationName, 'LOPR');
	prefs.FindBool("release_mode", &releaseMode);

	return releaseMode;
}

BString const
Project::Scm()
{
	BString	scm("");
	TPreferences prefs(fExtensionedName, GenioNames::kApplicationName, 'LOPR');
	prefs.FindString("project_scm", &scm);

	return scm;
}

void
Project::SetReleaseMode(bool releaseMode)
{
	TPreferences prefs(fExtensionedName, GenioNames::kApplicationName, 'LOPR');
	prefs.SetBool("release_mode", releaseMode);
}

std::vector<BString> const
Project::SourcesList()
{
	fSourcesList.clear();
	TPreferences prefs(fExtensionedName, GenioNames::kApplicationName, 'LOPR');
	BString file;
	int count = 0;

	while (prefs.FindString("project_source", count++, &file) == B_OK)
		fSourcesList.push_back(file);

	return fSourcesList;
}

BString const
Project::Target()
{
	BString target("");
	TPreferences prefs(fExtensionedName, GenioNames::kApplicationName, 'LOPR');
	prefs.FindString("project_target", &target);

	return target;
}
