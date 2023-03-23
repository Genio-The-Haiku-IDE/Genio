/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef Draggable_H
#define Draggable_H


#include <SupportDefs.h>
#include <stdio.h>

class Draggable {
public:
	Draggable() {};
	
	void OnMouseDown(BPoint where )
	{
		fDragging = false;
		fTryDrag = true;
		fDragPoint = where;
	}
	
	void OnMouseUp(BPoint where)
	{
		fTryDrag = false;
		fDragging = false;
		fDragPoint = BPoint(0, 0);
	}
	
	void OnMouseMoved(BPoint where)
	{
		if (fTryDrag) {
			// initiate a drag if the mouse was moved far enough
			BPoint offset = where - fDragPoint;
			float dragDistance = sqrtf(offset.x * offset.x + offset.y * offset.y);
			if (dragDistance >= 5.0f) {
				fTryDrag = false;
				fDragging = InitiateDrag(fDragPoint);
			}
		}
	}	
	virtual bool InitiateDrag(BPoint where)
	{
		return false;
	}
	
private:

	bool 	fTryDrag   = false;
	bool	fDragging  = false;
	BPoint	fDragPoint = BPoint(0, 0);

};


#endif // Draggable.h_H
