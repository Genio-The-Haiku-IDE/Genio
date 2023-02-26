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

ifeq ($(BUILD_WITH_CLANG), 0)		# gcc build
  ifeq ($(platform), x86)			# x86
	CC   := gcc-x86
	CXX  := g++-x86
   endif
else								# clang build
	CC  := clang
	CXX := clang++
	LD  := clang++
	ifeq ($(platform), x86)			# x86
		INCLUDE_PATH_HACK  :=  $(shell gcc-x86 --version | grep ^gcc | sed 's/^.* //g')
	endif
	ifeq ($(platform), x86_64)		# x86_64
		INCLUDE_PATH_HACK  :=  $(shell gcc --version | grep ^gcc | sed 's/^.* //g')
	endif 
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
SRCS +=  src/ui/Editor.cpp
SRCS +=  src/ui/GenioWindow.cpp
SRCS +=  src/ui/SettingsWindow.cpp
SRCS +=  src/project/AddToProjectWindow.cpp
SRCS +=  src/project/NewProjectWindow.cpp
SRCS +=  src/project/Project.cpp
SRCS +=  src/project/ProjectParser.cpp
SRCS +=  src/project/ProjectSettingsWindow.cpp
SRCS +=  src/project/ProjectFolder.cpp
SRCS +=  src/project/ProjectItem.cpp
SRCS +=  src/helpers/GenioCommon.cpp
SRCS +=  src/helpers/TPreferences.cpp
SRCS +=  src/helpers/GSettings.cpp
SRCS +=  src/helpers/console_io/ConsoleIOView.cpp
SRCS +=  src/helpers/console_io/ConsoleIOThread.cpp
SRCS +=  src/helpers/console_io/GenericThread.cpp
SRCS +=  src/helpers/tabview/TabContainerView.cpp
SRCS +=  src/helpers/tabview/TabManager.cpp
SRCS +=  src/helpers/tabview/TabView.cpp
SRCS +=  src/helpers/Logger.cpp
SRCS += src/helpers/console_io/WordTextView.cpp


RDEFS := Genio.rdef

LIBS = be shared translation localestub $(STDCPPLIBS)
LIBS += scintilla columnlistview tracker

# LIBPATHS = $(shell findpaths -a $(platform) B_FIND_PATH_DEVELOP_LIB_DIRECTORY)
# LIBPATHS  = /boot/home/config/non-packaged/lib
# $(info LIBPATHS="$(LIBPATHS)")

SYSTEM_INCLUDE_PATHS  = $(shell findpaths -e B_FIND_PATH_HEADERS_DIRECTORY private/interface)
SYSTEM_INCLUDE_PATHS +=	$(shell findpaths -e B_FIND_PATH_HEADERS_DIRECTORY private/shared)
SYSTEM_INCLUDE_PATHS +=	$(shell findpaths -a $(platform) -e B_FIND_PATH_HEADERS_DIRECTORY scintilla)


################################################################################
## clang++ headers hack
ifneq ($(BUILD_WITH_CLANG), 0)

TOOLS_PATH := $(shell findpaths -e B_FIND_PATH_DEVELOP_DIRECTORY tools)

ifeq ($(platform), x86)
###### x86 clang++ build (mind scan-build too) #################################
SYSTEM_INCLUDE_PATHS +=  \
	$(TOOLS_PATH)/x86/lib/gcc/i586-pc-haiku/$(INCLUDE_PATH_HACK)/include/c++ \
	$(TOOLS_PATH)/x86/lib/gcc/i586-pc-haiku/$(INCLUDE_PATH_HACK)/include/c++/i586-pc-haiku
endif
ifeq ($(platform), x86_64)
######## x86_64 clang++ build (mind scan-build too) ############################
SYSTEM_INCLUDE_PATHS += \
	$(TOOLS_PATH)/lib/gcc/x86_64-unknown-haiku/$(INCLUDE_PATH_HACK)/include/c++ \
	$(TOOLS_PATH)/lib/gcc/x86_64-unknown-haiku/$(INCLUDE_PATH_HACK)/include/c++/x86_64-unknown-haiku
endif
endif


CFLAGS := -Wall -Werror

CXXFLAGS := -std=c++17 -fPIC

LOCALES := en it

## Include the Makefile-Engine
ENGINE_DIRECTORY := $(shell findpaths -r "makefile_engine" B_FIND_PATH_DEVELOP_DIRECTORY)
include $(ENGINE_DIRECTORY)/etc/makefile-engine

## CXXFLAGS rule
$(OBJ_DIR)/%.o : %.cpp
	$(CXX) -c $< $(INCLUDES) $(CFLAGS) $(CXXFLAGS) -o "$@"

