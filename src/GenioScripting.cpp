/*
 * Copyright 2023, The Genio Team
 * All rights reserved. Distributed under the terms of the MIT license.
 * Authors:
 * 		Nexus6 <nexus6@disroot.org>
 */


#include "GenioApp.h"

#include <PropertyInfo.h>

#include "Editor.h"
#include "EditorTabManager.h"
#include "GenioNamespace.h"
#include "GenioWindow.h"


// Genio Scripting
namespace Properties {
	enum GeneralProperties {
		EditorCount,
		EditorCreate,
		SelectedEditor
	};

	enum EditorProperties {
		Selection,
		Symbol,
		Text,
		LineCount,
		Line,
		Undo,
		Redo,
		Ref
	};
}

const property_info sGeneralProperties[] = {
	{
		"Editor", {B_COUNT_PROPERTIES, 0},
		{B_DIRECT_SPECIFIER, 0},
		"Return the number of Editors opened.",
		0,
		{B_INT32_TYPE, 0}
	},
	{
		"Editor", {B_CREATE_PROPERTY, 0},
		{B_NAME_SPECIFIER, 0},
		"Open the file at the path provided.",
		0,
		{B_INT32_TYPE}
	},
	{
		"SelectedEditor", {0},
		{0},
		"Return the selected Editor.",
		0,
		{0}
	},
	{ 0 }
};

const property_info sEditorProperties[] = {
	{
		"Selection", {B_GET_PROPERTY, B_SET_PROPERTY, 0},
		{B_DIRECT_SPECIFIER, B_RANGE_SPECIFIER, 0},
		"Get/Set the selected text.",
		0,
		{B_STRING_TYPE, B_STRING_TYPE, 0}
	},
	{
		"Symbol", {B_GET_PROPERTY, 0},
		{B_DIRECT_SPECIFIER, 0},
		"Return the symbol under the caret.",
		0,
		{B_STRING_TYPE, 0}
	},
	{
		"Text", {B_SET_PROPERTY, 0},
		{B_NO_SPECIFIER, 0},
		"Insert the text at the specified position. "
		"If Index is omitted, insert at the current caret position or replace the current selection. "
		"If Index is -1 append at the end of the file.",
		0,
		{B_STRING_TYPE, 0}
	},
	{
		"Line", {B_COUNT_PROPERTIES, 0},
		{B_DIRECT_SPECIFIER, 0},
		"Return the number of lines in the document.",
		0,
		{B_INT32_TYPE, 0}
	},
	{
		"Line", {B_GET_PROPERTY, B_SET_PROPERTY, 0},
		{B_INDEX_SPECIFIER, B_INDEX_SPECIFIER, 0},
		"Get/Insert the line of text at the specified index.",
		0,
		{B_STRING_TYPE, 0}
	},
	{
		"Undo", {B_EXECUTE_PROPERTY, 0},
		{B_DIRECT_SPECIFIER, 0},
		"Undo last change.",
		0,
		{0}
	},
	{
		"Redo", {B_EXECUTE_PROPERTY, 0},
		{B_DIRECT_SPECIFIER, 0},
		"Redo last change.",
		0,
		{0}
	},
	{
		"Ref", {B_GET_PROPERTY, 0},
		{B_DIRECT_SPECIFIER, 0},
		"Return the entry_ref of the document.",
		0,
		{B_REF_TYPE, 0}
	},
	{ 0 }
};


