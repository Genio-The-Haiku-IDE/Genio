/*
 * Copyright 2017-2019 Kacper Kasper <kacperkasper@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef UTILS_H
#define UTILS_H


#include <string>

#include <Alert.h>
#include <MessageFilter.h>
#include <Notification.h>


class BBitmap;
class BCheckBox;
class BRadioButton;
class BScintillaView;

struct entry_ref;

rgb_color	TextColorByLuminance(rgb_color background);

bool	GetGenioDirectory(BPath& destPath);
BPath	GetDataDirectory();
BPath	GetUserSettingsDirectory();
BString	GetVersion();
bool	IsXMasPeriod();

std::string GetFileName(const std::string filename);
std::string GetFileExtension(const std::string filename);
// Gets an icon from executable's resources
status_t GetVectorIcon(const std::string icon, BBitmap* bitmap);

// source code
status_t FindSourceOrHeader(const entry_ref* editorRef, entry_ref* foundRef);
bool IsCppSourceExtension(std::string extension);
bool IsCppHeaderExtension(std::string extension);

// Notifications
void ProgressNotification(const BString& group, const BString&  title, const BString&  messageID,
							const BString& content, float progress, bigtime_t timeout = -1);
void ShowNotification(const BString& group, const BString&  title, const BString&  messageID,
							const BString& content,
							notification_type type = B_INFORMATION_NOTIFICATION,
							bigtime_t timeout = -1);

// Convenience
template<typename T>
bool IsChecked(T* control);
template<typename T>
void SetChecked(T* control, bool checked = true);

void OKAlert(const char* title, const char* message,
	alert_type type = B_INFO_ALERT);

BString	ReadFileContent(const char* filename, off_t maxSize);

// Other
void	FakeMouseMovement(BView* view);

class KeyDownMessageFilter : public BMessageFilter {
public:
							KeyDownMessageFilter(uint32 commandToSend,
								char key, uint32 modifiers = 0, filter_result filterResult = B_SKIP_MESSAGE);

	virtual	filter_result	Filter(BMessage* message, BHandler** target);

private:
	static	uint32 			AllowedModifiers();
			char			fKey;
			uint32			fModifiers;
			uint32			fCommandToSend;
			filter_result	fFilterResult;
};

// TODO: Move to ScintillaUtils.h ?
int32 rgb_colorToSciColor(rgb_color color);
bool CanScintillaViewCut(BScintillaView* scintilla);
bool CanScintillaViewCopy(BScintillaView* scintilla);
bool CanScintillaViewPaste(BScintillaView* scintilla);
bool IsScintillaViewReadOnly(BScintillaView* scintilla);

#endif // UTILS_H
