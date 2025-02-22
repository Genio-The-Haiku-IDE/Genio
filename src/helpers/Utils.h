/*
 * Copyright 2017-2019 Kacper Kasper <kacperkasper@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef UTILS_H
#define UTILS_H


#include <OutlineListView.h>
#include <string>

#include <Alert.h>
#include <MessageFilter.h>
#include <Notification.h>
#include <ObjectList.h>

#include "Log.h"


class BBitmap;
class BCheckBox;
class BRadioButton;

struct entry_ref;

void	FakeMouseMovement(BView* view);

std::string GetFileName(const std::string filename);
std::string GetFileExtension(const std::string filename);
// Gets an icon from executable's resources
status_t GetVectorIcon(const std::string icon, BBitmap* bitmap);


std::string ParseFileArgument(const std::string argument,
	int32* line = nullptr, int32* column = nullptr);


status_t FindSourceOrHeader(const entry_ref* editorRef, entry_ref* foundRef);

bool IsCppSourceExtension(std::string extension);
bool IsCppHeaderExtension(std::string extension);

double Round(double value, int precision);
void ProgressNotification(const BString& group, const BString&  title, const BString&  messageID,
							const BString& content, float progress, bigtime_t timeout = -1);
void ShowNotification(const BString& group, const BString&  title, const BString&  messageID,
							const BString& content,
							notification_type type = B_INFORMATION_NOTIFICATION,
							bigtime_t timeout = -1);

template<typename T>
bool IsChecked(T* control);
template<typename T>
void SetChecked(T* control, bool checked = true);


void OKAlert(const char* title, const char* message,
	alert_type type = B_INFO_ALERT);


int32 rgb_colorToSciColor(rgb_color color);


class KeyDownMessageFilter : public BMessageFilter
{
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


template<type_code BType>
typename property_type<BType>::type
find_value(BMessage* message, std::string name, int index) {
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
find_value<B_REF_TYPE>(BMessage* message, std::string name, int index);


template<type_code BType>
class message_property
{
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
		iterator(BMessage* message, std::string prop_name, int index = 0)
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
	message_property(BMessage* message, std::string prop_name)
		: message_(message), prop_name_(prop_name) {}
	iterator begin() { return iterator(message_, prop_name_, 0); }
	iterator end() { return iterator(message_, prop_name_, size()); }
	size_t size() {
		int32 count;
		if(message_->GetInfo(prop_name_.c_str(), nullptr, &count) == B_OK) {
			return static_cast<size_t>(count);
		}
		return 0; // FIXME: throw an exception here
	}
};

bool		GetGenioDirectory(BPath& destPath);
BPath		GetDataDirectory();
BPath		GetUserSettingsDirectory();
BPath		GetNearbyDataDirectory();

bool	IsXMasPeriod();

BString	ReadFileContent(const char* filename, off_t maxSize);

BString	GetVersion();

#endif // UTILS_H
