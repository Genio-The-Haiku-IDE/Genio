/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * Copyright 2014-2018 Kacper Kasper <kacperkasper@gmail.com> (from Koder editor)
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef STYLER_H
#define STYLER_H


#include <map>
#include <string>
#include <set>
#include <unordered_map>

#include <yaml-cpp/yaml.h>
#include <String.h>

class BFont;
class BPath;
class BScintillaView;


class Styler {
public:
	struct Style {
		Style() : fgColor(-1), bgColor(-1), style(-1) {}
		Style(int fg, int bg, int s) : fgColor(fg), bgColor(bg), style(s) {}
		int fgColor;
		int bgColor;
		int style;
	};
	static	void	ApplyGlobal(BScintillaView* editor, const char* style, const BFont* font = nullptr);
	static	void	ApplyLanguage(BScintillaView* editor, const std::map<int, int>& styleMapping);

	static	void	GetAvailableStyles(std::set<std::string> &styles);


	static	void	ApplyBasicStyle(BScintillaView* editor, const char* style, const BFont* font = nullptr);
	static	void	ApplySystemStyle(BScintillaView* editor);

private:
	static	void	_ApplyGlobal(BScintillaView* editor, const char* style, const BPath &path, const BFont* font = nullptr);
	static	void	_GetAvailableStyles(std::set<std::string> &styles, const BPath &path);
	static	void	_GetAttributesFromNode(const YAML::Node &node, int& styleId, Style& style);
	static	void	_ApplyAttributes(BScintillaView* editor, int styleId, Style style);
	static	void	_ApplyBasicStyle(BScintillaView* editor, const char* style, const BPath &path, const BFont* font = nullptr);
	static  void	_ApplyDefaultStyle(BScintillaView* editor, YAML::Node& node, const BFont* font);
	static	void	_ApplyBasicStyle(BScintillaView* editor, YAML::Node& global);
	static  BString _FullStylePath(const char* style, const BPath &path);

	static	std::unordered_map<int, Style>	sStylesMapping;
};


#endif // STYLER_H
