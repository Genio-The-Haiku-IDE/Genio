/*
 * Copyright 2018 A. Mosca <amoscaster@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "AddToProjectWindow.h"

#include <Alignment.h>
#include <Application.h>
#include <Architecture.h>
#include <Catalog.h>
#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <LayoutBuilder.h>
#include <Path.h>
#include <SeparatorView.h>

#include <fstream>

#include "GenioNamespace.h"
#include "GenioCommon.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "AddToProjectWindow"



enum
{
	MSG_EXIT_CLICKED				= 'exit',
	MSG_ITEM_ADD_CLICKED			= 'iacl',
	MSG_ITEM_CHOSEN					= 'itch'
};

const std::string kCppExtension {".cpp"};
const std::string kCExtension {".c"};
const std::string kCppHeaderExtension {".h"};
const std::string kCppClassExtension {".{cpp,h}"};

AddToProjectWindow::AddToProjectWindow(const BString&  projectName,
	const BString& itemPath)
	:
	BWindow(BRect(0, 0, 799, 299), B_TRANSLATE("Add Item"), B_MODAL_WINDOW,
													B_ASYNCHRONOUS_CONTROLS | 
													B_NOT_ZOOMABLE |
//													B_NOT_RESIZABLE |
													B_AVOID_FRONT |
													B_AUTO_UPDATE_SIZE_LIMITS |
													B_CLOSE_ON_ESCAPE)
	, fProjectName(projectName)
	, fCurrentItem(nullptr)
{
	fCurrentDirectory.SetTo(_CurrentDirectory(itemPath));

	_InitWindow();

	CenterOnScreen();

	fFileName->MakeFocus(true);
}

AddToProjectWindow::~AddToProjectWindow()
{
}

bool
AddToProjectWindow::QuitRequested()
{
	//TODO warn on modifications?
	Quit();
	
	// TODO reactivate main window
	return true;
}

void
AddToProjectWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case MSG_EXIT_CLICKED: {
			PostMessage(B_QUIT_REQUESTED);
			break;
		}
		case MSG_ITEM_ADD_CLICKED: {
			_Add();
			break;
		}
		case MSG_ITEM_CHOSEN: {
			_ItemChosen(fItemListView->CurrentSelection());
			break;
		}
		default: {
			BWindow::MessageReceived(msg);
			break;
		}
	}
}

void
AddToProjectWindow::_Add()
{
	std::string filename(fFileName->Text());
	// Empty string, return
	if (filename.empty())
		return;

	BPath path(fDirectoryPath->Text());
	BPath secondPath(path);

	BEntry entry(path.Path());

// 	Path should start with default Projects directory ?
//	BString projectsDir(GenioNames::Settings.projects_directory);
	
	// Path leaf is not exixting, create or return
	if (!entry.Exists()) {
		// TODO: Set mask somewhere + check permissions
		if (create_directory(path.Path(), 0755) != B_OK)
			return;
	} else {
		// Path leaf is existing and not a directory, return
		if (!entry.IsDirectory())
			return;
	}

	BFile file;

	if (fCurrentItem == fCppSourceItem) {
		// Cpp file
		filename.append(kCppExtension);
		path.Append(filename.c_str());

		if (_WriteCCppSourceFile(path.Path(), fFileName->Text()) != B_OK)
			return;

	} else if (fCurrentItem == fCSourceItem) {
		// C file
		filename.append(kCExtension);
		path.Append(filename.c_str());

		if (_WriteCCppSourceFile(path.Path(), fFileName->Text()) != B_OK)
			return;

	} else if (fCurrentItem == fCppHeaderItem) {
		// Header file
		filename.append(kCppHeaderExtension);
		path.Append(filename.c_str());

		if (_WriteCppHeaderFile(path.Path(), fFileName->Text()) != B_OK)
			return;;

	} else if (fCurrentItem == fCppClassItem) {
		// Cpp file
		filename.append(kCppExtension);
		path.Append(filename.c_str());

		if (_WriteCCppSourceFile(path.Path(), fFileName->Text()) != B_OK)
			return;

		// Header file
		std::string filename2(fFileName->Text());
		filename2.append(kCppHeaderExtension);
		secondPath.Append(filename2.c_str());

		if (_WriteCppHeaderFile(secondPath.Path(), fFileName->Text()) != B_OK)
			return;

	} else if (fCurrentItem == fCppMakefileItem) {
		path.Append(filename.c_str());
		if (_WriteGenericMakefile(path.Path()) != B_OK)
			return;
	
	} else if (fCurrentItem == fHaikuMakefileItem) {
		path.Append(filename.c_str());
		if (_WriteHaikuMakefile(path.Path()) != B_OK)
			return;

	} else if (fCurrentItem == fFileItem) {
		// Generic empty file, no extension added
		path.Append(filename.c_str());
		if (_CreateFile(file, path.Path()) != B_OK)
			return;
	} else
		return;


//		fIdmproFile->AddString("project_source", path.Path());
//		fIdmproFile->AddString("project_file", path.Path());


	// Post a message
	BMessage message(GenioNames::ADDTOPROJECTWINDOW_NEW_ITEM);
	message.AddString("project", fProjectName);
	message.AddString("file_path", path.Path());
	if (fCurrentItem == fCppClassItem)
		message.AddString("file2_path", secondPath.Path());
	be_app->WindowAt(0)->PostMessage(&message);

	PostMessage(B_QUIT_REQUESTED);
}

status_t
AddToProjectWindow::_CreateFile(BFile& file, const std::string& filePath)
{
	return file.SetTo(filePath.c_str(), B_WRITE_ONLY | B_CREATE_FILE | B_FAIL_IF_EXISTS);
}

BString	const
AddToProjectWindow::_CurrentDirectory(const BString& itemPath)
{
	BPath path;
	BEntry entry(itemPath);

	if (entry.IsDirectory())
		return itemPath;
	else if (entry.GetParent(&entry) == B_OK) {
		entry.GetPath(&path);
		return path.Path();
	}
	return "";
}

void
AddToProjectWindow::_InitWindow()
{
	// Project types OutlineListView
	fItemListView = new BOutlineListView("ItemListView", B_SINGLE_SELECTION_LIST);
	fItemListView->SetSelectionMessage(new BMessage(MSG_ITEM_CHOSEN));

	fItemScrollView = new BScrollView("ItemScrollView",
		fItemListView, B_FRAME_EVENTS | B_WILL_DRAW, false, true, B_FANCY_BORDER);

	// Items
	TitleItem* cppItem = new TitleItem(B_TRANSLATE("C/C++"));
	cppItem->SetEnabled(false);
	cppItem->SetExpanded(false);
	fCppSourceItem = new BStringItem(B_TRANSLATE("C++ source file"), 1, true);
	fCSourceItem = new BStringItem(B_TRANSLATE("C source file"), 1, true);
	fCppHeaderItem = new BStringItem(B_TRANSLATE("C/C++ header file"), 1, true);
	fCppClassItem = new BStringItem(B_TRANSLATE("C++ class"), 1, true);

	fItemListView->SetToolTip(B_TRANSLATE("NOTE: Extensions automatically added"));

	TitleItem* cargoItem = new TitleItem(B_TRANSLATE("Cargo"));
	cargoItem->SetEnabled(false);
	cargoItem->SetExpanded(false);

	TitleItem* makefileItem = new TitleItem(B_TRANSLATE("Makefile"));
	makefileItem->SetEnabled(false);
	makefileItem->SetExpanded(false);
	fCppMakefileItem = new BStringItem(B_TRANSLATE("C/C++ Makefile"), 1, true);
	fHaikuMakefileItem = new BStringItem(B_TRANSLATE("Haiku Makefile"), 1, true);

	TitleItem* genericItem = new TitleItem(B_TRANSLATE("Generic"));
	genericItem->SetEnabled(false);
	genericItem->SetExpanded(false);
	fFileItem = new BStringItem(B_TRANSLATE("Generic file"), 1, true);

	fItemListView->AddItem(cppItem);
	fItemListView->AddItem(fCppSourceItem);
	fItemListView->AddItem(fCSourceItem);
	fItemListView->AddItem(fCppHeaderItem);
	fItemListView->AddItem(fCppClassItem);
	fItemListView->AddItem(cargoItem);
	fItemListView->AddItem(makefileItem);
	fItemListView->AddItem(fCppMakefileItem);
	fItemListView->AddItem(fHaikuMakefileItem);
	fItemListView->AddItem(genericItem);
	fItemListView->AddItem(fFileItem);

	// "Project" Box
	BBox* addBox = new BBox("addBox");
	addBox->SetLabel(fProjectName);

	fFileName = new BTextControl("Name", B_TRANSLATE("Name:"), "", nullptr);
//	fFileName->SetModificationMessage(new BMessage(MSG_ITEM_NAME_EDITED));

	fExtensionShown = new BTextControl("ExtensionText", "", "", nullptr);
	fExtensionShown->SetEnabled(false);

	fDirectoryPath = new BTextControl("DirectoryPath",
		B_TRANSLATE("Directory:"), "", nullptr);
//	fDirectoryPath->SetModificationMessage(new BMessage(MSG_DIRECTORY_PATH_EDITED));

	// Buttons
	BButton* exitButton = new BButton("exit",
		B_TRANSLATE("Exit"), new BMessage(MSG_EXIT_CLICKED));

	fAddButton = new BButton("AddButton", B_TRANSLATE("Add"),
		new BMessage(MSG_ITEM_ADD_CLICKED) );
	fAddButton->SetEnabled(false);

	fDirectoryPath->SetText(fCurrentDirectory);

	// Box Layout
	BLayoutBuilder::Grid<>(addBox)
	.SetInsets(10.0f)
	.AddGlue(0, 0)
	.Add(fFileName->CreateLabelLayoutItem(), 0, 1, 1)
	.Add(fFileName->CreateTextViewLayoutItem(), 1, 1, 3)
	.Add(fExtensionShown->CreateTextViewLayoutItem(), 4, 1, 1)
	.Add(fDirectoryPath->CreateLabelLayoutItem(), 0, 2, 1)
	.Add(fDirectoryPath->CreateTextViewLayoutItem(), 1, 2, 5)
	.AddGlue(0, 3)
	;

	// Window layout
	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
//		.SetInsets(2.0f)
		.AddGroup(B_HORIZONTAL)
			.Add(fItemScrollView, 2.0f)
			.Add(addBox, 6.0f)
		.End()
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(exitButton)
			.AddGlue(4.0f)
			.Add(fAddButton)
			.AddGlue()
		.End()
	;
}

void
AddToProjectWindow::_ItemChosen(int32 selection)
{
	if (selection < 0 ) {
		fCurrentItem = nullptr;
		fAddButton->SetEnabled(false);
		fExtensionShown->SetText("");
		return;
	}

	fCurrentItem = dynamic_cast<BStringItem*>(fItemListView->ItemAt(selection));
	fAddButton->SetEnabled(true);

	if (fCurrentItem == fCppSourceItem)
		fExtensionShown->SetText(kCppExtension.c_str());
	else if (fCurrentItem == fCSourceItem)
		fExtensionShown->SetText(kCExtension.c_str());
	else if (fCurrentItem == fCppHeaderItem)
		fExtensionShown->SetText(kCppHeaderExtension.c_str());
	else if (fCurrentItem == fCppClassItem)
		fExtensionShown->SetText(kCppClassExtension.c_str());
	else
		fExtensionShown->SetText("");
}

status_t
AddToProjectWindow::_WriteCCppSourceFile(const std::string& filePath,
										const std::string& fileLeaf)
{
	if(Genio::file_exists(filePath))
		return B_ERROR;	

	std::ofstream file_out(filePath);	
	if (!file_out.is_open())
		return B_ERROR;

	file_out << Genio::Copyright()
		<< "#include \"" << fileLeaf << kCppHeaderExtension <<  "\"\n\n\n"
		<< fFileName->Text() << "::" << fFileName->Text() << "()" << "\n"
		<< "{" << "\n"
		<< "}" << "\n\n"
		<< fFileName->Text() << "::" << "~" << fFileName->Text() << "()" << "\n"
		<< "{" << "\n"
		<< "}" << "\n\n"
	;

	return B_OK;
}

status_t
AddToProjectWindow::_WriteCppHeaderFile(const std::string& filePath,
									const std::string& fileLeaf)
{
	if(Genio::file_exists(filePath))
		return B_ERROR;	

	std::ofstream file_out(filePath);	
	if (!file_out.is_open())
		return B_ERROR;

	std::string headerGuard = Genio::HeaderGuard(fileLeaf);
	headerGuard += "_H";

	file_out
		<< Genio::Copyright()
		<< "#ifndef " << headerGuard.c_str() << "\n"
		<< "#define " << headerGuard.c_str() << "\n\n"
		<< "class " << fFileName->Text() << " {" << "\n"
		<< "public:\n"
		<< "\t\t\t\t\t\t\t\t" << fFileName->Text() << "();" << "\n"
		<< "\t\t\t\t\t\t\t\t" << "~" << fFileName->Text() << "();" << "\n\n\n"
		<< "private:\n\n"
		<< "};\n\n\n"
		<< "#endif // " << headerGuard.c_str() << "\n"
	;

	return B_OK;
}

status_t
AddToProjectWindow::_WriteGenericMakefile(const std::string& filePath)
{
	if(Genio::file_exists(filePath))
		return B_ERROR;

	std::ofstream file_out(filePath);	
	if (!file_out.is_open())
		return B_ERROR;

	std::string arch("");
	if (!strcmp(get_primary_architecture(), "x86_gcc2"))
		arch = "-x86";

	file_out <<
"## Genio generic Makefile ######################################################\n"
		<< "##\n\n"
		<< "CC		:= gcc" << arch << "\n"
		<< "C++		:= g++" << arch << "\n"
		<< "LD		:= gcc" << arch << "\n\n"

		<< "# Determine the CPU type\n"
		<< "CPU = $(shell uname -m)\n"
		<< "# Get the compiler version.\n"
		<< "CC_VER = $(word 1, $(subst -, , $(subst ., , $(shell $(CC) -dumpversion))))\n\n"

		<< "Debug    := -g -ggdb\n"
		<< "CFLAGS   := -c -Wall -O2 ${Debug}\n"
		<< "CXXFLAGS := -std=c++11\n"
		<< "ASFLAGS   =\n"
		<< "LDFLAGS   = -Xlinker -soname=_APP_\n"
		<< "LIBS	  = -lstdc++ -lsupc++ \n\n"
		<<
"################################################################################\n"
		<< "# Vars\n"
		<< "#\n\n"

		<< "# application name\n"
		<< "target	:= app/"  << "\n\n"

		<< "# sources directories\n"
		<< "dirs	:= src\n\n"

		<< "# find sources\n"
		<< "sources := $(foreach dir, $(dirs), $(wildcard $(dir)/*.cpp $(dir)/*.c  $(dir)/*.S))\n\n"
//		<< "sources := $(foreach dir, $(dirs), $(wildcard $(dir)/*.[cpp,c,S]))\n\n"
		<< "# object files top directory\n"
		<< "objdir  := objects.$(CPU)-$(CC)$(CC_VER)-$(if $(Debug),debug,release)\n\n"

		<< "# ensue the object files\n"
		<< "objects :=	$(patsubst \%.S, $(objdir)/\%.o, $(filter \%.S,$(sources))) \\\n"
		<< "\t\t\t$(patsubst \%.c, $(objdir)/\%.o, $(filter \%.c,$(sources))) \\\n"
		<< "\t\t\t$(patsubst \%.cpp, $(objdir)/\%.o, $(filter \%.cpp,$(sources)))\n\n"

		<< "# object files subdirs\n"
		<< "objdirs := $(addprefix $(objdir)/, $(dirs))\n\n"

		<<
"################################################################################\n"

		<< "all: $(objdirs) $(target)\n\n"

		<< "$(objdir)/\%.o: \%.c\n"
		<< "\t${CC}  ${CFLAGS} $(includes) $< -o $@\n\n"

		<< "$(objdir)/\%.o: \%.cpp\n"
		<< "\t${C++}  ${CFLAGS} $(CXXFLAGS) $(includes) $< -o $@\n\n"
			
		<< "$(objdir)/\%.o: \%.S\n"
		<< "\t${CC} ${CFLAGS} ${ASFLAGS} $(includes) $< -o $@\n\n"

		<< "$(target): $(objects)\n"
		<< "\t${LD} $(DEBUG) ${LDFLAGS} ${LIBS} -o $@ $^\n\n"

		<< "# create needed dirs to store object files\n"
		<< "$(objdirs):\n"
		<< "\t@echo Creating dir: $@\n"
		<< "\t@mkdir -p  $@\n\n"

		<< "clean:\n"
		<< "\t@echo cleaning: $(objdir)\n"
		<< "\t@rm -f $(objects)\n\n"

		<< "rmapp:\n"
		<< "\t@echo cleaning: $(target)\n"
		<< "\t@rm -f $(target)\n\n"

		<< "list:\n"
		<< "\t@echo srcs: $(sources)\n"
		<< "\t@echo objs: $(objects)\n"
		<< "\t@echo dirs: $(objdirs)\n\n"

		<< "print-defines:\n"
		<< "\t@echo | ${CC} -dM -E -\n\n\n"


		<< ".PHONY:  clean rmapp list print-defines objdirs\n";

	return B_OK;
}

status_t
AddToProjectWindow::_WriteHaikuMakefile(const std::string& filePath)
{
	if(Genio::file_exists(filePath))
		return B_ERROR;

	status_t status;
	BPath path;

	status = find_directory(B_SYSTEM_DEVELOP_DIRECTORY, &path);
	if (status != B_OK)
		return status;

	path.Append("etc/Makefile");
	BEntry entry(path.Path());
	if (!entry.Exists())
		return B_ERROR;

	std::ifstream file_in(path.Path());
	std::fstream file_out(filePath, std::fstream::out);

	if (!file_in.is_open() || !file_out.is_open())
		return B_ERROR;
	
	std::string line;
	while (std::getline(file_in, line))
		file_out << line << std::endl;

//	if (!file_in.eof())
//		return B_ERROR;

	return B_OK;
}


