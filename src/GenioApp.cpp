/*
 * Copyright 2017 A. Mosca <amoscaster@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "GenioApp.h"

#include <Alert.h>
#include <AboutWindow.h>
#include <Catalog.h>
#include <String.h>
#include <StringList.h>

#include <getopt.h>


#include "GenioNamespace.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "GenioApp"

#include "Log.h"

static log_level sSessionLogLevel = log_level(LOG_LEVEL_UNSET);

const char kChangeLog[] = {
#include "Changelog.h"
};

static BStringList
SplitChangeLog(const char* changeLog)
{
	BStringList list;
	char* stringStart = (char*)changeLog;
	int i = 0;
	char c;
	while ((c = stringStart[i]) != '\0') {
		if (c == '-'  && i > 2 && stringStart[i - 1] == '-' && stringStart[i - 2] == '-') {
			BString string;
			string.Append(stringStart, i - 2);
			string.RemoveAll("\t");
			string.ReplaceAll("- ", "\n\t- ");
			list.Add(string);
			stringStart = stringStart + i + 1;
			i = 0;
		} else
			i++;
	}
	return list;
}


GenioApp::GenioApp()
	:
	BApplication(GenioNames::kApplicationSignature),
	fGenioWindow(nullptr),
	fUISettingsFile(nullptr)
{
	// Load UI settings
	fUISettingsFile = new TPreferences(GenioNames::kUISettingsFileName,
										GenioNames::kApplicationName, 'UISE');
}

GenioApp::~GenioApp()
{
}

void
GenioApp::AboutRequested()
{
	BAboutWindow* window = new BAboutWindow(GenioNames::kApplicationName,
											GenioNames::kApplicationSignature);

	// create the about window
	const char* authors[] = {
		"D. Alfano",
		"A. Anzani",
		"S. Ceccherini",
		NULL
	};

	window->AddCopyright(2023, "The Genio Team");
	window->AddAuthors(authors);

	BStringList list = SplitChangeLog(kChangeLog);
	int32 stringCount = list.CountStrings();
	char** charArray = new char* [stringCount + 1];
	for (int32 i = 0; i < stringCount; i++) {
		charArray[i] = (char*)list.StringAt(i).String();
	}
	charArray[stringCount] = NULL;

	window->AddVersionHistory((const char**)charArray);
	delete[] charArray;

	BString extraInfo;
	extraInfo << B_TRANSLATE("Genio is a fork of Ideam and available under the MIT license.");
	extraInfo << "\nIdeam (c) 2017 A. Mosca\n\n";
	extraInfo << B_TRANSLATE("Genio uses:"
		"\nScintilla lib"
		"\nCopyright 1998-2003 by Neil Hodgson <neilh@scintilla.org>"
		"\n\nScintilla for Haiku"
		"\nCopyright 2011 by Andrea Anzani <andrea.anzani@gmail.com>"
		"\nCopyright 2014-2015 by Kacper Kasper <kacperkasper@gmail.com>\n\n");
	extraInfo << B_TRANSLATE("See credits for a complete list.\n\n");
	extraInfo << B_TRANSLATE("Made with love in Italy");

	window->AddExtraInfo(extraInfo);
	window->ResizeBy(0, 200);

	window->Show();
}


void
GenioApp::ArgvReceived(int32 argc, char** argv)
{
	BApplication::ArgvReceived(argc, argv);
	if (argc == 0)
		return;

	BMessage *message = new BMessage(B_REFS_RECEIVED);
	int i = 0;
	while (i < argc) {
		entry_ref ref;
		if (get_ref_for_path(argv[i], &ref) == B_OK) {
			message->AddRef("refs", &ref);
		}
		i++;
	}

	PostMessage(message);
}


void
GenioApp::MessageReceived(BMessage* message)
{
	switch (message->what) {
		default:
			BApplication::MessageReceived(message);
			break;
	}

}

bool
GenioApp::QuitRequested()
{
	// Manage settings counter
	int32 count;
	if (fUISettingsFile->FindInt32("use_count", &count) != B_OK)
		count = 0;

	// Check if window position was modified
	BRect actualFrame, savedFrame;
	fUISettingsFile->FindRect("ui_bounds", &savedFrame);
	actualFrame = fGenioWindow->ConvertToScreen(fGenioWindow->Bounds());

	// Automatically save window position via TPreferences
	// only if modified
	if (actualFrame != savedFrame) {
		// Check if settings are available and apply
		if (fUISettingsFile->InitCheck() == B_OK) {
			fUISettingsFile->SetRect("ui_bounds", actualFrame);
			fUISettingsFile->SetInt64("last_used", real_time_clock());
			fUISettingsFile->SetInt32("use_count", ++count);
		}
	}

	delete fUISettingsFile;

	return BApplication::QuitRequested();
}


void
GenioApp::RefsReceived(BMessage* message)
{
	if (fGenioWindow != NULL)
		fGenioWindow->PostMessage(message);
}


void
GenioApp::ReadyToRun()
{
	// Fill Settings vars before using
	GenioNames::LoadSettingsVars();

	Logger::SetDestination(GenioNames::Settings.log_destination);
	if (sSessionLogLevel == LOG_LEVEL_UNSET)
		Logger::SetLevel(log_level(GenioNames::Settings.log_level));
	else
		Logger::SetLevel(sSessionLogLevel);

	// Load frame from settings if present or use default
	BRect frame;
	if (fUISettingsFile->FindRect("ui_bounds", &frame) != B_OK)
		frame.Set(40, 40, 839, 639);

	fGenioWindow = new GenioWindow(frame);
	fGenioWindow->Show();
}


void
SetSessionLogLevel(char level)
{
	switch(level) {
		case 'o':
			sSessionLogLevel = log_level(1);
			printf("Log level set to OFF\n");
		break;
		case 'e':
			sSessionLogLevel = log_level(2);
			printf("Log level set to ERROR\n");
		break;
		case 'i':
			sSessionLogLevel = log_level(3);
			printf("Log level set to INFO\n");
		break;
		case 'd':
			sSessionLogLevel = log_level(4);
			printf("Log level set to DEBUG\n");
		break;
		case 't':
			sSessionLogLevel = log_level(5);
			printf("Log level set to TRACE\n");
		break;
		default:
			LogFatal("Invalid log level, valid levels are: o, e, i, d, t");
		break;
	}
}


static
struct option sLongOptions[] = {
		{ "loglevel", required_argument, 0, 'l' },
		{ 0, 0, 0, 0 }
};


static int
HandleArgs(int argc, char **argv)
{
	int optIndex = 0;
	int c = 0;
	while ((c = ::getopt_long(argc, argv, "l:",
			sLongOptions, &optIndex)) != -1) {
		switch (c) {
			case 'l':
				SetSessionLogLevel(optarg[0]);
				break;
			case 0:
			{
				std::string optName = sLongOptions[optIndex].name;
				if (optName == "loglevel")
					SetSessionLogLevel(optarg[0]);
				break;
			}
		}
	}

	return optIndex;
}


int
main(int argc, char* argv[])
{
	int nextArg = HandleArgs(argc, argv);
	try {
		GenioApp *app = new GenioApp();
		if (nextArg < argc)
			app->ArgvReceived(argc - nextArg, &argv[nextArg]);
		app->Run();

		delete app;
	} catch (...) {
		debugger("Exception caught.");
	}

	return 0;
}
