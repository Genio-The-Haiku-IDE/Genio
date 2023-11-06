/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * Copyright 2014-2018 Kacper Kasper <kacperkasper@gmail.com> (from Koder editor)
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef LANGUAGES_H
#define LANGUAGES_H


#include <map>
#include <string>
#include <vector>


class BPath;
class Editor;


class Languages {
public:
	static	size_t								GetCount() { return sLanguages.size(); }
	static	std::string							GetLanguage(int index) { return sLanguages[index]; }
	static	std::string							GetMenuItemName(std::string lang) { return sMenuItems[lang]; }
	static	bool								GetLanguageForExtension(const std::string ext, std::string& lang);
	static	void								SortAlphabetically();
	static	std::map<int, int>					ApplyLanguage(Editor* editor, const char* lang);
	static	void								LoadLanguages();

private:
	static	void								_LoadLanguages(const BPath& path);
	static	std::map<int, int>					_ApplyLanguage(Editor* editor, const char* lang, const BPath &path);
	static	std::vector<std::string>			sLanguages;
	static	std::map<std::string, std::string>	sMenuItems;
	static	std::map<std::string, std::string> 	sExtensions;
};


#endif // LANGUAGES_H
