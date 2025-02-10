## Genio - The Haiku IDE Makefile ##############################################
COMPILER_FLAGS = -Werror -std=c++20 -gdwarf-3
WARNINGS = ALL

TARGET_DIR := app
TYPE := APP

APP_MIME_SIG := "application/x-vnd.Genio"

debug ?= 0
ifneq ($(debug), 0)
	DEBUGGER := TRUE
endif

ifeq ($(strip $(DEBUGGER)), TRUE)
	NAME := Genio_debug
	COMPILER_FLAGS += -DGDEBUG
else
	NAME := Genio
endif

arch := $(shell getarch)
platform := $(shell uname -p)

SRCS := src/GenioApp.cpp
SRCS += src/GenioScripting.cpp
SRCS += src/alert/GTextAlert.cpp
SRCS += src/config/ConfigManager.cpp
SRCS += src/config/ConfigWindow.cpp
SRCS += src/config/GMessage.cpp
SRCS += src/extensions/ExtensionManager.cpp
SRCS += src/extensions/ToolsMenu.cpp
SRCS += src/helpers/ActionManager.cpp
SRCS += src/helpers/FSUtils.cpp
SRCS += src/helpers/Languages.cpp
SRCS += src/helpers/Logger.cpp
SRCS += src/helpers/MakeFileHandler.cpp
SRCS += src/helpers/GSettings.cpp
SRCS += src/helpers/GrepThread.cpp
SRCS += src/helpers/PipeImage.cpp
SRCS += src/helpers/ResourceImport.cpp
SRCS += src/helpers/StatusView.cpp
SRCS += src/helpers/Styler.cpp
SRCS += src/helpers/TextUtils.cpp
SRCS += src/helpers/Utils.cpp
SRCS += src/helpers/console_io/ConsoleIOView.cpp
SRCS += src/helpers/console_io/ConsoleIOThread.cpp
SRCS += src/helpers/console_io/GenericThread.cpp
SRCS += src/helpers/console_io/WordTextView.cpp
SRCS += src/helpers/mterm/KeyTextViewScintilla.cpp
SRCS += src/helpers/mterm/MTerm.cpp
SRCS += src/helpers/mterm/MTermView.cpp
SRCS += src/helpers/JumpNavigator.cpp
SRCS += src/helpers/CircleColorMenuItem.cpp
SRCS += src/helpers/gtab/GTabView.cpp
SRCS += src/helpers/gtab/TabsContainer.cpp
SRCS += src/helpers/gtab/GTab.cpp
SRCS += src/lsp-client/CallTipContext.cpp
SRCS += src/lsp-client/LSPEditorWrapper.cpp
SRCS += src/lsp-client/LSPProjectWrapper.cpp
SRCS += src/lsp-client/LSPPipeClient.cpp
SRCS += src/lsp-client/LSPReaderThread.cpp
SRCS += src/lsp-client/LSPServersManager.cpp
SRCS += src/lsp-client/Transport.cpp
SRCS += src/project/ProjectFolder.cpp
SRCS += src/project/ProjectItem.cpp
SRCS += src/git/BranchItem.cpp
SRCS += src/git/GitAlert.cpp
SRCS += src/git/GitCredentialsWindow.cpp
SRCS += src/git/GitRepository.cpp
SRCS += src/git/RemoteProjectWindow.cpp
SRCS += src/git/RepositoryView.cpp
SRCS += src/git/SourceControlPanel.cpp
SRCS += src/git/SwitchBranchMenu.cpp
SRCS += src/editor/GTabEditor.cpp
SRCS += src/ui/EditorStatusView.cpp
SRCS += src/ui/Editor.cpp
SRCS += src/ui/EditorContextMenu.cpp
SRCS += src/ui/FunctionsOutlineView.cpp
SRCS += src/ui/GenioWindow.cpp
SRCS += src/ui/GenioSecondaryWindow.cpp
SRCS += src/ui/GlobalStatusView.cpp
SRCS += src/ui/GoToLineWindow.cpp
SRCS += src/ui/IconCache.cpp
SRCS += src/ui/ProblemsPanel.cpp
SRCS += src/ui/ProjectBrowser.cpp
SRCS += src/ui/QuitAlert.cpp
SRCS += src/ui/SearchResultPanel.cpp
SRCS += src/ui/SearchResultTab.cpp
SRCS += src/ui/StyledItem.cpp
SRCS += src/ui/ToolBar.cpp
SRCS += src/ui/PanelTabManager.cpp
SRCS += src/editor/EditorTabView.cpp
SRCS += src/templates/IconMenuItem.cpp
SRCS += src/templates/TemplateManager.cpp
SRCS += src/templates/TemplatesMenu.cpp

RDEFS := Genio.rdef Spinner.rdef

LIBS  = be shared translation localestub $(STDCPPLIBS)
LIBS += columnlistview tracker
LIBS += git2
LIBS += src/scintilla/bin/libscintilla.a
LIBS += yaml-cpp
LIBS += editorconfig
LIBS += game

SYSTEM_INCLUDE_PATHS  = $(shell findpaths -e B_FIND_PATH_HEADERS_DIRECTORY private/interface)
SYSTEM_INCLUDE_PATHS += $(shell findpaths -e B_FIND_PATH_HEADERS_DIRECTORY private/shared)
SYSTEM_INCLUDE_PATHS += $(shell findpaths -e B_FIND_PATH_HEADERS_DIRECTORY private/storage)
SYSTEM_INCLUDE_PATHS += $(shell findpaths -e B_FIND_PATH_HEADERS_DIRECTORY private/support)
SYSTEM_INCLUDE_PATHS += $(shell findpaths -e B_FIND_PATH_HEADERS_DIRECTORY private/tracker)
SYSTEM_INCLUDE_PATHS += $(shell findpaths -e B_FIND_PATH_HEADERS_DIRECTORY private/locale)
SYSTEM_INCLUDE_PATHS += $(shell findpaths -a $(platform) -e B_FIND_PATH_HEADERS_DIRECTORY lexilla)
SYSTEM_INCLUDE_PATHS += src/scintilla/haiku
SYSTEM_INCLUDE_PATHS += src/scintilla/include


## clang build flag ############################################################
BUILD_WITH_CLANG ?= 0
################################################################################
ifeq ($(BUILD_WITH_CLANG), 1)
	# clang build
	CC  := clang
	CXX := clang++
	LD  := clang++
endif


LOCALES := ca de en_AU en_GB en es_419 es fr fur it nb tr

## Include the Makefile-Engine
BUILDHOME := \
	$(shell findpaths -r "makefile_engine" B_FIND_PATH_DEVELOP_DIRECTORY)
include $(BUILDHOME)/etc/makefile-engine

# Rules to compile the resource definition files.
# Taken from makefile_engine and removed  CFLAGS because
# clang doesn't like if we pass -std=c++20 to it
$(OBJ_DIR)/%.rsrc : %.rdef
	cat $< | $(CC) -E $(INCLUDES) - | grep -av '^#' | $(RESCOMP) -I $(dir $<) -o "$@" -
$(OBJ_DIR)/%.rsrc : %.RDEF
	cat $< | $(CC) -E $(INCLUDES)  - | grep -av '^#' | $(RESCOMP) -I $(dir $<) -o "$@" -

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

compiledb:
	compiledb make -Bnwk
