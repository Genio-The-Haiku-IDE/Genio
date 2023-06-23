/*
	PathBox.cpp: A control to easily allow the user to choose a file/folder path
	Written by DarkWyrm <darkwyrm@earthlink.net>, Copyright 2007
	Released under the MIT license.
*/


#include "PathBox.h"

#include <Alert.h>
#include <Alignment.h>
#include <LayoutBuilder.h>
#include <Messenger.h>
#include <Path.h>
#include <PropertyInfo.h>
#include <Size.h>
#include <String.h>
#include <Window.h>


enum {
	M_PATHBOX_CHANGED = 'pbch',
	M_SHOW_FILEPANEL,
	M_ENTRY_CHOSEN
};


static property_info sProperties[] = {
	{
		"Value",
		{ B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 },
		"Returns the whether or not the path box is enabled.",
		0,
		{ B_BOOL_TYPE }
	},
	
	{
		"Value",
		{ B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0},
		"Enables/disables the path box.",
		0,
		{ B_BOOL_TYPE }
	},

	{
		"Enabled",
		{ B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 },
		"Returns the value for the path box.",
		0,
		{ B_STRING_TYPE }
	},
	
	{
		"Enabled",
		{ B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0},
		"Sets the value for the path box.",
		0,
		{ B_STRING_TYPE }
	},
};


class DropControl : public BTextControl {
public:
							DropControl(BRect frame, const char* name,
								const char* label, const char* text,
								BMessage* message,
								uint32 resizingMode = B_FOLLOW_LEFT | B_FOLLOW_TOP,
								uint32 flags = B_WILL_DRAW | B_NAVIGABLE);
							DropControl(const char* name, const char* label,
								const char* text, BMessage* message,
								uint32 flags = B_WILL_DRAW | B_NAVIGABLE);
							DropControl(BMessage *data);
	virtual					~DropControl(void);

	static	BArchivable*	Instantiate(BMessage *data);
	virtual	status_t		Archive(BMessage *data, bool deep = true) const;
	virtual	void			MessageReceived(BMessage *message);
};


//	#pragma mark - DropControl


DropControl::DropControl(BRect frame, const char* name, const char* label,
	const char* text, BMessage* message, uint32 resizingMode, uint32 flags)
	:
	BTextControl(frame, name, label, text, message, resizingMode, flags)
{
}

DropControl::DropControl(const char* name, const char* label, const char* text,
	BMessage* message, uint32 flags)
	:
	BTextControl(name, label, text, message, flags)
{
}


DropControl::DropControl(BMessage *data)
	:	BTextControl(data)
{
}


DropControl::~DropControl(void)
{
}


BArchivable*
DropControl::Instantiate(BMessage* data)
{
	if (validate_instantiation(data, "DropControl"))
		return new DropControl(data);

	return NULL;
}


status_t
DropControl::Archive(BMessage* data, bool deep) const
{
	status_t status = BTextControl::Archive(data,deep);
	data->AddString("class","DropControl");
	return status;
}


void
DropControl::MessageReceived(BMessage* message)
{
	if (message->WasDropped() && Window() != NULL) {
		entry_ref ref;
		if (message->FindRef("refs",&ref) == B_OK) {
			BPath path(&ref);
			SetText(path.Path());
		}
	} else
		BTextControl::MessageReceived(message);
}


//	#pragma mark - PathBox


PathBox::PathBox(const BRect &frame, const char* name, const char* path,
	const char* label, const int32 &resizingMode, const int32 &flags)
	:
	BView(frame, name, resizingMode, flags),
	fFilePanel(NULL),
	fPathControl(NULL),
	fBrowseButton(NULL),
 	fValidate(false)
{
	_Init(path);

	fPathControl = new DropControl(BRect(0, 0, 1, 1), "path", label, path,
		new BMessage(M_PATHBOX_CHANGED), B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);

	fBrowseButton = new BButton(BRect(0, 0, 1, 1), "browse",
		"Browse" B_UTF8_ELLIPSIS, new BMessage(M_SHOW_FILEPANEL));
	fBrowseButton->ResizeToPreferred();

	float buttonLeft = frame.right - fBrowseButton->Bounds().Width();
	fBrowseButton->MoveTo(buttonLeft, 0);

	float width;
	float height;
	fPathControl->GetPreferredSize(&width, &height);
	fPathControl->ResizeTo(buttonLeft - 10.0f, height);
	fPathControl->MoveTo(frame.left, floorf((fBrowseButton->Bounds().Height()
		- fPathControl->Bounds().Height()) / 2));

	AddChild(fPathControl);
	AddChild(fBrowseButton);
}


