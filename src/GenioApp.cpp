/*
 * Copyright 2017 A. Mosca 
 * Copyright 2023, The Genio Team
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "GenioApp.h"

#include <Alert.h>
#include <AboutWindow.h>
#include <Bitmap.h>
#include <Catalog.h>
#include <FindDirectory.h>
#include <String.h>

#include <getopt.h>

#include "ConfigManager.h"
#include "GenioNamespace.h"
#include "GenioWindow.h"
#include "Languages.h"
#include "Log.h"
#include "LSPLogLevels.h"
#include "Styler.h"
#include "Utils.h"
#include "LSPServersManager.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "GenioApp"


const int32 MSG_NOTIFY_CONFIGURATION_UPDATED = 'noCU';

ConfigManager gCFG(MSG_NOTIFY_CONFIGURATION_UPDATED);

static
struct option sLongOptions[] = {
		{ "loglevel", required_argument, nullptr, 'l' },
		{ nullptr, 0, nullptr, 0 }
};

const char kChangeLog[] = {
#include "Changelog.h"
};


GenioApp::GenioApp()
	:
	BApplication(GenioNames::kApplicationSignature),
	fGenioWindow(nullptr)
{
	find_directory(B_USER_SETTINGS_DIRECTORY, &fConfigurationPath);
	fConfigurationPath.Append(GenioNames::kApplicationName);
	fConfigurationPath.Append(GenioNames::kSettingsFileName);

	_PrepareConfig(gCFG);

	// Global settings file check.
	if (gCFG.LoadFromFile({fConfigurationPath}) != B_OK) {
		LogInfo("Cannot load global settings file");
	}

	Logger::SetDestination(gCFG["log_destination"]);
	Logger::SetLevel(log_level(int32(gCFG["log_level"])));

	Languages::LoadLanguages();

	LSPServersManager::InitLSPServersConfig();

	fGenioWindow = new GenioWindow(BRect(gCFG["ui_bounds"]));
}


GenioApp::~GenioApp()
{
	// Save settings on quit, anyway
	gCFG.SaveToFile({fConfigurationPath});
	LSPServersManager::DisposeLSPServersConfig();
}


void
GenioApp::AboutRequested()
{
	BAboutWindow* window = new BAboutWindow(GenioNames::kApplicationName,
											GenioNames::kApplicationSignature);

	// create the about window
	const char* authors[] = {
		"Nexus6",
		"Andrea Anzani",
		"Stefano Ceccherini",
		NULL
	};

	const char* contributors[] = {
		"Humdinger",
		"Máximo Castañeda",
		NULL
	};

	window->AddCopyright(2022, "The Genio Team");
	window->AddAuthors(authors);
	window->AddText(B_TRANSLATE("Contributors:"), contributors);
	window->SetVersion(GetVersion().String());

	BStringList list = _SplitChangeLog(kChangeLog);
	int32 stringCount = list.CountStrings();
	char** charArray = new char* [stringCount + 1];
	for (int32 i = 0; i < stringCount; i++) {
		charArray[i] = const_cast<char*>(list.StringAt(i).String());
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

	//xmas-icon!
	if (IsXMasPeriod() && window->Icon()) {
		GetVectorIcon("xmas-icon", window->Icon());
	}

	window->Show();
}


void
GenioApp::ArgvReceived(int32 argc, char** argv)
{
	BApplication::ArgvReceived(argc, argv);
	if (argc == 1)
		return;

	int i = _HandleArgs(argc, argv);
	if (i <= 0)
		return;

	BMessage message(B_REFS_RECEIVED);
	while (i < argc) {
		entry_ref ref;
		if (get_ref_for_path(argv[i], &ref) == B_OK) {
			message.AddRef("refs", &ref);
		}
		i++;
	}
	if (message.HasRef("refs"))
		PostMessage(&message);
}


void
GenioApp::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_OBSERVER_NOTICE_CHANGE: {
			int32 code;
			message->FindInt32(B_OBSERVE_WHAT_CHANGE, &code);
			if (code == gCFG.UpdateMessageWhat()) {
				// TODO: Long list of strcmp
				const char* key = NULL;
				message->FindString("key", &key);
				if (key != NULL) {
					if (::strcmp(key, "log_destination") == 0)
						Logger::SetDestination(gCFG["log_destination"]);
					else if (::strcmp(key, "log_level") == 0)
						Logger::SetLevel(log_level(int32(gCFG["log_level"])));
				}
				gCFG.SaveToFile({fConfigurationPath});
				LogInfo("Configuration file saved! (updating %s)", message->GetString("key", "ERROR!"));
			}
			break;
		}
		case B_SILENT_RELAUNCH:
			if (fGenioWindow)
				fGenioWindow->Activate(true);
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
	// let's subscribe config changes updates
	StartWatching(this, MSG_NOTIFY_CONFIGURATION_UPDATED);

	fGenioWindow->Show();
}


BStringList
GenioApp::_SplitChangeLog(const char* changeLog)
{
	BStringList list;
	char* stringStart = const_cast<char*>(changeLog);
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

int
GenioApp::_HandleArgs(int argc, char **argv)
{
	int optIndex = 0;
	int c = 0;
	while ((c = ::getopt_long(argc, argv, "l:",
			sLongOptions, &optIndex)) != -1) {
		switch (c) {
			case 'l':
				Logger::SetLevelByChar(optarg[0]);
				break;
			case 0:
			{
				BString optName = sLongOptions[optIndex].name;
				if (optName == "loglevel")
					Logger::SetLevelByChar(::optarg[0]);
				break;
			}
			default:
				break;
		}
	}

	return ::optind;
}


// These settings will show up in the ConfigWindow.
// Use this context to avoid invalidating previous translations
// TODO: Remove after v2 release ?
#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SettingsWindow"


void
GenioApp::_PrepareConfig(ConfigManager& cfg)
{
	BString general(B_TRANSLATE("General"));
	cfg.AddConfig(general.String(), "projects_directory",
		B_TRANSLATE("Projects folder:"), "/boot/home/workspace");
	cfg.AddConfig(general.String(), "auto_expand_collapse_projects",
		B_TRANSLATE("Auto collapse/expand projects"), false);
	cfg.AddConfig(general.String(), "fullpath_title",
		B_TRANSLATE("Show full path in window title"), true);
	GMessage loggers = {
		{"mode", "options"},
		{"option_1", {
			{"value", (int32)Logger::LOGGER_DEST_STDOUT },
			{"label", "stdout" }}},
		{"option_2", {
			{"value", (int32)Logger::LOGGER_DEST_STDERR },
			{"label", "stderr"}}},
		{"option_3", {
			{"value", (int32)Logger::LOGGER_DEST_SYSLOG },
			{"label", "syslog"}}},
		{"option_4", {
			{"value", (int32)Logger::LOGGER_DEST_BEDC },
			{"label", "BeDC"}}}
	};
	cfg.AddConfig(general.String(), "log_destination",
		B_TRANSLATE("Log destination:"), (int32)Logger::LOGGER_DEST_STDOUT, &loggers);

	GMessage levels = {
		{"mode", "options"},
		{"option_1", {
			{"value", (int32)LOG_LEVEL_OFF },
			{"label", "Off" }}},
		{"option_2", {
			{"value", (int32)LOG_LEVEL_ERROR },
			{"label", "Error"}}},
		{"option_3", {
			{"value", (int32)LOG_LEVEL_INFO },
			{"label", "Info"}}},
		{"option_4", {
			{"value", (int32)LOG_LEVEL_DEBUG },
			{"label", "Debug"}}},
		{"option_5", {
			{"value", (int32)LOG_LEVEL_TRACE },
			{"label", "Trace"}}}
	};

	cfg.AddConfig(general.String(), "log_level",
		B_TRANSLATE("Log level:"), (int32)LOG_LEVEL_ERROR, &levels);

	BString generalStartup = general;
	generalStartup.Append("/").Append(B_TRANSLATE("Startup"));
	cfg.AddConfig(generalStartup.String(), "reopen_projects", B_TRANSLATE("Reload projects"), true);
	cfg.AddConfig(generalStartup.String(), "reopen_files", B_TRANSLATE("Reload files"), true);
	cfg.AddConfig(generalStartup.String(), "show_projects", B_TRANSLATE("Show projects pane"), true);
	cfg.AddConfig(generalStartup.String(), "show_outline", B_TRANSLATE("Show outline pane"), true);
	cfg.AddConfig(generalStartup.String(), "show_output", B_TRANSLATE("Show output pane"), true);
	cfg.AddConfig(generalStartup.String(), "show_toolbar", B_TRANSLATE("Show toolbar"), true);

	GMessage sizes;
	sizes = { {"mode","options"},
			  {"option_1", { {"value", -1}, {"label", B_TRANSLATE("Default size") } } }
			};
	int32 c = 2;
	for (int32 i = 10; i <= 18; i++) {
		BString key("option_");
		key << c;
		BString text;
		text << i;
		sizes[key.String()] = { {"value", i}, {"label", text.String() } };
		c++;
	}

	GMessage fontCfg = { {"mode","options"},
			  {"option_1", { {"value", ""}, {"label", B_TRANSLATE("Default font") } } }
	};
	c = 2;
	int32 numFamilies = count_font_families();
	for(int32 i = 0; i < numFamilies; i++) {
		font_family family;
		if(get_font_family(i, &family) == B_OK) {
			BString key("option_");
			key << c;
			fontCfg[key.String()] = { {"value", family}, {"label", family } };
			c++;
		}
	}
	BString editor(B_TRANSLATE("Editor"));
	cfg.AddConfig(editor.String(), "edit_fontfamily", B_TRANSLATE("Font:"), "", &fontCfg);
	cfg.AddConfig(editor.String(), "edit_fontsize", B_TRANSLATE("Font size:"), -1, &sizes);
	cfg.AddConfig(editor.String(), "syntax_highlight", B_TRANSLATE("Enable syntax highlighting"), true);
	cfg.AddConfig(editor.String(), "brace_match", B_TRANSLATE("Enable brace matching"), true);
	cfg.AddConfig(editor.String(), "save_caret", B_TRANSLATE("Save caret position"), true);
	cfg.AddConfig(editor.String(), "ignore_editorconfig", B_TRANSLATE("Ignore .editorconfig"), false);

	cfg.AddConfigSeparator(editor.String(), "banner_ignore_editorconfig", B_TRANSLATE_COMMENT("\nThese are only applied if no .editorconfig is used:",
		"The translation shouldn't be much longer than the English original"));

	cfg.AddConfig(editor.String(), "trim_trailing_whitespace", B_TRANSLATE("Trim trailing whitespace on save"), false);
	// TODO: change to "indent_style" to be coherent with editorconfig
	cfg.AddConfig(editor.String(), "tab_to_space", B_TRANSLATE("Convert tabs to spaces"), false);

	GMessage tabs = { {"min",1},{"max",8} };
	// TODO: change to "indent_size" to be coherent with editorconfig
	cfg.AddConfig(editor.String(), "tab_width", B_TRANSLATE("Tab width:"), 4, &tabs);

	GMessage zooms = { {"min", -9}, {"max", 19} };
	cfg.AddConfig(editor.String(), "editor_zoom", B_TRANSLATE("Editor zoom:"), 0, &zooms);

	GMessage styles = { {"mode", "options"} };
	std::set<std::string> allStyles;
	Styler::GetAvailableStyles(allStyles);
	int32 style_index = 1;
	for (auto style : allStyles) {
		BString opt("option_");
		opt << style_index;

		styles[opt.String()] = { {"value", style_index - 1}, {"label", style.c_str() } };
		style_index++;
	}

	BString editorVisual = editor;
	editorVisual.Append("/").Append(B_TRANSLATE("Visuals"));
	cfg.AddConfig(editorVisual.String(), "editor_style", B_TRANSLATE("Editor style:"), "default", &styles);
	cfg.AddConfig(editorVisual.String(), "show_linenumber", B_TRANSLATE("Show line numbers"), true);
	cfg.AddConfig(editorVisual.String(), "show_commentmargin", B_TRANSLATE("Show comment margin"), true);
	cfg.AddConfig(editorVisual.String(), "enable_folding", B_TRANSLATE("Show folding margin"), true);
	cfg.AddConfig(editorVisual.String(), "mark_caretline", B_TRANSLATE("Mark caret line"), true);
	cfg.AddConfig(editorVisual.String(), "show_white_space", B_TRANSLATE("Show whitespace"), false);
	cfg.AddConfig(editorVisual.String(), "show_line_endings", B_TRANSLATE("Show line endings"), false);
	cfg.AddConfig(editorVisual.String(), "wrap_lines", B_TRANSLATE("Wrap lines"), true);
	cfg.AddConfig(editorVisual.String(), "show_ruler", B_TRANSLATE("Show vertical ruler"), true);
	GMessage limits = { {"min", 0}, {"max", 500} };
	cfg.AddConfig(editorVisual.String(), "ruler_column", B_TRANSLATE("Set ruler to column:"), 100, &limits);

	BString build(B_TRANSLATE("Build"));
	cfg.AddConfig(build.String(), "wrap_console", B_TRANSLATE("Wrap lines in console"), false);
	cfg.AddConfig(build.String(), "console_banner", B_TRANSLATE_COMMENT("Console banner",
		"A separating line inserted at the start and end of a command output in the console. Short as possible."), true);
	cfg.AddConfig(build.String(), "build_on_save", B_TRANSLATE("Auto-Build on resource save"), false);
	cfg.AddConfig(build.String(), "save_on_build", B_TRANSLATE("Auto-Save changed files when building"), false);

	BString editorFind = editor;
	editorFind.Append("/").Append(B_TRANSLATE("Find"));
	cfg.AddConfig(editorFind.String(), "find_wrap", B_TRANSLATE_COMMENT("Wrap around",
		"Continue searching from the beginning when reaching the end of the file"), false);
	cfg.AddConfig(editorFind.String(), "find_whole_word", B_TRANSLATE_COMMENT("Whole word", "Short as possible."), false);
	cfg.AddConfig(editorFind.String(), "find_match_case", B_TRANSLATE_COMMENT("Match case", "Short as possible."), false);
	cfg.AddConfig(editorFind.String(), "find_exclude_directory", B_TRANSLATE("Exclude folders:"), ".*,objects.*");

	GMessage lsplevels = { {"mode", "options"},
						   {"note", B_TRANSLATE("This setting will be updated on restart.")},
						   {"option_1", {
								{"value", (int32)lsp_log_level::LSP_LOG_LEVEL_ERROR },
								{"label", "Error" }}},
						   {"option_2", {
								{"value", (int32)lsp_log_level::LSP_LOG_LEVEL_INFO },
								{"label", "Info" }}},
						   {"option_3", {
								{"value", (int32)lsp_log_level::LSP_LOG_LEVEL_TRACE },
								{"label", "Trace" }}},
						 };

	// TODO: Not sure about translating "LSP"
	cfg.AddConfig("LSP", "lsp_clangd_log_level", B_TRANSLATE("Log level:"),
		(int32)lsp_log_level::LSP_LOG_LEVEL_ERROR, &lsplevels);

	BString sourceControl(B_TRANSLATE("Source control"));
	cfg.AddConfig(sourceControl.String(), "repository_outline",
		B_TRANSLATE("Show repository outline"), true);

	cfg.AddConfig("Hidden", "ui_bounds", "ui_bounds", BRect(40, 40, 839, 639));
	cfg.AddConfig("Hidden", "config_version", "config_version", "2.0");
	cfg.AddConfig("Hidden", "run_without_buffering", "run_without_buffering", true);
	GMessage log_limits = { {"min", 1024}, {"max", 4096} };
	cfg.AddConfig("Hidden", "log_size", B_TRANSLATE("Log size:"), 1024, &log_limits);
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