void
GenioApp::_HandleScripting(BMessage* data)
{
	BMessage reply(B_REPLY);
	status_t result = B_BAD_SCRIPT_SYNTAX;
	const char* property = nullptr;
	int32 index = -1;
	int32 what = 0;
	BMessage specifier;
	Editor* editor = nullptr;

	status_t status = data->GetCurrentSpecifier(&index, &specifier, &what, &property);

	if (status != B_OK || index == -1) {
		BApplication::MessageReceived(data);
		return;
	}

	BPropertyInfo propertyInfo(const_cast<property_info*>(sGeneralProperties));
	int32 match = propertyInfo.FindMatch(data, index, &specifier, what, property);
	switch (match) {
		case Properties::GeneralProperties::EditorCount:
		{
			if (data->what == B_COUNT_PROPERTIES) {
				result = reply.AddInt32("result", fGenioWindow->TabManager()->CountTabs());
			}
			break;
		}
		case Properties::GeneralProperties::EditorCreate:
		{
			if (data->what == B_CREATE_PROPERTY) {
				BString name;
				if (specifier.FindString("name", &name) == B_OK) {
					BMessage open(B_REFS_RECEIVED);
					entry_ref ref;
					BEntry entry(name);
					entry.GetRef(&ref);
					open.AddRef("refs", &ref);
					BMessenger messenger(fGenioWindow);
					BMessage openReply;
					messenger.SendMessage(&open, &openReply);
					auto index = openReply.GetInt32("result", -1);
					result = reply.AddInt32("result", index);
				}
			}
			break;
		}
		case Properties::GeneralProperties::SelectedEditor:
		{
			BPropertyInfo editorPropertyInfo(const_cast<property_info*>(sEditorProperties));
			if (data->what == B_GET_SUPPORTED_SUITES) {
				result = reply.AddFlat("messages", &editorPropertyInfo);
				break;
			}

			data->SetCurrentSpecifier(0);
			if (data->GetCurrentSpecifier(&index, &specifier, &what, &property) != B_OK)
				break;
			specifier.PrintToStream();

			editor = fGenioWindow->TabManager()->SelectedEditor();

			int32 imatch = editorPropertyInfo.FindMatch(data, index, &specifier, what, property);
			switch (imatch) {
				case Properties::EditorProperties::Selection:
				{
					if (data->what == B_GET_PROPERTY) {
						if (editor != nullptr)
							result = reply.AddString("result", editor->Selection());
					} else if (data->what == B_SET_PROPERTY) {
						if (editor != nullptr) {
							int32 start = specifier.GetInt32("index", -1);
							int32 range = specifier.GetInt32("range", -1);
							if (start != -1 && range != -1) {
								editor->SetSelection(start, start + range);
								result = B_OK;
							}
						}
					}
					break;
				}
				case Properties::EditorProperties::Symbol:
				{
					if (data->what == B_GET_PROPERTY) {
						if (editor != nullptr)
								result = reply.AddString("result", editor->GetSymbol());
						break;
					}
				}
				case Properties::EditorProperties::Text:
				{
					if (editor != nullptr) {
						if (data->what == B_SET_PROPERTY) {
							BString text;
							int32 start = -1;
							if (data->FindString("data", &text) == B_OK) {
								if (specifier.FindInt32("index", &start) != B_OK)
									editor->Append(text);
								else
									editor->Insert(text, start);
								result = B_OK;
							}
						}
					}
					break;
				}
				case Properties::EditorProperties::LineCount:
				{
					if (data->what == B_COUNT_PROPERTIES)
						if (editor != nullptr)
							result = reply.AddInt32("result", editor->CountLines());
					break;
				}
				case Properties::EditorProperties::Line:
				{
					if (editor != nullptr) {
						BString text;
						int32 line = -1;

						if (specifier.FindInt32("index", &line) == B_OK)
						{
							if (data->what == B_SET_PROPERTY) {
								if (data->FindString("data", &text) == B_OK) {
									editor->InsertLine(text, line);
									result = B_OK;
								}
							} else {
								result = reply.AddString("result", editor->GetLine(line));
							}
						}
					}
					break;
				}
				case Properties::EditorProperties::Undo:
				{
					if (editor != nullptr) {
						if (data->what == B_EXECUTE_PROPERTY) {
							editor->Undo();
							result = B_OK;
						}
					}
					break;
				}
				case Properties::EditorProperties::Redo:
				{
					if (editor != nullptr) {
						if (data->what == B_EXECUTE_PROPERTY) {
							editor->Redo();
							result = B_OK;
						}
					}
					break;
				}
				case Properties::EditorProperties::Ref:
				{
					if (data->what == B_GET_PROPERTY && editor != nullptr)
						result = reply.AddRef("result", editor->FileRef());
					break;
				}
			}
			break;
		}
		default:
		{
			BApplication::MessageReceived(data);
			break;
		}
	}

	if (result != B_OK) {
		reply.what = B_MESSAGE_NOT_UNDERSTOOD;
		reply.AddString("message", strerror(result));
		reply.AddInt32("error", result);
	}

	data->SendReply(&reply);

	return;
}


status_t
GenioApp::GetSupportedSuites(BMessage* data)
{
	if (data == nullptr)
		return B_BAD_VALUE;

	status_t status = data->AddString("suites", GenioNames::kApplicationSignature);
	if (status != B_OK)
		return status;

	BPropertyInfo propertyInfo(const_cast<property_info*>(sGeneralProperties));
	status = data->AddFlat("messages", &propertyInfo);
	if (status == B_OK)
		status = BApplication::GetSupportedSuites(data);
	return status;
}


BHandler*
GenioApp::ResolveSpecifier(BMessage* message, int32 index, BMessage* specifier,
	int32 what, const char* property)
{
	BPropertyInfo propertyInfo(const_cast<property_info*>(sGeneralProperties));
	int32 match = propertyInfo.FindMatch(message, index, specifier, what, property);
	if (match < 0)
		return BApplication::ResolveSpecifier(message, index, specifier, what, property);
	return this;
}
// Genio Scripting