PathBox::PathBox(const char* name, const char* path, const char* label,
	const int32 &flags)
	:
	BView(name, flags),
	fFilePanel(NULL),
	fPathControl(NULL),
	fBrowseButton(NULL),
 	fValidate(false)
{
	_Init(path);

	fPathControl = new DropControl("path", label, path,
		new BMessage(M_PATHBOX_CHANGED));
	fPathControl->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT,
		B_ALIGN_VERTICAL_CENTER));

	fBrowseButton = new BButton("browse", "Browse" B_UTF8_ELLIPSIS,
		new BMessage(M_SHOW_FILEPANEL));

	BLayoutBuilder::Group<>(this, B_HORIZONTAL)
		.Add(fPathControl)
		.Add(fBrowseButton)
		.End();
}


PathBox::PathBox(BMessage* data)
	:
	BView(data)
{
	BString path;
	
	if (data->FindString("path",&path) != B_OK)
		path = "";

	fPathControl->SetText(path.String());

	if (data->FindBool("validate",&fValidate) != B_OK)
		fValidate = false;
}


PathBox::~PathBox(void)
{
	delete fFilePanel;
	delete fPathControl;
	delete fBrowseButton;
}


void
PathBox::AttachedToWindow(void)
{
	fFilePanel->SetTarget(BMessenger(this));
	fPathControl->SetTarget(this);
	fBrowseButton->SetTarget(this);
}


BArchivable*
PathBox::Instantiate(BMessage *data)
{
	if (validate_instantiation(data, "PathBox"))
		return new PathBox(data);

	return NULL;
}


status_t
PathBox::Archive(BMessage* data, bool deep) const
{
	status_t status = BView::Archive(data,deep);
	data->AddString("class", "PathBox");

	if (status == B_OK)
		status = data->AddString("path", fPathControl->Text());

	if (status == B_OK)
		status = data->AddBool("validate", fValidate);

	return status;
}


void
PathBox::ResizeToPreferred(void)
{
	float width;
	float height;

	GetPreferredSize(&width, &height);
	ResizeTo(width, height);
	fPathControl->ResizeToPreferred();
	fBrowseButton->ResizeToPreferred();
	fPathControl->ResizeTo(fBrowseButton->Frame().left - 10,
		fPathControl->Bounds().Height());
}


void
PathBox::GetPreferredSize(float* width, float* height)
{
	float newWidth;
	float newHeight;
	float tempw;
	float temph;

	fPathControl->GetPreferredSize(&newWidth, &newHeight);
	fBrowseButton->GetPreferredSize(&tempw, &temph);
	newWidth += tempw + 30;
	newHeight = (newHeight > temph) ? newHeight : temph;

	if (width != NULL)
		*width = newWidth;

	if (height != NULL)
		*height = newHeight;
}

	
status_t
PathBox::GetSupportedSuites(BMessage* message)
{
	message->AddString("suites","suite/vnd.DW-pathbox");

	BPropertyInfo prop_info(sProperties);
	message->AddFlat("messages",&prop_info);
	return BView::GetSupportedSuites(message);
}


BHandler*
PathBox::ResolveSpecifier(BMessage* message, int32 index,
	BMessage* specifier, int32 form, const char* property)
{
	BPropertyInfo propInfo(sProperties);

	if (propInfo.FindMatch(message, 0, specifier, form, property) >= B_OK)
		return this;

	return BView::ResolveSpecifier(message, index, specifier, form, property);
}


