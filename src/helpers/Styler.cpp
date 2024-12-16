/*
 * Copyright 2023, Andrea Anzani 
 * Copyright 2014-2018 Kacper Kasper  (from Koder editor)
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "Styler.h"

#include <vector>

#include <yaml-cpp/yaml.h>

#include <Alert.h>
#include <Catalog.h>
#include <Directory.h>
#include <FindDirectory.h>
#include <Font.h>
#include <Path.h>
#include <String.h>

#include "Editor.h"
#include "ScintillaView.h"
#include "Utils.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Styler"


namespace YAML {

namespace {

int
CSSToInt(const std::string cssColor)
{
	if(cssColor[0] != '#' || cssColor.length() != 7)
		return -1;

	std::string red = cssColor.substr(1, 2);
	std::string green = cssColor.substr(3, 2);
	std::string blue = cssColor.substr(5, 2);

	return std::stoi(blue + green + red, nullptr, 16);
}

}

template<>
struct convert<Styler::Style> {
	static Node encode(const Styler::Style& rhs) {
		return Node();
	}

	static bool decode(const Node& node, Styler::Style& rhs) {
		if (!node.IsMap()) {
			return false;
		}

		if (node["foreground"]) {
			rhs.fgColor = CSSToInt(node["foreground"].as<std::string>());
		}
		if (node["background"]) {
			rhs.bgColor = CSSToInt(node["background"].as<std::string>());
		}
		if (node["style"] && node["style"].IsSequence()) {
			rhs.style = 0;
			const auto styles = node["style"].as<std::vector<std::string>>();
			for (const auto& style : styles) {
				if (style == "bold")
					rhs.style |= 1;
				else if (style == "italic")
					rhs.style |= 2;
				else if (style == "underline")
					rhs.style |= 4;
			}
		}
		return true;
	}
};

}


std::unordered_map<int, Styler::Style>	Styler::sStylesMapping;


/* static */ void
Styler::ApplyGlobal(BScintillaView* editor, const char* style, const BFont* font)
{
	sStylesMapping.clear();
	try {
		_ApplyGlobal(editor, style, GetDataDirectory(), font);
	} catch (const YAML::BadFile &) {
	}
	try {
		_ApplyGlobal(editor, style, GetUserSettingsDirectory(), font);
	} catch (const YAML::BadFile &) {
	}
}


/* static */ void
Styler::_ApplyGlobal(BScintillaView* editor, const char* style, const BPath &path, const BFont* font)
{
	BString fullpath = _FullStylePath(style, path);
	const YAML::Node styles = YAML::LoadFile(fullpath.String());
	YAML::Node global;
	if (styles["Global"]) {
		global = styles["Global"];
	}

	_ApplyDefaultStyle(editor, global, font);
	_ApplyBasicStyle(editor, global);

	int id;
	Style s;
	for(const auto &node : global) {
		std::string name = node.first.as<std::string>();
		_GetAttributesFromNode(node.second, id, s);
		if (id != -1) {
			_ApplyAttributes(editor, id, s);
			sStylesMapping.emplace(id, s);
		} else {
			if (name == "Fold") {
				if (s.fgColor != -1) {
					editor->SendMessage(SCI_SETFOLDMARGINHICOLOUR, true, s.fgColor);
				}
				if (s.bgColor != -1) {
					editor->SendMessage(SCI_SETFOLDMARGINCOLOUR, true, s.bgColor);
				}
			} else if (name == "Fold marker") {
				std::array<int32, 7> markers = {
					SC_MARKNUM_FOLDER,
					SC_MARKNUM_FOLDEROPEN,
					SC_MARKNUM_FOLDERSUB,
					SC_MARKNUM_FOLDERTAIL,
					SC_MARKNUM_FOLDEREND,
					SC_MARKNUM_FOLDEROPENMID,
					SC_MARKNUM_FOLDERMIDTAIL
				};
				for (auto marker : markers) {
					if (s.fgColor != -1) {
						editor->SendMessage(SCI_MARKERSETFORE, marker, s.fgColor);
					}
					if (s.bgColor != -1) {
						editor->SendMessage(SCI_MARKERSETBACK, marker, s.bgColor);
					}
				}
			} else if (name == "Bookmark marker") {
				if (s.fgColor != -1) {
					editor->SendMessage(SCI_MARKERSETFORE, sci_BOOKMARK, s.fgColor);
				}
				if (s.bgColor != -1) {
					editor->SendMessage(SCI_MARKERSETBACK, sci_BOOKMARK, s.bgColor);
				}
			}
		}
	}
	for (const auto& style : styles) {
		if (style.first.as<std::string>() == "Global")
			continue;
		_GetAttributesFromNode(style.second, id, s);
		sStylesMapping.emplace(id, s);
	}
}


