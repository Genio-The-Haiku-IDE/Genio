/*
 * Copyright 2017 A. Mosca 
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "GenioApp.h"

#include <Alert.h>
#include <AboutWindow.h>
#include <Catalog.h>
#include <String.h>
#include <iostream>

#include "GenioNamespace.h"
#include "SettingsWindow.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "GenioApp"

#include "FileWrapper.h"

GenioApp::GenioApp()
	:
	BApplication(GenioNames::kApplicationSignature)
{
	
	BRect frame;

	// Load UI settings
	fUISettingsFile = new TPreferences(GenioNames::kUISettingsFileName,
										GenioNames::kApplicationName, 'UISE');

	// Load frame from settings if present or use default
	if (fUISettingsFile->FindRect("ui_bounds", &frame) != B_OK)
		frame.Set(40, 40, 839, 639);

	fGenioWindow = new GenioWindow(frame);

	fGenioWindow->Show();
}

GenioApp::~GenioApp()
{
	FileWrapper::Dispose();
}

void
GenioApp::AboutRequested()
{
	BAboutWindow* window = new BAboutWindow(GenioNames::kApplicationName,
											GenioNames::kApplicationSignature);
	
	// create the about window
	const char* authors[] = {
		"Nexus6",
		"A. Anzani",
		"S. Ceccherini",
		"...",
		NULL
	}; 

	window->AddCopyright(2022, "...");
	window->AddAuthors(authors);

	BString extraInfo;
	extraInfo << B_TRANSLATE("Genio is a fork of Ideam and available under the MIT license.");
	extraInfo << "\nIdeam (c) 2017 A. Mosca\n\n";
	extraInfo << GenioNames::kApplicationName << " " << B_TRANSLATE("uses:");
	extraInfo << "\nScintilla lib";
	extraInfo << "\nCopyright 1998-2003 by Neil Hodgson ";
	extraInfo << "\n\nScintilla for Haiku";
	extraInfo << "\nCopyright 2011 by Andrea Anzani ";
	extraInfo << "\nCopyright 2014-2015 by Kacper Kasper \n\n";
	extraInfo << B_TRANSLATE("See Credits for a complete list.");
	extraInfo << B_TRANSLATE("Made with love in Italy");

	window->AddExtraInfo(extraInfo);

	window->Show();
}

void
GenioApp::ArgvReceived(int32 agrc, char** argv)
{
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
	
	be_app->PostMessage(B_QUIT_REQUESTED);
	
	return true;
}

void
GenioApp::RefsReceived(BMessage* message)
{
	fGenioWindow->PostMessage(message);
}

void
GenioApp::ReadyToRun()
{
	// Window Settings file needs updating?
	_CheckSettingsVersion();

	std::cerr << GenioNames::GetSignature() << std::endl;
	

}

void
GenioApp::_CheckSettingsVersion()
{
	BString fileVersion("");
	int32 result;

	TPreferences* settings = new TPreferences(GenioNames::kSettingsFileName,
												GenioNames::kApplicationName, 'IDSE');
	settings->FindString("app_version", &fileVersion);
	delete settings;

	// Settings file missing or corrupted
	if (fileVersion.IsEmpty() || fileVersion == "0.0.0.0") {

		BString text;
		text << B_TRANSLATE("Settings file is corrupted or deleted,")
			 << "\n"
			 << B_TRANSLATE("do You want to ignore, review or load to defaults?");

		BAlert* alert = new BAlert("SettingsDeletedDialog", text,
			B_TRANSLATE("Ignore"), B_TRANSLATE("Review"), B_TRANSLATE("Load"),
			B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);

		alert->SetShortcut(0, B_ESCAPE);

		int32 choice = alert->Go();
	 
		if (choice == 0)
			;
		else if (choice == 1) {
			// Fill file with defaults
			GenioNames::UpdateSettingsFile();
			SettingsWindow* window = new SettingsWindow();
			window->Show();
		}
		else if (choice == 2) {
			GenioNames::UpdateSettingsFile();
			GenioNames::LoadSettingsVars();
		}
	}
	else {

		result = GenioNames::CompareVersion(GenioNames::GetVersionInfo(), fileVersion);
		// App version > file version
		if (result > 0) {

			BString text;
			text << B_TRANSLATE("Settings file for a previous version detected,")
				 << "\n"
				 << B_TRANSLATE("do You want to ignore, review or load to defaults?");

			BAlert* alert = new BAlert("SettingsUpdateDialog", text,
				B_TRANSLATE("Ignore"), B_TRANSLATE("Review"), B_TRANSLATE("Load"),
				B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);

			alert->SetShortcut(0, B_ESCAPE);

			int32 choice = alert->Go();
		 
			if (choice == 0)
				;
			else if (choice == 1) {
				SettingsWindow* window = new SettingsWindow();
				window->Show();
			}
			else if (choice == 2) {
				GenioNames::UpdateSettingsFile();
				GenioNames::LoadSettingsVars();
			}
		}
	}
}

int
main(int argc, char* argv[])
{
	try {
		GenioApp *app = new GenioApp();

		app->Run();

		delete app;
	} catch (...) {

		debugger("Exception caught.");

	}

	return 0;
}
