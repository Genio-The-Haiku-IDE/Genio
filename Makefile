## Genio - The Haiku IDE Makefile ##############################################
debug ?= 0
ifeq ($(debug), 0)
	DEBUGGER :=
	CFLAGS :=
else
	DEBUGGER := TRUE
	CFLAGS := -DGDEBUG
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

arch := $(shell getarch)
platform := $(shell uname -p)

SRCS := src/GenioApp.cpp
SRCS += src/alert/GTextAlert.cpp
SRCS += src/config/ConfigManager.cpp
SRCS += src/config/ConfigWindow.cpp
SRCS += src/config/GMessage.cpp
SRCS += src/helpers/ActionManager.cpp
SRCS += src/helpers/FSUtils.cpp
SRCS += src/helpers/HvifImporter.cpp
SRCS += src/helpers/GSettings.cpp
SRCS += src/helpers/Logger.cpp
SRCS += src/helpers/StatusView.cpp
SRCS += src/helpers/TextUtils.cpp
SRCS += src/helpers/Utils.cpp
SRCS += src/helpers/GrepThread.cpp
SRCS += src/helpers/console_io/ConsoleIOView.cpp
SRCS += src/helpers/console_io/ConsoleIOThread.cpp
SRCS += src/helpers/console_io/GenericThread.cpp
SRCS += src/helpers/console_io/WordTextView.cpp
SRCS += src/helpers/tabview/TabContainerView.cpp
SRCS += src/helpers/tabview/TabManager.cpp
SRCS += src/helpers/tabview/TabView.cpp
SRCS += src/helpers/tabview/CircleColorMenuItem.cpp
SRCS += src/helpers/Languages.cpp
SRCS += src/helpers/Styler.cpp
SRCS += src/lsp-client/LSPEditorWrapper.cpp
SRCS += src/lsp-client/LSPProjectWrapper.cpp
SRCS += src/lsp-client/LSPPipeClient.cpp
SRCS += src/lsp-client/Transport.cpp
SRCS += src/lsp-client/LSPReaderThread.cpp
SRCS += src/lsp-client/LSPServersManager.cpp
SRCS += src/lsp-client/CallTipContext.cpp
SRCS += src/override/BarberPole.cpp
SRCS += src/project/ProjectFolder.cpp
SRCS += src/project/ProjectItem.cpp
SRCS += src/git/BranchItem.cpp
SRCS += src/git/GitRepository.cpp
SRCS += src/git/GitAlert.cpp
SRCS += src/git/GitCredentialsWindow.cpp
SRCS += src/git/RemoteProjectWindow.cpp
SRCS += src/git/RepositoryView.cpp
SRCS += src/git/SourceControlPanel.cpp
SRCS += src/git/SwitchBranchMenu.cpp
SRCS += src/ui/EditorStatusView.cpp
SRCS += src/ui/Editor.cpp
SRCS += src/ui/EditorContextMenu.cpp
SRCS += src/ui/EditorTabManager.cpp
SRCS += src/ui/FunctionsOutlineView.cpp
SRCS += src/ui/GenioWindow.cpp
SRCS += src/ui/GoToLineWindow.cpp
SRCS += src/ui/IconCache.cpp
SRCS += src/ui/ProblemsPanel.cpp
SRCS += src/ui/ProjectsFolderBrowser.cpp
SRCS += src/ui/SearchResultPanel.cpp
SRCS += src/ui/StyledItem.cpp
SRCS += src/ui/ToolBar.cpp
SRCS += src/ui/QuitAlert.cpp
SRCS += src/templates/IconMenuItem.cpp
SRCS += src/templates/TemplatesMenu.cpp
SRCS += src/templates/TemplateManager.cpp
SRCS += src/helpers/PipeImage.cpp

RDEFS := Genio.rdef

LIBS  = be shared translation localestub $(STDCPPLIBS)
LIBS += columnlistview tracker
LIBS += git2
LIBS += src/scintilla/bin/libscintilla.a
LIBS += yaml-cpp

SYSTEM_INCLUDE_PATHS  = $(shell findpaths -e B_FIND_PATH_HEADERS_DIRECTORY private/interface)
SYSTEM_INCLUDE_PATHS += $(shell findpaths -e B_FIND_PATH_HEADERS_DIRECTORY private/shared)
SYSTEM_INCLUDE_PATHS += $(shell findpaths -e B_FIND_PATH_HEADERS_DIRECTORY private/storage)
SYSTEM_INCLUDE_PATHS += $(shell findpaths -e B_FIND_PATH_HEADERS_DIRECTORY private/support)
SYSTEM_INCLUDE_PATHS += $(shell findpaths -e B_FIND_PATH_HEADERS_DIRECTORY private/tracker)
SYSTEM_INCLUDE_PATHS += $(shell findpaths -e B_FIND_PATH_HEADERS_DIRECTORY private/locale)
SYSTEM_INCLUDE_PATHS += $(shell findpaths -a $(platform) -e B_FIND_PATH_HEADERS_DIRECTORY lexilla)
SYSTEM_INCLUDE_PATHS += src/scintilla/haiku
SYSTEM_INCLUDE_PATHS += src/scintilla/include

# For BarberPole.h, which is not available in beta4
SYSTEM_INCLUDE_PATHS += src/override

CFLAGS += -Wall -Werror
CXXFLAGS := -std=c++20 -fPIC

#ifneq ($(BUILD_WITH_CLANG), 0)
	ifneq ($(debug), 0)
		CXXFLAGS += -gdwarf-3
	endif
#endif

LOCALES := ca de en en_AU en_GB es es_419 fr fur it nb sc tr

## Include the Makefile-Engine
include $(BUILDHOME)/etc/makefile-engine

## CXXFLAGS rule
$(OBJ_DIR)/%.o : %.cpp
	$(CXX) -c $< $(INCLUDES) $(CFLAGS) $(CXXFLAGS) -o "$@"

deps:
	$(MAKE) -C src/scintilla/haiku

.PHONY: clean deps

cleanall: clean
	$(MAKE) clean -C src/scintilla/haiku
	rm -f txt2header
	rm -f Changelog.h

$(TARGET): deps

GenioApp.cpp : Changelog.h

Changelog.h : Changelog.txt txt2header
	txt2header < Changelog.txt > Changelog.h

txt2header :
	$(CXX) txt2header.cpp -o txt2header

