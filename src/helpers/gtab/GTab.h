/*
 * Copyright 2024, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once


#include <Button.h>
#include <GridLayout.h>
#include <LayoutBuilder.h>
#include <Rect.h>
#include <Size.h>
#include <SupportDefs.h>
#include <View.h>
#include <cstdio>
#include <typeinfo>
#include <MessageRunner.h>
#include "TabButtons.h"
#include "Draggable.h"
#include "TabsContainer.h"

class GTab;

class GTabDropZone : Draggable
{
	public:

		 GTabDropZone(TabsContainer* container) : fTabsContainer(container)
		 {
		 }

		virtual void DropZoneDraw(BView* view, BRect rect)
		{
			if (fTabDragging) {
				TabViewTools::DrawDroppingZone(view, rect);
			}
		}

		virtual void DropZoneMouseUp(BView* view, BPoint where)
		{
			Draggable::OnMouseUp(where);
			StopDragging(view);
		}

		virtual void DropZoneMouseDown(BPoint where)
		{
			Draggable::OnMouseDown(where);
		}

		virtual bool DropZoneMouseMoved(BView* view, BPoint where, uint32 transit,
									const BMessage* dragMessage);


		virtual bool DropZoneMessageReceived(BMessage* message);

		virtual void OnDropMessage(BMessage* message) = 0;

		TabsContainer* 		Container() { return fTabsContainer; }

		virtual void StopDragging(BView* view)
		{
			if (fTabDragging) {
				fTabDragging = false;
				view->Invalidate();
			}
		}

		virtual void StartDragging(BView* view)
		{
			fTabDragging = true;
			view->Invalidate();
		}

		bool	_ValidDragAndDrop(const BMessage* message);

		bool				fTabDragging = false;
		TabsContainer*		fTabsContainer;

};

class GTab : public BView , public GTabDropZone {
public:
								GTab(const char* label, TabsContainer* container);
	virtual						~GTab();

	virtual	BSize				MinSize() override;
	virtual	BSize				MaxSize() override;

			void 				Draw(BRect updateRect) override;

	virtual void				DrawTab(BView* owener, BRect updateRect);
	virtual	void				DrawBackground(BView* owner, BRect frame,
									const BRect& updateRect, bool isFront);
	virtual	void				DrawContents(BView* owner, BRect frame,
									const BRect& updateRect, bool isFront);

	virtual	void				MouseDown(BPoint where) override;
	virtual	void				MouseUp(BPoint where) override;
	virtual	void				MouseMoved(BPoint where, uint32 transit,
									const BMessage* dragMessage) override;

	virtual	void				MessageReceived(BMessage* message) override;

			bool 				InitiateDrag(BPoint where) override;

			void				SetIsFront(bool isFront);
			bool				IsFront() const;

			BLayoutItem*		LayoutItem() const { return fLayoutItem; }
			void				SetLayoutItem(BLayoutItem* layItem) { fLayoutItem = layItem; }

			BString				Label() { return fLabel; };
				void			SetLabel(const char* label) { fLabel.SetTo(label); }

	virtual void 				OnDropMessage(BMessage* message);

protected:

			BLayoutItem*		fLayoutItem;
			bool				fIsFront;
			BString				fLabel;
};

class GTabCloseButton : public GTab {
public:

							GTabCloseButton(const char* label,
												TabsContainer* controller,
												const BHandler* handler);

		virtual	BSize		MinSize() override;
		virtual	BSize		MaxSize() override;
		virtual	void		DrawContents(BView* owner, BRect frame,
										const BRect& updateRect, bool isFront) override;
		virtual	void		MouseDown(BPoint where) override;
		virtual	void		MouseUp(BPoint where) override;
		virtual	void		MouseMoved(BPoint where, uint32 transit,
										const BMessage* dragMessage) override;
private:
				void		DrawCloseButton(BView* owner, BRect butFrame, const BRect& updateRect,
											bool isFront);

				BRect		RectCloseButton();

				void		CloseButtonClicked();
private:

				bool fOverCloseRect;
				bool fClicked;
				const BHandler* fHandler;
};

class Filler : public BView, public GTabDropZone
{
	public:
				Filler(TabsContainer* tabsContainer);

		BSize	MinSize() override
		{
			return BSize(0,0);
		}

		void 	Draw(BRect rect) override;

		void	MouseUp(BPoint where) override;


		void	MessageReceived(BMessage* message) override;


		void 	MouseMoved(BPoint where, uint32 transit,
									const BMessage* dragMessage) override;

		void	OnDropMessage(BMessage* message) override;

		void	OnDropObject();

};


class TabButtonDropZone : public GTabButton, public GTabDropZone {

	enum {
		kRunnerTick = 'RUNN'
	};

public:
	TabButtonDropZone(BMessage* message, TabsContainer* container)
		: GTabButton(" ", message), GTabDropZone(container), fRunner(nullptr)
	{
	}

	virtual void Draw(BRect updateRect) override
	{
		GTabButton::Draw(updateRect);
		if (IsEnabled())
			DropZoneDraw(this, Bounds());
	}

	virtual void MouseUp(BPoint where) override
	{
		DropZoneMouseUp(this, where);
		GTabButton::MouseUp(where);
	}

	virtual void MouseMoved(BPoint where, uint32 transit, const BMessage* dragMessage) override
	{
		if (DropZoneMouseMoved(this, where, transit, dragMessage) == false)
			GTabButton::MouseMoved(where, transit, dragMessage);
	}


	virtual	void OnDropMessage(BMessage* message) override
	{
		return;
	}

	virtual void MessageReceived(BMessage* message) override
	{
		switch(message->what) {
			case kRunnerTick:
				if (fRunner != nullptr && IsEnabled()) {
						Invoke();
				} else {
					if (fRunner != nullptr) {
						delete fRunner;
						fRunner = nullptr;
					}
				}
			break;
			default:
				GTabButton::MessageReceived(message);
			break;

		};
	}

	virtual void StopDragging(BView* view) override
	{
		GTabDropZone::StopDragging(view);
		if (fRunner != nullptr) {
			delete fRunner;
			fRunner = nullptr;
		}
	}

	virtual void StartDragging(BView* view) override
	{
		GTabDropZone::StartDragging(view);

		// create a message to update the project
		if (fRunner == nullptr) {
			BMessage message(kRunnerTick);
			fRunner = new BMessageRunner(BMessenger(this), &message, 500000);
			if (fRunner->InitCheck() != B_OK) {
				if (fRunner != nullptr) {
					delete fRunner;
					fRunner = nullptr;
				}
			}
		}
	}

private:
	bool			fTabDragging;
	BMessageRunner*	fRunner;
};


class GTabScrollLeftButton : public TabButtonDropZone {
public:
	GTabScrollLeftButton(BMessage* message, TabsContainer* container)
		: TabButtonDropZone(message, container)
	{
	}

	virtual void DrawSymbol(BRect frame, const BRect& updateRect,
		const rgb_color& base) override
	{
		float tint = IsEnabled() ? B_DARKEN_4_TINT : B_DARKEN_1_TINT;
		be_control_look->DrawArrowShape(this, frame, updateRect,
			base, BControlLook::B_LEFT_ARROW, 0, tint);
	}
};


class GTabScrollRightButton : public TabButtonDropZone {
public:
	GTabScrollRightButton(BMessage* message, TabsContainer* container)
		: TabButtonDropZone(message, container)
	{
	}

	virtual void DrawSymbol(BRect frame, const BRect& updateRect,
		const rgb_color& base)
	{
		frame.OffsetBy(1, 0);
		float tint = IsEnabled() ? B_DARKEN_4_TINT : B_DARKEN_1_TINT;
		be_control_look->DrawArrowShape(this, frame, updateRect,
			base, BControlLook::B_RIGHT_ARROW, 0, tint);
	}
};

