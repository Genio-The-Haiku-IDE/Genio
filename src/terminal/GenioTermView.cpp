/*
 * Copyright 2025, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "GenioTermView.h"
#include "TermView.h"
#include <ControlLook.h>
#include <cstdio>
#include <Messenger.h>

const static int32 kTermViewOffset = 3;

class GenioTermViewContainerView : public BView {
public:
	GenioTermViewContainerView(BView* termView)
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

private:
	BView*	fTermView;
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
	fMessenger.SendMessage(&msg);
	TermView::Listener::NotifyTermViewQuit(view, reason);
  }

  BMessenger fMessenger;
};

/*static*/
BArchivable*
GenioTermView::Instantiate(BMessage* data)
{
	if (validate_instantiation(data, "GenioTermView")) {
		TermView *view = new (std::nothrow) TermView(data);
		BMessenger messenger;
		if (data->FindMessenger("listener", &messenger) == B_OK) {
			view->SetListener(new GenioListener(messenger));
		}
		GenioTermViewContainerView* containerView = new (std::nothrow) GenioTermViewContainerView(view);
		BScrollView* scrollView = new (std::nothrow) TermScrollView("scrollView",
			containerView, view, true);

		scrollView->ScrollBar(B_VERTICAL)
				->ResizeBy(0, -(be_control_look->GetScrollBarWidth(B_VERTICAL) - 1));

		return scrollView;
	}

	return NULL;
}
