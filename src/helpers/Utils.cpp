/*
 * Copyright 2023 Nexus6 <nexus6.haiku@icloud.com>
 * Copyright 2017-2019 Kacper Kasper <kacperkasper@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "Utils.h"

#include <Alert.h>
#include <Application.h>
#include <Bitmap.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <IconUtils.h>
#include <MessageFilter.h>
#include <Notification.h>
#include <RadioButton.h>
#include <Resources.h>
#include <Path.h>
#include <string>
#include <algorithm>
#include <Roster.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Utilities"


std::string
GetFileName(const std::string filename)
{
	size_t pos = filename.rfind('.');
	// pos != 0 is for dotfiles
	if(pos != std::string::npos && pos != 0)
		return filename.substr(0, pos);
	return filename;
}


std::string
GetFileExtension(const std::string filename)
{
	size_t pos = filename.rfind('.');
	// pos != 0 is for dotfiles
	if(pos != std::string::npos && pos != 0)
		return filename.substr(pos + 1);
	return "";
}


void
GetVectorIcon(const std::string icon, BBitmap* bitmap)
{
	if(bitmap == nullptr)
		return;

	BResources* resources = BApplication::AppResources();
	size_t size;
	const uint8* rawIcon;
	rawIcon = (const uint8*) resources->LoadResource(B_VECTOR_ICON_TYPE, icon.c_str(), &size);
	if(rawIcon == nullptr)
		return;

	BIconUtils::GetVectorIcon(rawIcon, size, bitmap);
}


/**
 * Splits command line argument in format a/b/file:10:92 into filename, line
 * and column. If column or line are missing -1 is returned in their place.
 */
std::string
ParseFileArgument(const std::string argument, int32* line, int32* column)
{
	auto is_all_digits = [](const std::string &str) {
		return str.find_first_not_of("-0123456789") == std::string::npos;
	};
	bool wrongFormat = false;
	std::string filename = argument;
	if(line != nullptr)
		*line = -1;
	if(column != nullptr)
		*column = -1;
	// first :
	int32 first = argument.find(':');
	if(first != (int32)std::string::npos) {
		filename = argument.substr(0, first);
		// second :
		int32 second = argument.find(':', first + 1);
		if(line != nullptr) {
			const int32 length = second != (int32)std::string::npos ?
				second - (first + 1) : second;
			const std::string line_str = argument.substr(first + 1, length);
			if(!line_str.empty()) {
				if(!is_all_digits(line_str) || line_str == "-") {
					wrongFormat = true;
				} else {
					*line = std::stoi(line_str);
				}
			}
		}
		if(column != nullptr && second != (int32)std::string::npos && !wrongFormat) {
			const std::string column_str = argument.substr(second + 1);
			if(!column_str.empty()) {
				if(!is_all_digits(column_str) || column_str == "-") {
					wrongFormat = true;
				} else {
					*column = std::stoi(column_str);
				}
			}
		}
	}

	if(wrongFormat) {
		return argument;
	}
	return filename;
}


template<typename T>
bool IsChecked(T* control)
{
	return control->Value() == B_CONTROL_ON;
}
template bool IsChecked<BCheckBox>(BCheckBox*);
template bool IsChecked<BRadioButton>(BRadioButton*);


template<typename T>
void SetChecked(T* control, bool checked)
{
	control->SetValue(checked ? B_CONTROL_ON : B_CONTROL_OFF);
}
template void SetChecked<BCheckBox>(BCheckBox*, bool);
template void SetChecked<BRadioButton>(BRadioButton*, bool);


void
OKAlert(const char* title, const char* message, alert_type type)
{
	BAlert* alert = new BAlert(title, message, B_TRANSLATE("OK"),
		nullptr, nullptr, B_WIDTH_AS_USUAL, type);
	alert->SetShortcut(0, B_ESCAPE);
	alert->Go();
}


int32 rgb_colorToSciColor(rgb_color color)
{
	return color.red | (color.green << 8) | (color.blue << 16);
}


KeyDownMessageFilter::KeyDownMessageFilter(uint32 commandToSend, char key,
	uint32 modifiers, filter_result filterResult)
	:
	BMessageFilter(B_KEY_DOWN),
	fKey(key),
	fModifiers(modifiers),
	fCommandToSend(commandToSend),
	fFilterResult(filterResult)
{
}

uint32
KeyDownMessageFilter::AllowedModifiers()
{
	return B_COMMAND_KEY | B_OPTION_KEY | B_SHIFT_KEY | B_CONTROL_KEY
		| B_MENU_KEY;
}

