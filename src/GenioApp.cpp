/*
 * Copyright 2017 A. Mosca 
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "GenioApp.h"

#include <Alert.h>
#include <AboutWindow.h>
#include <Catalog.h>
#include <String.h>
#include <StringList.h>

#include <getopt.h>
#include "LSPLogLevels.h"
#include "ConfigManager.h"
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
	fGenioWindow(nullptr)
{
	find_directory(B_USER_SETTINGS_DIRECTORY, &fConfigurationPath);
	fConfigurationPath.Append(GenioNames::kApplicationName);
	fConfigurationPath.Append(GenioNames::kSettingsFileName);
}

GenioApp::~GenioApp()
{
	// Save settings on quit, anyway
	gCFG.SaveToFile(fConfigurationPath);
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
		"\nCopyright 1998-2003 by Neil Hodgson "
		"\n\nScintilla for Haiku"
		"\nCopyright 2011 by Andrea Anzani "
		"\nCopyright 2014-2015 by Kacper Kasper \n\n");
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
		case B_OBSERVER_NOTICE_CHANGE: {
			int32 code;
			message->FindInt32(B_OBSERVE_WHAT_CHANGE, &code);
			switch (code) {
				case MSG_NOTIFY_CONFIGURATION_UPDATED:
				{
					// TODO: Long list of strcmp
					const char* key = NULL;
					message->FindString("key", &key);
					if (key != NULL) {
						if (strcmp(key, "log_destination") == 0)
							Logger::SetDestination(gCFG["log_destination"]);
						else if (strcmp(key, "log_level") == 0
							&& sSessionLogLevel == LOG_LEVEL_UNSET)
							Logger::SetLevel(log_level(int32(gCFG["log_level"])));
					}
					gCFG.SaveToFile(fConfigurationPath);
					LogInfo("Configuration file saved! (updating %s)", message->GetString("key", "ERROR!"));
				}
				break;
			default:
				break;
			};
		}
		break;
		default:
			BApplication::MessageReceived(message);
			break;
	}

}

bool
GenioApp::QuitRequested()
{
	gCFG["ui_bounds"] = fGenioWindow->ConvertToScreen(fGenioWindow->Bounds());

	if (Logger::IsDebugEnabled()) {
		gCFG.PrintValues();
	}

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
	PrepareConfig(gCFG);

	// Global settings file check.
	if (gCFG.LoadFromFile(fConfigurationPath) != B_OK) {
		LogInfo("Cannot load global settings file");
	}

	// let's subscribe config changes updates
	StartWatching(this, MSG_NOTIFY_CONFIGURATION_UPDATED);

	Logger::SetDestination(gCFG["log_destination"]);
	if (sSessionLogLevel == LOG_LEVEL_UNSET)
		Logger::SetLevel(log_level(int32(gCFG["log_level"])));
	else
		Logger::SetLevel(sSessionLogLevel);

	fGenioWindow = new GenioWindow(gCFG["ui_bounds"]);
	fGenioWindow->Show();
}

// These settings will show up in the ConfigWindow.
// Use this context to avoid invalidating previous translations
#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SettingsWindow"


void
GenioApp::PrepareConfig(ConfigManager& cfg)
{
	cfg.AddConfig("General", "projects_directory", B_TRANSLATE("Projects folder:"), "/boot/home/workspace");
	cfg.AddConfig("General", "fullpath_title", B_TRANSLATE("Show full path in window title"), true);
	GMessage loggers = {{ {"mode", "options"} }};
	loggers["option_1"]["value"] = (int32)Logger::LOGGER_DEST_STDOUT;
	loggers["option_1"]["label"] = "Stdout";
	loggers["option_2"]["value"] = (int32)Logger::LOGGER_DEST_STDERR;
	loggers["option_2"]["label"] = "Stderr";
	loggers["option_3"]["value"] = (int32)Logger::LOGGER_DEST_SYSLOG;
	loggers["option_3"]["label"] = "Syslog";
	loggers["option_4"]["value"] = (int32)Logger::LOGGER_DEST_BEDC;
	loggers["option_4"]["label"] = "BeDC";
	cfg.AddConfig("General", "log_destination", B_TRANSLATE("Log destination:"), (int32)Logger::LOGGER_DEST_STDOUT, &loggers);

	GMessage levels = {{ {"mode", "options"} }};
	levels["option_1"]["value"] = (int32)LOG_LEVEL_OFF;
	levels["option_1"]["label"] = "Off";
	levels["option_2"]["value"] = (int32)LOG_LEVEL_ERROR;
	levels["option_2"]["label"] = "Error";
	levels["option_3"]["value"] = (int32)LOG_LEVEL_INFO;
	levels["option_3"]["label"] = "Info";
	levels["option_4"]["value"] = (int32)LOG_LEVEL_DEBUG;
	levels["option_4"]["label"] = "Debug";
	levels["option_5"]["value"] = (int32)LOG_LEVEL_TRACE;
	levels["option_5"]["label"] = "Trace";
	cfg.AddConfig("General", "log_level", B_TRANSLATE("Log level:"), (int32)LOG_LEVEL_ERROR, &levels);

	cfg.AddConfig("General/Startup", "reopen_projects", B_TRANSLATE("Reload projects"), true);
	cfg.AddConfig("General/Startup", "reopen_files", B_TRANSLATE("Reload files"), true);
	cfg.AddConfig("General/Startup", "show_projects", B_TRANSLATE("Show projects pane"), true);
	cfg.AddConfig("General/Startup", "show_output", B_TRANSLATE("Show output pane"), true);
	cfg.AddConfig("General/Startup", "show_toolbar", B_TRANSLATE("Show toolbar"), true);

	GMessage sizes;
	sizes = {{ {"mode","options"} }};
	sizes["option_1"]["value"] = -1;
	sizes["option_1"]["label"] = B_TRANSLATE("Default size");
	int32 c = 2;
	for (int32 i = 10; i <= 18; i++) {
		BString key("option_");
		key << c;
		BString text;
		text << i;
		sizes[key.String()]["value"] = i;
		sizes[key.String()]["label"] = text.String();
		c++;
	}

	cfg.AddConfig("Editor", "edit_fontsize", B_TRANSLATE("Font size:"), -1, &sizes);
	cfg.AddConfig("Editor", "syntax_highlight", B_TRANSLATE("Enable syntax highlighting"), true);
	cfg.AddConfig("Editor", "brace_match", B_TRANSLATE("Enable brace matching"), true);
	cfg.AddConfig("Editor", "save_caret", B_TRANSLATE("Save caret position"), true);
	cfg.AddConfig("Editor", "trim_trailing_whitespace", B_TRANSLATE("Trim trailing whitespace on save"), false);
	GMessage tabs = {{ {"min",1},{"max",8} }};
	cfg.AddConfig("Editor", "tab_width", B_TRANSLATE("Tab width:  "), 4, &tabs);
	GMessage zooms = {{ {"min", -9}, {"max", 19} }};
	cfg.AddConfig("Editor", "editor_zoom", B_TRANSLATE("Editor zoom:"), 0, &zooms);

	cfg.AddConfig("Editor/Visual", "show_linenumber", B_TRANSLATE("Show line number"), true);
	cfg.AddConfig("Editor/Visual", "show_commentmargin", B_TRANSLATE("Show comment margin"), true);
	cfg.AddConfig("Editor/Visual", "mark_caretline", B_TRANSLATE("Mark caret line"), true);
	cfg.AddConfig("Editor/Visual", "enable_folding", B_TRANSLATE("Enable folding"), true);
	cfg.AddConfig("Editor/Visual", "show_white_space", B_TRANSLATE("Show whitespace"), false);
	cfg.AddConfig("Editor/Visual", "show_line_endings", B_TRANSLATE("Show line endings"), false);

	cfg.AddConfig("Editor/Visual", "show_edgeline", B_TRANSLATE("Show edge line"), true);
	GMessage limits = {{ {"min", 0}, {"max", 500} }};
	cfg.AddConfig("Editor/Visual", "edgeline_column", B_TRANSLATE("Edge column"), 80, &limits);

	cfg.AddConfig("Build", "wrap_console",   B_TRANSLATE("Wrap console"), false);
	cfg.AddConfig("Build", "console_banner", B_TRANSLATE("Console banner"), true);
	cfg.AddConfig("Build", "build_on_save",  B_TRANSLATE("Auto-Build on resource save"), false);
	cfg.AddConfig("Build", "save_on_build",  B_TRANSLATE("Auto-Save changed files when building"), false);

	//New config, in Genio currently without a UI
	cfg.AddConfig("Editor/Find", "find_wrap", B_TRANSLATE("Wrap"), false);
	cfg.AddConfig("Editor/Find", "find_whole_word", B_TRANSLATE("Whole word"), false);
	cfg.AddConfig("Editor/Find", "find_match_case", B_TRANSLATE("Match case"), false);


	GMessage lsplevels = {{ {"mode", "options"},
							{"note", B_TRANSLATE("This setting will be updated on restart")}
						 }};

	lsplevels["option_1"]["value"] = (int32)lsp_log_level::LSP_LOG_LEVEL_ERROR;
	lsplevels["option_1"]["label"] = "Error";
	lsplevels["option_2"]["value"] = (int32)lsp_log_level::LSP_LOG_LEVEL_INFO;
	lsplevels["option_2"]["label"] = "Info";
	lsplevels["option_3"]["value"] = (int32)lsp_log_level::LSP_LOG_LEVEL_TRACE;
	lsplevels["option_3"]["label"] = "Trace";

	cfg.AddConfig("LSP", "lsp_log_level", B_TRANSLATE("Log level:"), (int32)lsp_log_level::LSP_LOG_LEVEL_ERROR, &lsplevels);

	cfg.AddConfig("Hidden", "ui_bounds", "", BRect(40, 40, 839, 639));
}


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "GenioApp"

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
