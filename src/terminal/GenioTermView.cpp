/*
 * Copyright 2025, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "GenioTermView.h"
#include "ShellInfo.h"
#include "TermView.h"
#include <ControlLook.h>
#include <Messenger.h>
#include <cstdio>
#include "PrefHandler.h"
#include "Colors.h"

const static int32 kTermViewOffset = 3;
extern BObjectList<const color_scheme, true> *gColorSchemes;


class GenioTermViewContainerView : public BView {
public:
	GenioTermViewContainerView(TermView* termView)
		:
		BView(BRect(), "term view container", B_FOLLOW_ALL, 0),
		fTermView(termView)
	{
		termView->MoveTo(kTermViewOffset, kTermViewOffset);
		BRect frame(termView->Frame());
		ResizeTo(frame.right + kTermViewOffset, frame.bottom + kTermViewOffset);
		AddChild(termView);
		termView->SetResizingMode(B_FOLLOW_ALL);
	}

	virtual void GetPreferredSize(float* _width, float* _height)
	{
		float width, height;
		fTermView->GetPreferredSize(&width, &height);
		*_width = width + 2 * kTermViewOffset;
		*_height = height + 2 * kTermViewOffset;
	}

	TermView*	GetTermView() { return fTermView; }

	void DefaultColors()
	{
		PrefHandler* handler = PrefHandler::Default();
		rgb_color background = handler->getRGB(PREF_TEXT_BACK_COLOR);

		SetViewColor(background);

		TermView *termView = GetTermView();
		termView->SetTextColor(handler->getRGB(PREF_TEXT_FORE_COLOR), background);

		termView->SetCursorColor(handler->getRGB(PREF_CURSOR_FORE_COLOR),
			handler->getRGB(PREF_CURSOR_BACK_COLOR));
		termView->SetSelectColor(handler->getRGB(PREF_SELECT_FORE_COLOR),
			handler->getRGB(PREF_SELECT_BACK_COLOR));

		// taken from TermApp::_InitDefaultPalette()
		const char * keys[kANSIColorCount] = {
			PREF_ANSI_BLACK_COLOR,
			PREF_ANSI_RED_COLOR,
			PREF_ANSI_GREEN_COLOR,
			PREF_ANSI_YELLOW_COLOR,
			PREF_ANSI_BLUE_COLOR,
			PREF_ANSI_MAGENTA_COLOR,
			PREF_ANSI_CYAN_COLOR,
			PREF_ANSI_WHITE_COLOR,
			PREF_ANSI_BLACK_HCOLOR,
			PREF_ANSI_RED_HCOLOR,
			PREF_ANSI_GREEN_HCOLOR,
			PREF_ANSI_YELLOW_HCOLOR,
			PREF_ANSI_BLUE_HCOLOR,
			PREF_ANSI_MAGENTA_HCOLOR,
			PREF_ANSI_CYAN_HCOLOR,
			PREF_ANSI_WHITE_HCOLOR
		};

		for (uint i = 0; i < kANSIColorCount; i++)
			termView->SetTermColor(i, handler->getRGB(keys[i]), false);
	}
private:
	TermView*	fTermView;
};

class GenioListener : public TermView::Listener {
public:
  GenioListener(BMessenger &msg) : fMessenger(msg) {};

  virtual ~GenioListener() {};

  // all hooks called in the window thread
  virtual void NotifyTermViewQuit(TermView *view, int32 reason) {
    BMessage msg('NOTM');
	msg.AddInt32("reason", reason);
	msg.AddPointer("term_view", view);
	ShellInfo sinfo;
	if (view->GetShellInfo(sinfo)) {
		msg.AddInt32("pid", sinfo.ProcessID());
	}
	fMessenger.SendMessage(&msg);
	TermView::Listener::NotifyTermViewQuit(view, reason);
  }

  BMessenger fMessenger;
};

class WrapTermView : public TermView
{
public:
	WrapTermView(BMessage* data): TermView(data)
	{
	}

	void	FrameResized(float width, float height)
	{
		DisableResizeView();
		TermView::FrameResized(width, height);
	}

	void	MessageReceived(BMessage* msg)
	{
		switch(msg->what) {
			case 'clea':
				TermView::Clear();
			break;
/*			case 'teme':
				if (gColorSchemes == nullptr ||
					gColorSchemes->CountItems() == 0)
					return;

				for (int32 i=0;i<gColorSchemes->CountItems();i++) {
					printf("%d %s\n", i , gColorSchemes->ItemAt(i)->name);
				}
				msg->SendReply('teme');
			break;*/
