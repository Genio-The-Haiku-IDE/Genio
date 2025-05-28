/*
 * Copyright 2025, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "GenioTermView.h"

#include <ControlLook.h>
#include <Messenger.h>

#include "Colors.h"
#include "PrefHandler.h"
#include "ShellInfo.h"
#include "TermScrollView.h"
#include "TermView.h"


const static int32 kTermViewOffset = 3;
#if B_HAIKU_VERSION > B_HAIKU_VERSION_1_BETA_5
extern BObjectList<const color_scheme, true> *gColorSchemes;
#else
extern BObjectList<const color_scheme> *gColorSchemes;
#endif


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

	void GetPreferredSize(float* _width, float* _height) override
	{
		float width, height;
		fTermView->GetPreferredSize(&width, &height);
		*_width = width + 2 * kTermViewOffset;
		*_height = height + 2 * kTermViewOffset;
	}

	TermView* GetTermView() const { return fTermView; }

	void DefaultSettings()
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

		for (uint i = 0; i < kANSIColorCount; i++) {
			termView->SetTermColor(i, handler->getRGB(keys[i]), false);
		}

		uint colorIndex = kANSIColorCount;
		// 16 - 231 are 6x6x6 color "cubes" in xterm color model
		for (uint red = 0; red < 256; red += (red == 0) ? 95 : 40)
			for (uint green = 0; green < 256; green += (green == 0) ? 95 : 40)
				for (uint blue = 0; blue < 256; blue += (blue == 0) ? 95 : 40) {
					termView->SetTermColor(colorIndex++,
											rgb_color{static_cast<uint8>(red),
													  static_cast<uint8>(green),
													  static_cast<uint8>(blue)}, false);
				}

		// 232 - 255 are grayscale ramp in xterm color model
		for (uint gray = 8; gray < 240; gray += 10)
			termView->SetTermColor(colorIndex++,
									rgb_color{static_cast<uint8>(gray),
											  static_cast<uint8>(gray),
											  static_cast<uint8>(gray)}, false);

		// Setting font
		font_family family;
		font_style style;
		const char *prefFamily = handler->getString(PREF_HALF_FONT_FAMILY);
		const int32 familiesCount = (prefFamily != NULL) ? count_font_families() : 0;
		for (int32 i = 0; i < familiesCount; i++) {
			if (get_font_family(i, &family) != B_OK
				|| strcmp(family, prefFamily) != 0)
				continue;

			const char *prefStyle = handler->getString(PREF_HALF_FONT_STYLE);
			const int32 stylesCount = (prefStyle != NULL) ? count_font_styles(family) : 0;
			for (int32 j = 0; j < stylesCount; j++) {
				// check style if we can safely use this font
				if (get_font_style(family, j, &style) == B_OK
						&& strcmp(style, prefStyle) == 0) {
					const char* size = handler->getString(PREF_HALF_FONT_SIZE);
					BFont font;
					font.SetFamilyAndStyle(family, style);
					font.SetSize(::atof(size));
					termView->SetTermFont(&font);
				}
			}
		}
	}
private:
	TermView* fTermView;
};


class GenioListener : public TermView::Listener {
public:
	GenioListener(BMessenger &msg)
		:
		fMessenger(msg)
	{
	}

	virtual ~GenioListener()
	{
	}

	// all hooks called in the window thread
	void NotifyTermViewQuit(TermView *view, int32 reason) override
	{
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


class WrapTermView : public TermView {
public:
	WrapTermView(BMessage* data)
		:
		TermView(data)
	{
	}

	void FrameResized(float width, float height) override
	{
		DisableResizeView();
		TermView::FrameResized(width, height);
	}

	void MessageReceived(BMessage* msg) override
	{
		switch(msg->what) {
			case TERMVIEW_CLEAR:
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
				break;
		}
	}
};


/* static */
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

		containerView->DefaultSettings();

		return scrollView;
	}

	return NULL;
}
