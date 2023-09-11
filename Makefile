## Genio - The Haiku IDE Makefile ##############################################

arch := $(shell getarch)
platform := $(shell uname -p)

debug ?= 0
ifeq ($(debug), 0)
	DEBUGGER :=
else
	DEBUGGER := TRUE
endif

## clang build flag ############################################################
BUILD_WITH_CLANG := 0
################################################################################
ifeq ($(BUILD_WITH_CLANG), 1)
	# clang build
	CC  := clang
	CXX := clang++
	LD  := clang++
endif

ifeq ($(debug), 0)
	NAME := Genio
else
	NAME := Genio_debug
endif


TARGET_DIR := app

TYPE := APP

APP_MIME_SIG := "application/x-vnd.Genio"

SRCS :=  src/GenioApp.cpp
SRCS +=  src/GenioNamespace.cpp
SRCS +=  src/helpers/ActionManager.cpp
SRCS +=  src/helpers/FSUtils.cpp
SRCS +=  src/helpers/GenioCommon.cpp
SRCS +=  src/helpers/GSettings.cpp
SRCS +=  src/helpers/Logger.cpp
SRCS +=  src/helpers/PathBox.cpp
SRCS +=  src/helpers/StatusView.cpp
SRCS +=  src/helpers/TextUtils.cpp
SRCS +=  src/helpers/TPreferences.cpp
SRCS +=  src/helpers/Utils.cpp
SRCS +=  src/helpers/GrepThread.cpp
SRCS +=  src/helpers/console_io/ConsoleIOView.cpp
SRCS +=  src/helpers/console_io/ConsoleIOThread.cpp
SRCS +=  src/helpers/console_io/GenericThread.cpp
SRCS +=  src/helpers/console_io/WordTextView.cpp
SRCS +=  src/helpers/tabview/TabContainerView.cpp
SRCS +=  src/helpers/tabview/TabManager.cpp
SRCS +=  src/helpers/tabview/TabView.cpp
SRCS +=  src/helpers/git/GitRepository.cpp
SRCS +=  src/lsp-client/FileWrapper.cpp
SRCS +=  src/lsp-client/LSPClientWrapper.cpp
SRCS +=  src/lsp-client/LSPClient.cpp
SRCS +=  src/lsp-client/Transport.cpp
SRCS +=  src/override/MenuItem.cpp
SRCS +=  src/override/OutlineListView.cpp
SRCS +=  src/project/ProjectSettingsWindow.cpp
SRCS +=  src/project/ProjectFolder.cpp
SRCS +=  src/project/ProjectItem.cpp
SRCS +=  src/ui/EditorStatusView.cpp
SRCS +=  src/ui/Editor.cpp
SRCS +=  src/ui/EditorContextMenu.cpp
SRCS +=  src/ui/EditorTabManager.cpp
SRCS +=  src/ui/GenioWindow.cpp
SRCS +=  src/ui/GoToLineWindow.cpp
SRCS +=  src/ui/IconCache.cpp
SRCS +=  src/ui/ProblemsPanel.cpp
SRCS +=  src/ui/SearchResultPanel.cpp
SRCS +=  src/ui/ProjectsFolderBrowser.cpp
SRCS +=  src/ui/RemoteProjectWindow.cpp
SRCS +=  src/ui/SettingsWindow.cpp
SRCS +=  src/ui/ToolBar.cpp
SRCS +=  src/ui/QuitAlert.cpp
SRCS +=  src/templates/IconMenuItem.cpp
SRCS +=  src/templates/TemplatesMenu.cpp
SRCS +=  src/templates/TemplateManager.cpp

RDEFS := Genio.rdef

LIBS = be shared translation localestub $(STDCPPLIBS)
LIBS += columnlistview tracker
LIBS += git2
LIBS += src/lexilla/bin/liblexilla.a
LIBS += src/scintilla/bin/libscintilla.a

# LIBPATHS = $(shell findpaths -a $(platform) B_FIND_PATH_DEVELOP_LIB_DIRECTORY)
# LIBPATHS  = /boot/home/config/non-packaged/lib
# $(info LIBPATHS="$(LIBPATHS)")

SYSTEM_INCLUDE_PATHS  = $(shell findpaths -e B_FIND_PATH_HEADERS_DIRECTORY private/interface)
SYSTEM_INCLUDE_PATHS +=	$(shell findpaths -e B_FIND_PATH_HEADERS_DIRECTORY private/shared)
SYSTEM_INCLUDE_PATHS +=	$(shell findpaths -e B_FIND_PATH_HEADERS_DIRECTORY private/storage)
SYSTEM_INCLUDE_PATHS +=	$(shell findpaths -e B_FIND_PATH_HEADERS_DIRECTORY private/support)
SYSTEM_INCLUDE_PATHS +=	$(shell findpaths -e B_FIND_PATH_HEADERS_DIRECTORY private/tracker)
# SYSTEM_INCLUDE_PATHS +=	$(shell findpaths -a $(platform) -e B_FIND_PATH_HEADERS_DIRECTORY scintilla)
#SYSTEM_INCLUDE_PATHS +=	$(shell findpaths -a $(platform) -e B_FIND_PATH_HEADERS_DIRECTORY lexilla)
SYSTEM_INCLUDE_PATHS   +=  src/scintilla/haiku
SYSTEM_INCLUDE_PATHS   +=  src/scintilla/include
SYSTEM_INCLUDE_PATHS  += src/lexilla/include

SYSTEM_INCLUDE_PATHS  +=  /boot/system/develop/headers/private
LOCAL_INCLUDE_PATHS  +=  src/lsp-client/include

CFLAGS := -Wall -Werror

CXXFLAGS := -std=c++17 -fPIC

#ifneq ($(BUILD_WITH_CLANG), 0)
	ifneq ($(debug), 0)
		CXXFLAGS += -gdwarf-3
	endif
#endif

LOCALES := en it

## Include the Makefile-Engine
ENGINE_DIRECTORY := $(shell findpaths -r "makefile_engine" B_FIND_PATH_DEVELOP_DIRECTORY)
include $(ENGINE_DIRECTORY)/etc/makefile-engine

## CXXFLAGS rule
$(OBJ_DIR)/%.o : %.cpp
	$(CXX) -c $< $(INCLUDES) $(CFLAGS) $(CXXFLAGS) -o "$@"

deps:
	$(MAKE) -C src/lexilla/src
	$(MAKE) -C src/scintilla/haiku  

.PHONY: clean deps
cleanall: clean
	$(MAKE) clean -C src/scintilla/haiku
	$(MAKE) clean -C src/lexilla/src
       
$(TARGET): deps

GenioApp.cpp : Changelog.h

Changelog.h : txt2header Changelog.txt
	txt2header < Changelog.txt > Changelog.h

txt2header : txt2header.cpp