void
PathBox::MessageReceived(BMessage *message)
{
	switch(message->what) {
		case M_ENTRY_CHOSEN: {
			entry_ref ref;
			if (message->FindRef("refs",&ref) == B_OK) {
				BPath path(&ref);
				fPathControl->SetText(path.Path());
			}
			break;
		}
		case M_SHOW_FILEPANEL: {
			if (!fFilePanel->IsShowing())
				fFilePanel->Show();
			break;
		}
		case M_PATHBOX_CHANGED: {
			if (IsValidating()) {
				if (strlen(fPathControl->Text()) < 1)
					break;
				
				BEntry entry(fPathControl->Text());
				if (entry.InitCheck() != B_OK
					|| !entry.Exists()) {
					BAlert *alert = new BAlert("", "The location entered does not exist."
												" Please check to make sure you have"
												" entered it correctly.",
												"OK");
					alert->Go();
					fPathControl->MakeFocus();
				}
			}
			break;
		}
		case B_SET_PROPERTY:
		case B_GET_PROPERTY: {
			BMessage reply(B_REPLY);
			bool handled = false;
	
			BMessage specifier;
			int32 index;
			int32 form;
			const char *property;
			if (message->GetCurrentSpecifier(&index, &specifier, &form, &property) == B_OK) {
				if (strcmp(property, "Value") == 0) {
					if (message->what == B_GET_PROPERTY) {
						reply.AddString("result", fPathControl->Text());
						handled = true;
					} else {
						// B_GET_PROPERTY
						BString value;
						if (message->FindString("data", &value) == B_OK) {
							SetPath(value.String());
							reply.AddInt32("error", B_OK);
							handled = true;
						}
					}
				} else if (strcmp(property, "Enabled") == 0) {
					if (message->what == B_GET_PROPERTY) {
						reply.AddBool("result", IsEnabled());
						handled = true;
					} else {
						// B_GET_PROPERTY
						bool enabled;
						if (message->FindBool("data", &enabled) == B_OK) {
							SetEnabled(enabled);
							reply.AddInt32("error", B_OK);
							handled = true;
						}
					}
				}
			}
			
			if (handled) {
				message->SendReply(&reply);
				return;
			}
			break;
		}
		default: {
			BView::MessageReceived(message);
			break;
		}
	}
}


void
PathBox::SetEnabled(bool value)
{
	if (value == IsEnabled())
		return;
	
	fPathControl->SetEnabled(value);
	fBrowseButton->SetEnabled(value);
}


bool
PathBox::IsEnabled(void) const
{
	return fPathControl->IsEnabled();
}


void
PathBox::SetPath(const char* path)
{
	fPathControl->SetText(path);
}


void
PathBox::SetPath(const entry_ref& ref)
{
	BPath path(&ref);
	fPathControl->SetText(path.Path());
}


const char*
PathBox::Path(void) const
{
	return fPathControl->Text();
}


void
PathBox::SetDivider(float position)
{
	if (fPathControl != NULL)
		fPathControl->SetDivider(position);
}


float
PathBox::Divider(void) const
{
	if (fPathControl == NULL)
		return 0.0f;

	return fPathControl->Divider();
}


void
PathBox::SetLabel(const char* label)
{
	if (fPathControl != NULL)
		fPathControl->SetLabel(label);
}


const char*
PathBox::Label() const
{
	if (fPathControl == NULL)
		return NULL;

	return fPathControl->Label();
};

			
void
PathBox::MakeValidating(bool value)
{
	fValidate = value;
}


bool
PathBox::IsValidating(void) const
{
	return fValidate;
}


void
PathBox::MakeFocus(bool focus)
{
	if (focus)
		fPathControl->MakeFocus(true);
}


BFilePanel *
PathBox::FilePanel(void) const
{
	return fFilePanel;
}


//	#pragma mark - PathBox private methods


void
PathBox::_Init(const char* path)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	BMessenger messager(this);
	BEntry entry(path);
	entry_ref ref;
	entry.GetRef(&ref);

	fFilePanel = new BFilePanel(B_OPEN_PANEL, &messager, &ref,
		B_DIRECTORY_NODE | B_SYMLINK_NODE, false,
		new BMessage(M_ENTRY_CHOSEN));
	fFilePanel->SetButtonLabel(B_DEFAULT_BUTTON, "Select");
}
