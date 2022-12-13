/*
 * Copyright 2018 A. Mosca <amoscaster@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "ProjectParser.h"

#include <Application.h>
#include <Catalog.h>
#include <Directory.h>
#include <Entry.h>
#include <Window.h>
#include <iostream>

#include "GenioCommon.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ProjectParser"

const std::vector<std::string> ProjectParser :: source_extensions =
{ "s", "S", "awk", "c", "cpp", "cxx", "c++", "h", "rs" };

const std::vector<std::string> ProjectParser :: excluded_extensions =
{ "o", "d"};


ProjectParser::ProjectParser(TPreferences* prefs)
	:
	fPreferences(prefs)
{
	if (fPreferences == nullptr)
		throw;

	fPreferences->FindString("project_extensioned_name", &fProjectFullName);
	fPreferences->FindString("project_type", &fProjectType);
}

ProjectParser::~ProjectParser()
{
}

status_t
ProjectParser::ParseProjectFiles(BString directory)
{
	if (directory.IsEmpty())
		throw;

	fPreferences->RemoveName("project_file");
	fPreferences->RemoveName("project_source");
	fPreferences->RemoveName("project_scm");

	fSourcesCount = 0;
	fFilesCount = 0;
	parseless_found.clear();
	fParselessIn = _GetParselessItems();

	// Mind: recursive function
	_ScanDirectory(directory);

	fParselessOut = _SetParselessItems();

	BString notification;
	notification
		<< B_TRANSLATE("Project scan") << ":  " << fProjectFullName
		<< "  S = " << fSourcesCount
		<< "; F = " << fFilesCount
		<< "; Pi = " << fParselessIn
		<< "; Po = " << fParselessOut
		;
	BMessage message('NOTI');
	message.AddString("notification", notification);
	message.AddString("type", "PROJ_SCAN");
	be_app->WindowAt(0)->PostMessage(&message);

	return B_OK;
}

int32
ProjectParser::_GetParselessItems()
{
	int32 count = 0;
	BString item;

	parseless_items.clear();

	while (fPreferences->FindString("parseless_item", count++, &item) == B_OK)
		parseless_items.push_back(item);
 
	return --count;
}

void
ProjectParser::_ScanDirectory(BString directory)
{
	BDirectory dir(directory);
	BEntry entry;
	entry_ref ref;
	char name[B_FILE_NAME_LENGTH];

	// TODO: does not work with traverse on
	while ((dir.GetNextEntry(&entry)) == B_OK) {
		entry.GetName(name);
		std::string token_name(name);

		// Manage directories
		if (entry.IsDirectory()) {

			if (token_name == ".git") {
				fPreferences->SetBString("project_scm", "git");
			} else if (token_name == ".hg") {
				fPreferences->SetBString("project_scm", "hg");
			} else if (token_name == ".bzr") {
				fPreferences->SetBString("project_scm", "bzr");
			}
			// Exclude objects dir (starts with)
			else if (token_name.find("objects.") == 0) {
				;
			}
			// Exclude app dir if any
			else if (token_name == "app") {
				;
			}
			// Cargo specific
			else if (fProjectType == "cargo"
				&& token_name == "target") {
				// Exclude target dir TODO: better tune
				;
			}
			// Mind recurse quirks
			else {
				BPath newPath(directory);
				newPath.Append(entry.Name());
				_ScanDirectory(newPath.Path());
			}
		} else {
			// Entry is a file, get it
			if ((entry.GetRef(&ref)) == B_OK) {

				BPath path(&entry);
				// Get extension if any
				std::string extension("");
				if (token_name.find_last_of(".") != std::string::npos)
					extension = token_name.substr(token_name.find_last_of(".") + 1);

				// Excluded item found: save in a list
				// If a parseless item was not found it means it has been
				// deleted, so one may like to delete the reference as well
				if (Genio::_in_container(path.Path(), parseless_items))
					parseless_found.push_back(path.Path());
				// Excluded extension: skip
				else if (Genio::_in_container(extension, excluded_extensions))
					;
				// Sources list
				else if (Genio::_in_container(extension, source_extensions)) {
					fPreferences->AddString("project_source", path.Path());
					fSourcesCount++;
				}
				// Everything else is a file
				else {
					fPreferences->AddString("project_file", path.Path());
					fFilesCount++;
				}
			}
		}
	}
}

// Set parseless_items to parseless_found
// (= delete parseless items not found when scanning)
int32
ProjectParser::_SetParselessItems()
{
	BString item;

	fPreferences->RemoveName("parseless_item");

	for (auto it = parseless_found.begin(); it != parseless_found.end(); it++)
			fPreferences->AddString("parseless_item", *it);

	return parseless_found.size();
}
