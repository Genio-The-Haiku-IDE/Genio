/*
 * Copyright 2024, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once


#include <Button.h>
#include <GridLayout.h>
#include <LayoutBuilder.h>
#include <MessageRunner.h>
#include <Size.h>

#include <cstdio>

#include "Draggable.h"
#include "TabButtons.h"
#include "TabsContainer.h"


class GTab;
class GTabDropZone : Draggable {
	public:
		 GTabDropZone() : fTabsContainer(nullptr)
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

		TabsContainer* Container() const { return fTabsContainer; }

		void SetContainer(TabsContainer* container) { fTabsContainer = container; }

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


class GTab : public BView, public GTabDropZone {
public:
								GTab(const char* label);
	virtual						~GTab();

			BSize				MinSize() override;
			BSize				MaxSize() override;

			void 				Draw(BRect updateRect) override;

	virtual void				DrawTab(BView* owener, BRect updateRect);
	virtual	void				DrawBackground(BView* owner, BRect frame,
									const BRect& updateRect, bool isFront);
	virtual	void				DrawContents(BView* owner, BRect frame,
									const BRect& updateRect, bool isFront);
	virtual	void				DrawLabel(BView* owner, BRect frame,
									const BRect& updateRect, bool isFront);

			void				MouseDown(BPoint where) override;
			void				MouseUp(BPoint where) override;
			void				MouseMoved(BPoint where, uint32 transit,
									const BMessage* dragMessage) override;

			void				MessageReceived(BMessage* message) override;

			bool 				InitiateDrag(BPoint where) override;

			void				SetIsFront(bool isFront);
			bool				IsFront() const;

			BLayoutItem*		LayoutItem() const { return fLayoutItem; }
			void				SetLayoutItem(BLayoutItem* layItem) { fLayoutItem = layItem; }

			BString				Label() const { return fLabel; };
	virtual void				SetLabel(const char* label) { fLabel.SetTo(label); }

			void 				OnDropMessage(BMessage* message) override;

protected:
			BLayoutItem*		fLayoutItem;
			bool				fIsFront;
			BString				fLabel;
};


class GTabCloseButton : public GTab {
public:
			enum { kTVCloseTab = 'TVCt' };

						GTabCloseButton(const char* label, const BHandler* handler);

			BSize		MinSize() override;
			BSize		MaxSize() override;
			void		DrawContents(BView* owner, BRect frame,
									const BRect& updateRect, bool isFront) override;
			void		MouseDown(BPoint where) override;
			void		MouseUp(BPoint where) override;
			void		MouseMoved(BPoint where, uint32 transit,
									const BMessage* dragMessage) override;

protected:

virtual		void		CloseButtonClicked();
			const BHandler* 	Handler() { return fHandler; }

private:
			void		DrawCloseButton(BView* owner, BRect butFrame, const BRect& updateRect,
										bool isFront);

			BRect		RectCloseButton();


private:
			bool fOverCloseRect;
			bool fClicked;
			const BHandler* fHandler;
};


class Filler : public BView, public GTabDropZone {
	public:
				Filler(TabsContainer* tabsContainer);

		BSize	MinSize() override
		{
			return BSize(0,0);
		}

		void 	Draw(BRect rect) override;

		void	MouseUp(BPoint where) override;
		void 	MouseMoved(BPoint where, uint32 transit,
							const BMessage* dragMessage) override;
		void	MessageReceived(BMessage* message) override;

		void	OnDropMessage(BMessage* message) override;

		void	OnDropObject();
};


class TabButtonDropZone : public GTabButton, public GTabDropZone {

	enum {
		kRunnerTick = 'RUNN'
	};

public:
	TabButtonDropZone(BMessage* message, TabsContainer* container)
		:
		GTabButton(" ", message),
		fRunner(nullptr)
	{
		SetContainer(container);
	}

	void Draw(BRect updateRect) override
	{
		GTabButton::Draw(updateRect);
		if (IsEnabled())
			DropZoneDraw(this, Bounds());
	}

	void MouseUp(BPoint where) override
	{
		DropZoneMouseUp(this, where);
		GTabButton::MouseUp(where);
	}

	void MouseMoved(BPoint where, uint32 transit, const BMessage* dragMessage) override
	{
		if (!DropZoneMouseMoved(this, where, transit, dragMessage))
			GTabButton::MouseMoved(where, transit, dragMessage);
	}

	void OnDropMessage(BMessage* message) override
	{
	}

	void MessageReceived(BMessage* message) override
	{
		switch (message->what) {
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
		}
	}

	void StopDragging(BView* view) override
	{
		GTabDropZone::StopDragging(view);
		if (fRunner != nullptr) {
			delete fRunner;
			fRunner = nullptr;
		}
	}

	void StartDragging(BView* view) override
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
	BMessageRunner*	fRunner;
};


class GTabScrollLeftButton : public TabButtonDropZone {
public:
	GTabScrollLeftButton(BMessage* message, TabsContainer* container)
		: TabButtonDropZone(message, container)
	{
	}

	void DrawSymbol(BRect frame, const BRect& updateRect,
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