/* static */ void
Styler::ApplySystemStyle(BScintillaView* editor)
{
	Style s(rgb_colorToSciColor(ui_color(B_DOCUMENT_TEXT_COLOR)),
			rgb_colorToSciColor(ui_color(B_DOCUMENT_BACKGROUND_COLOR)), 0);

	const BFont* font = be_fixed_font;
	font_family fontName;
	font->GetFamilyAndStyle(&fontName, nullptr);
	editor->SendMessage(SCI_STYLESETFONT, 32, (sptr_t) fontName);
	editor->SendMessage(SCI_STYLESETSIZE, 32, (sptr_t) font->Size());

	editor->SendMessage(SCI_STYLESETFORE, 32, s.fgColor);
	editor->SendMessage(SCI_STYLESETBACK, 32, s.bgColor);

	editor->SendMessage(SCI_STYLECLEARALL, 0, 0);

	// "Current line"
	editor->SendMessage(SCI_SETCARETLINEBACK, s.bgColor, 0); //same has background!
    // "Whitespace"
	editor->SendMessage(SCI_SETWHITESPACEFORE, true, s.fgColor);
	editor->SendMessage(SCI_SETWHITESPACEBACK, true, s.bgColor);

	//"Selected text"
	editor->SendMessage(SCI_SETSELFORE, true, s.bgColor);
	editor->SendMessage(SCI_SETSELBACK, true, s.fgColor);

	//"Caret"
	editor->SendMessage(SCI_SETCARETFORE, s.fgColor, 0);

	//"Edge"
	editor->SendMessage(SCI_SETEDGECOLOUR, s.fgColor, 0);
}


/* static */ void
Styler::ApplyBasicStyle(BScintillaView* editor, const char* style, const BFont* font)
{
	try {
		_ApplyBasicStyle(editor, style, GetDataDirectory(), font);
	} catch (const YAML::BadFile &) {
	}
	try {
		_ApplyBasicStyle(editor, style, GetUserSettingsDirectory(), font);
	} catch (const YAML::BadFile &) {
	}
}


/* static */
void
Styler::_ApplyBasicStyle(BScintillaView* editor, const char* style, const BPath &path, const BFont* font)
{
	BString fullpath = _FullStylePath(style, path);
	const YAML::Node styles = YAML::LoadFile(fullpath.String());
	YAML::Node global;
	if (styles["Global"]) {
		global = styles["Global"];
	}

	_ApplyDefaultStyle(editor, global, font);
	_ApplyBasicStyle(editor, global);

}


void
Styler::_ApplyBasicStyle(BScintillaView* editor, YAML::Node& global)
{
	int id;
	Style s;
	for (const auto &node : global) {
		std::string name = node.first.as<std::string>();
		_GetAttributesFromNode(node.second, id, s);

		if (name == "Current line") {
			editor->SendMessage(SCI_SETCARETLINEBACK, s.bgColor, 0);
			//editor->SendMessage(SCI_SETCARETLINEBACKALPHA, 128, 0);
		} else if (name == "Whitespace") {
			if (s.fgColor != -1) {
				editor->SendMessage(SCI_SETWHITESPACEFORE, true, s.fgColor);
			}
			if (s.bgColor != -1) {
				editor->SendMessage(SCI_SETWHITESPACEBACK, true, s.bgColor);
			}
		} else if (name == "Selected text") {
			if (s.fgColor != -1) {
				editor->SendMessage(SCI_SETSELFORE, true, s.fgColor);
			}
			if (s.bgColor != -1) {
				editor->SendMessage(SCI_SETSELBACK, true, s.bgColor);
			}
		} else if (name == "Caret") {
			editor->SendMessage(SCI_SETCARETFORE, s.fgColor, 0);
		} else if (name == "Edge") {
			editor->SendMessage(SCI_SETEDGECOLOUR, s.fgColor, 0);
		}
	}
}


