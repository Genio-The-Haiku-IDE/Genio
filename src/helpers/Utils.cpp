/*
 * Copyright 2023 Nexus6 <nexus6.haiku@icloud.com>
 * Copyright 2017-2019 Kacper Kasper <kacperkasper@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "Utils.h"

#include <Alert.h>
#include <AppFileInfo.h>
#include <Application.h>
#include <Bitmap.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <DateTime.h>
#include <FindDirectory.h>
#include <IconUtils.h>
#include <MessageFilter.h>
#include <Path.h>
#include <RadioButton.h>
#include <Resources.h>
#include <Roster.h>
#include <SystemCatalog.h>
#include <WindowScreen.h>

#include <algorithm>
#include <string>

#include "GenioApp.h"
#include "ScintillaView.h"


using BPrivate::gSystemCatalog;

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Utilities"


rgb_color
TextColorByLuminance(rgb_color background)
{
	double luminance = 0.299 * background.red + 0.587 * background.green + 0.114 * background.blue;
	return (luminance > 128.0) ? rgb_color({0, 0, 0}) : rgb_color({255,255,255});
}


void
FakeMouseMovement(BView* view)
{
	// This is mostly due to a limition of the app_server:
	// the wheel mouse message is always target to the view with the last mouse movement.
	// in case a view appear under the mouse without any movement (ALT+TAB on a .cpp to open the .h)
	// the wheel is moving the wrong scrollbars (no matter of the focus).
	// the workaround is to fake a mouse movement to update the view under the cursor when an
	// Editor is selected.
	//
	BPoint location;
	uint32 buttons = 0;
	view->GetMouse(&location, &buttons);
	view->ConvertToScreen(&location);
	set_mouse_position(location.x, location.y);
}


std::string
GetFileName(const std::string& filename)
{
	size_t pos = filename.rfind('.');
	// pos != 0 is for dotfiles
	if (pos != std::string::npos && pos != 0)
		return filename.substr(0, pos);
	return filename;
}


std::string
GetFileExtension(const std::string& filename)
{
	size_t pos = filename.rfind('.');
	// pos != 0 is for dotfiles
	if (pos != std::string::npos && pos != 0)
		return filename.substr(pos + 1);
	return "";
}


status_t
GetVectorIcon(const std::string& icon, BBitmap* bitmap)
{
	if (bitmap == nullptr)
		return B_ERROR;

	BResources* resources = BApplication::AppResources();
	size_t size;
	const uint8* rawIcon = reinterpret_cast<const uint8*>(
		resources->LoadResource(B_VECTOR_ICON_TYPE, icon.c_str(), &size));
	if (rawIcon == nullptr)
		return B_ERROR;

	return BIconUtils::GetVectorIcon(rawIcon, size, bitmap);
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
	if (message->what == B_KEY_DOWN) {
		const char* bytes;
		uint32 modifiers;
		message->FindString("bytes", &bytes);
		modifiers = static_cast<uint32>(message->GetInt32("modifiers", 0));
		modifiers = modifiers & AllowedModifiers();
		if (bytes[0] == fKey && modifiers == fModifiers) {
			Looper()->PostMessage(fCommandToSend);
			return fFilterResult;
		}
	}
	return B_DISPATCH_MESSAGE;
}


void
ProgressNotification(const BString& group, const BString&  title, const BString&  messageID,
							const BString& content, float progress, bigtime_t timeout)
{
	BNotification notification(B_PROGRESS_NOTIFICATION);
	notification.SetGroup(group);
	notification.SetTitle(title);
	notification.SetMessageID(messageID);
	notification.SetContent(content);
	notification.SetProgress(progress);
	notification.Send(timeout);
}


void
ShowNotification(const BString& group, const BString&  title, const BString&  messageID,
							const BString& content, notification_type type, bigtime_t timeout)
{
	BNotification notification(type);
	notification.SetGroup(group);
	notification.SetTitle(title);
	notification.SetMessageID(messageID);
	notification.SetContent(content);
	notification.Send(timeout);
}


template<type_code BType>
struct property_type
{
	using type = void*;
};


template<>
struct property_type<B_STRING_TYPE>
{
	using type = std::string;
};


template<>
struct property_type<B_REF_TYPE>
{
	using type = entry_ref;
};


// Used by the FIND_IN_ARRAY macro
template<type_code BType>
typename property_type<BType>::type
find_value(BMessage* message, const std::string& name, int32 index) {
	typename property_type<BType>::type typed_data;
	ssize_t size;
	const void* data;
	status_t status = message->FindData(name.c_str(), BType, index, &data, &size);
	if(status == B_OK) {
		memcpy(data, size, &typed_data);
	}
	return typename property_type<BType>::type();
}


template<>
entry_ref
find_value<B_REF_TYPE>(BMessage* message, const std::string& name, int32 index)
{
	entry_ref ref;
	status_t status = message->FindRef(name.c_str(), index, &ref);
	if(status == B_OK) {
		return ref;
	}
	return entry_ref();
}


template<type_code BType>
class message_property {
private:
	BMessage* message_;
	std::string prop_name_;

public:
	class iterator {
	private:
		int index_;
		BMessage* message_;
		std::string prop_name_;
	public:
		iterator(BMessage* message, const std::string& prop_name, int index = 0)
			: message_(message), prop_name_(prop_name), index_(index) {}
		bool operator==(iterator &rhs) {
			return index_ == rhs.index_ &&
				message_ == rhs.message_ &&
				prop_name_ == rhs.prop_name_;
		}
		bool operator!=(iterator &rhs) { return !(*this == rhs); }
		iterator &operator++() { ++index_; return *this; }
		iterator operator++(int) {
			iterator clone(*this);
			++index_;
			return clone;
		}
		typename property_type<BType>::type operator*() {
			return find_value<BType>(message_, prop_name_, index_);
		}
	};
	message_property(BMessage* message, const std::string& prop_name)
		: message_(message), prop_name_(prop_name) {}
	iterator begin() { return iterator(message_, prop_name_, 0); }
	iterator end() { return iterator(message_, prop_name_, size()); }
	size_t size() {
		int32 count;
		if (message_->GetInfo(prop_name_.c_str(), nullptr, &count) == B_OK) {
			return static_cast<size_t>(count);
		}
		return 0; // FIXME: throw an exception here
	}
};


#define FIND_IN_ARRAY(ARRAY, VALUE) (std::find(std::begin(ARRAY), std::end(ARRAY), VALUE) != std::end(ARRAY));


std::string sourceExt[] = {".cpp", ".c", ".cc", ".cxx", ".c++"}; //, ".m", ".mm"};
std::string headerExt[] = {".h", ".hh", ".hpp", ".hxx"}; //, ".inc"};


bool
IsCppSourceExtension(const std::string& extension)
{
	return FIND_IN_ARRAY(sourceExt, extension);
}


bool
IsCppHeaderExtension(const std::string& extension)
{
	return FIND_IN_ARRAY(headerExt, extension);
}


status_t
FindSourceOrHeader(const entry_ref* editorRef, entry_ref* foundRef)
{
	// TODO this is not language specific!

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
			[&prefixname, &foundFile](const std::string& extension) {
				std::string fullFilename = prefixname + extension;
				foundFile.SetTo(fullFilename.c_str());
				return foundFile.Exists();
			});
	} else if (IsCppHeaderExtension(extension)) {
		// search if the file exists with the possible source extensions..
		found = std::find_if(std::begin(sourceExt), std::end(sourceExt),
			[&prefixname, &foundFile](const std::string& extension) {
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
GetUserSettingsDirectory()
{
	BPath userPath;
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &userPath, true);
	if (status == B_OK)
		userPath.Append(GenioNames::kApplicationName);
	return userPath;
}


bool
GetGenioDirectory(BPath& destPath)
{
	// Default template directory
	app_info info;
	if (be_app->GetAppInfo(&info) == B_OK) {
		BPath genioPath(&info.ref);
		BPath parentPath;
		if (genioPath.GetParent(&destPath) == B_OK) {
			return true;
		}
	}
	return false;
}


BPath
GetDataDirectory()
{
	BPath genioPath;
	// /system/data/Genio when installed from package
	if (find_directory(B_SYSTEM_DATA_DIRECTORY, &genioPath, true) == B_OK) {
		genioPath.Append("Genio");
		if (BEntry(genioPath.Path(), true).IsDirectory())
			return genioPath;
	}

	// When running from repository folder
	// TODO: Would be nice to check this before the other one
	// but it's troubling when isntalled from packages,
	// because it ends up in /system/data/ and not in /system/data/Genio/
	return GetNearbyDataDirectory();
}


// Search for data directory nearby the Genio binary:
// or in ./data or in ../data/
BPath
GetNearbyDataDirectory()
{
	BPath genioPath;
	if (GetGenioDirectory(genioPath)) {
		genioPath.Append("data");
		// ./Genio
		// ./data/templates/
		if (!BEntry(genioPath.Path(), true).IsDirectory()) {
			// ./app/Genio
			// ./data/templates/
			genioPath.GetParent(&genioPath);
			genioPath.GetParent(&genioPath);
			genioPath.Append("data");
		}
	}
	return genioPath;
}


bool
IsXMasPeriod()
{
	//dates between 20-Dec and 10-Jan included.
	BDate today = BDate::CurrentDate(B_LOCAL_TIME);
	return ((today.Month() == 12 && today.Day() >= 20) ||
		    (today.Month() == 1  && today.Day() <= 10));
}


BString
ReadFileContent(const char* filename, off_t maxSize)
{
	// TODO: Very simple, does not return
	// an error if something goes wrong

	BString read;
	BPath destPath;
	if (!GetGenioDirectory(destPath))
		return read;
	destPath.Append(filename);
	BFile file;
	if ((file.SetTo(destPath.Path(), B_READ_ONLY)) != B_OK)
		return read;
	if ((file.InitCheck()) != B_OK)
		return read;

	off_t size;
	file.GetSize(&size);

	char* buffer = read.LockBuffer(size + 1);
	off_t len = file.Read(buffer, size);
	buffer[len] = '\0';
	read.UnlockBuffer();
	return read;
}


// mostly taken from BAboutWindow
BString
GetVersion()
{
	app_info info;
	if (be_app->GetAppInfo(&info) != B_OK)
		return NULL;

	BFile file(&info.ref, B_READ_ONLY);
	BAppFileInfo appMime(&file);
	if (appMime.InitCheck() != B_OK)
		return NULL;

	version_info versionInfo;
	if (appMime.GetVersionInfo(&versionInfo, B_APP_VERSION_KIND) == B_OK) {
		if (versionInfo.major == 0 && versionInfo.middle == 0
			&& versionInfo.minor == 0) {
			return NULL;
		}

		const char* version = B_TRANSLATE_MARK("Version");
		version = gSystemCatalog.GetString(version, "AboutWindow");
		BString appVersion(version);
		appVersion << " " << versionInfo.major << "." << versionInfo.middle;
		if (versionInfo.minor > 0)
			appVersion << "." << versionInfo.minor;

		// Add the version variety
		const char* variety = NULL;
		switch (versionInfo.variety) {
			case B_DEVELOPMENT_VERSION:
				variety = B_TRANSLATE_MARK("development");
				break;
			case B_ALPHA_VERSION:
				variety = B_TRANSLATE_MARK("alpha");
				break;
			case B_BETA_VERSION:
				variety = B_TRANSLATE_MARK("beta");
				break;
			case B_GAMMA_VERSION:
				variety = B_TRANSLATE_MARK("gamma");
				break;
			case B_GOLDEN_MASTER_VERSION:
				variety = B_TRANSLATE_MARK("gold master");
				break;
		}

		if (variety != NULL) {
			variety = gSystemCatalog.GetString(variety, "AboutWindow");
			appVersion << "-" << variety;
		}

		return appVersion;
	}

	return NULL;
}