/*
	PrefHandler* pref = PrefHandler::Default();

	pref->setRGB(PREF_TEXT_FORE_COLOR, scheme->text_fore_color);
	pref->setRGB(PREF_TEXT_BACK_COLOR, scheme->text_back_color);
	pref->setRGB(PREF_SELECT_FORE_COLOR, scheme->select_fore_color);
	pref->setRGB(PREF_SELECT_BACK_COLOR, scheme->select_back_color);
	pref->setRGB(PREF_CURSOR_FORE_COLOR, scheme->cursor_fore_color);
	pref->setRGB(PREF_CURSOR_BACK_COLOR, scheme->cursor_back_color);
	pref->setRGB(PREF_ANSI_BLACK_COLOR, scheme->ansi_colors.black);
	pref->setRGB(PREF_ANSI_RED_COLOR, scheme->ansi_colors.red);
	pref->setRGB(PREF_ANSI_GREEN_COLOR, scheme->ansi_colors.green);
	pref->setRGB(PREF_ANSI_YELLOW_COLOR, scheme->ansi_colors.yellow);
	pref->setRGB(PREF_ANSI_BLUE_COLOR, scheme->ansi_colors.blue);
	pref->setRGB(PREF_ANSI_MAGENTA_COLOR, scheme->ansi_colors.magenta);
	pref->setRGB(PREF_ANSI_CYAN_COLOR, scheme->ansi_colors.cyan);
	pref->setRGB(PREF_ANSI_WHITE_COLOR, scheme->ansi_colors.white);
	pref->setRGB(PREF_ANSI_BLACK_HCOLOR, scheme->ansi_colors_h.black);
	pref->setRGB(PREF_ANSI_RED_HCOLOR, scheme->ansi_colors_h.red);
	pref->setRGB(PREF_ANSI_GREEN_HCOLOR, scheme->ansi_colors_h.green);
	pref->setRGB(PREF_ANSI_YELLOW_HCOLOR, scheme->ansi_colors_h.yellow);
	pref->setRGB(PREF_ANSI_BLUE_HCOLOR, scheme->ansi_colors_h.blue);
	pref->setRGB(PREF_ANSI_MAGENTA_HCOLOR, scheme->ansi_colors_h.magenta);
	pref->setRGB(PREF_ANSI_CYAN_HCOLOR, scheme->ansi_colors_h.cyan);
	pref->setRGB(PREF_ANSI_WHITE_HCOLOR, scheme->ansi_colors_h.white);
*/
			default:
				TermView::MessageReceived(msg);
		};
	}

};

/*static*/
BArchivable*
GenioTermView::Instantiate(BMessage* data)
{
	if (validate_instantiation(data, "GenioTermView")) {
		TermView *view = new (std::nothrow) WrapTermView(data);
		BMessenger messenger;
		if (data->FindMessenger("listener", &messenger) == B_OK) {
			view->SetListener(new GenioListener(messenger));
		}
		GenioTermViewContainerView* containerView = new (std::nothrow) GenioTermViewContainerView(view);
		TermScrollView* scrollView = new (std::nothrow) TermScrollView("scrollView",
			containerView, view, true);

		scrollView->ScrollBar(B_VERTICAL)
				->ResizeBy(0, -(be_control_look->GetScrollBarWidth(B_VERTICAL) - 1));


		containerView->DefaultColors();

		return scrollView;
	}

	return NULL;
}