void
Styler::_ApplyDefaultStyle(BScintillaView* editor, YAML::Node& global,  const BFont* font)
{
	if (!global["Default"])
		return;

	int id;
	Style s;
	_GetAttributesFromNode(global["Default"], id, s);
	if (font == nullptr)
		font = be_fixed_font;
	font_family fontName;
	font->GetFamilyAndStyle(&fontName, nullptr);
	editor->SendMessage(SCI_STYLESETFONT, 32, (sptr_t) fontName);
	editor->SendMessage(SCI_STYLESETSIZE, 32, (sptr_t) font->Size());
	_ApplyAttributes(editor, 32, s);
	editor->SendMessage(SCI_STYLECLEARALL, 0, 0);
	editor->SendMessage(SCI_STYLESETFONT, 36, (sptr_t) fontName);
	editor->SendMessage(SCI_STYLESETSIZE, 36, (sptr_t) (font->Size() / 1.3));
	editor->SendMessage(SCI_SETWHITESPACESIZE, font->Size() / 6, 0);

	// whitespace
	editor->SendMessage(SCI_INDICSETSTYLE, 0, INDIC_ROUNDBOX);
	editor->SendMessage(SCI_INDICSETFORE, 0, 0x0000FF);
	editor->SendMessage(SCI_INDICSETALPHA, 0, 100);
	// IME
	editor->SendMessage(SCI_INDICSETSTYLE, INDIC_IME, INDIC_FULLBOX);
	editor->SendMessage(SCI_INDICSETFORE, INDIC_IME, 0xFF0000);
	editor->SendMessage(SCI_INDICSETSTYLE, INDIC_IME+1, INDIC_FULLBOX);
	editor->SendMessage(SCI_INDICSETFORE, INDIC_IME+1, 0x0000FF);
}


/* static */
void
Styler::ApplyLanguage(BScintillaView* editor, const std::map<int, int>& styleMapping)
{
	for (const auto& mapping : styleMapping) {
		int scintillaId = mapping.first;
		int styleId = mapping.second;
		const auto it = sStylesMapping.find(styleId);
		if (it != sStylesMapping.end()) {
			Style s = it->second;
			_ApplyAttributes(editor, scintillaId, s);
		}
	}
}


/* static */
void
Styler::GetAvailableStyles(std::set<std::string> &styles)
{
	_GetAvailableStyles(styles, GetDataDirectory());
	_GetAvailableStyles(styles, GetUserSettingsDirectory());
}


/* static */
void
Styler::_GetAvailableStyles(std::set<std::string> &styles, const BPath &path)
{
	BPath p(path);
	p.Append("styles");
	BDirectory directory(p.Path());
	BEntry entry;
	char name[B_FILE_NAME_LENGTH];
	while (directory.GetNextEntry(&entry) == B_OK) {
		if (entry.IsDirectory())
			continue;
		entry.GetName(name);
		if (GetFileExtension(name) == "yaml") {
			styles.insert(GetFileName(name));
		}
	}
}


void
Styler::_GetAttributesFromNode(const YAML::Node &node, int& styleId, Styler::Style& style)
{
	styleId = node["id"] ? node["id"].as<int>() : -1;
	style = node.as<Style>();
}


void
Styler::_ApplyAttributes(BScintillaView* editor, int styleId, Styler::Style style)
{
	if (styleId < 0) {
		// FIXME: What happened here?
		return;
	}
	if (style.fgColor != -1) {
		editor->SendMessage(SCI_STYLESETFORE, styleId, style.fgColor);
	}
	if (style.bgColor != -1) {
		editor->SendMessage(SCI_STYLESETBACK, styleId, style.bgColor);
	}
	if (style.style != -1) {
		if (style.style & 1) {
			editor->SendMessage(SCI_STYLESETBOLD, styleId, true);
		}
		if (style.style & 2) {
			editor->SendMessage(SCI_STYLESETITALIC, styleId, true);
		}
		if (style.style & 4) {
			editor->SendMessage(SCI_STYLESETUNDERLINE, styleId, true);
		}
	}
}


BString
Styler::_FullStylePath(const char* style, const BPath &path)
{
	BPath p(path);
	p.Append("styles");
	p.Append(style);
	BString fileName(p.Path());
	fileName.Append(".yaml");
	if (!BEntry(fileName.String()).Exists()) {
		// TODO: Workaround for a bug in Haiku x86_32: exceptions
		// thrown inside yaml_cpp aren't catchable. We throw this exception
		// inside Genio and that works.
		// Should be fixed in beta5, though
		throw YAML::BadFile(fileName.String());
	}
	return BString(p.Path()).Append(".yaml");
}