filter_result
KeyDownMessageFilter::Filter(BMessage* message, BHandler** target)
{

	if(message->what == B_KEY_DOWN) {
		const char* bytes;
		uint32 modifiers;
		message->FindString("bytes", &bytes);
		modifiers = static_cast<uint32>(message->GetInt32("modifiers", 0));
		modifiers = modifiers & AllowedModifiers();
		if(bytes[0] == fKey && modifiers == fModifiers) {
			Looper()->PostMessage(fCommandToSend);
			return fFilterResult;
		}
	}
	return B_DISPATCH_MESSAGE;
}


template<>
entry_ref
find_value<B_REF_TYPE>(BMessage* message, std::string name, int index) {
	entry_ref ref;
	status_t status = message->FindRef(name.c_str(), index, &ref);
	if(status == B_OK) {
		return ref;
	}
	return entry_ref();
}


void ProgressNotification(const char* group, const char* title, const char* messageID,
							const char* content, float progress, bigtime_t timeout)
{
	BNotification notification(B_PROGRESS_NOTIFICATION);
	notification.SetGroup(group);
	notification.SetTitle(title);
	notification.SetMessageID(messageID);
	notification.SetContent(content);
	notification.SetProgress(progress);
	notification.Send(timeout);
}

void ErrorNotification(const char* group, const char* title, const char* messageID,
							const char* content, bigtime_t timeout)
{
	BNotification notification(B_ERROR_NOTIFICATION);
	notification.SetGroup(group);
	notification.SetTitle(title);
	notification.SetMessageID(messageID);
	notification.SetContent(content);
	notification.Send(timeout);
}

#define FIND_IN_ARRAY(ARRAY, VALUE) (std::find(std::begin(ARRAY), std::end(ARRAY), VALUE) != std::end(ARRAY));

std::string sourceExt[] = {".cpp", ".c", ".cc", ".cxx", ".c++"}; //, ".m", ".mm"};
std::string headerExt[] = {".h", ".hh", ".hpp", ".hxx"}; //, ".inc"};

bool
IsCppSourceExtension(std::string extension)
{
	return FIND_IN_ARRAY(sourceExt, extension);
}

bool
IsCppHeaderExtension(std::string extension)
{
	return FIND_IN_ARRAY(headerExt, extension);
}

status_t
FindSourceOrHeader(const entry_ref* editorRef, entry_ref* foundRef)
{
	//TODO this is not language specific!

	status_t status;
	BEntry entry;
	if ((status = entry.SetTo(editorRef)) != B_OK)
		return status;

	BPath fullPath;
	if ((status = entry.GetPath(&fullPath)) != B_OK)
		return status;

	// extract extension
	std::string filename = fullPath.Path();
	size_t dotPos = filename.find_last_of('.');
	if (dotPos == std::string::npos)
		return B_ERROR;

	std::string extension = filename.substr(dotPos);
	std::string prefixname = filename.substr(0, dotPos);

	BEntry foundFile;
	bool found = false;

	if (IsCppSourceExtension(extension)) {
		// search if the file exists with the possible header extensions..
		found = std::find_if(std::begin(headerExt), std::end(headerExt),
			[&prefixname, &foundFile](std::string extension) {
				std::string fullFilename = prefixname + extension;
				foundFile.SetTo(fullFilename.c_str());
				return foundFile.Exists();
			});
	} else if (IsCppHeaderExtension(extension)) {
		// search if the file exists with the possible source extensions..
		found = std::find_if(std::begin(sourceExt), std::end(sourceExt),
			[&prefixname, &foundFile](std::string extension) {
				std::string fullFilename = prefixname + extension;
				foundFile.SetTo(fullFilename.c_str());
				return foundFile.Exists();
			});
	}

	if (!found)
		return B_ERROR;

	return foundFile.GetRef(foundRef);
}

BPath
GetDataDirectory()
{
	// Default template directory
	app_info info;
	BPath dataPath;
	if (be_app->GetAppInfo(&info) == B_OK) {
		// This code should work both for the case where Genio is
		// in the "app" subdirectory, like in the repo,
		// and when it's in the package.
		BPath genioPath(&info.ref);
		BPath parentPath;
		if (genioPath.GetParent(&parentPath) == B_OK) {
			dataPath = parentPath;
			dataPath.Append("data");
			// Genio
			// data/templates/
			if (!BEntry(dataPath.Path()).IsDirectory()) {
				// app/Genio
				// data/templates/
				parentPath.GetParent(&dataPath);
				dataPath.Append("data");
			}
		}
	}
	return dataPath;
}