/*
 * Copyright 2002-2012, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Vlad Slepukhin
 *		Siarzhuk Zharski
 *
 * Copied from Haiku commit a609673ce8c942d91e14f24d1d8832951ab27964.
 * Modifications:
 * Copyright 2018-2019 Kacper Kasper 
 * Distributed under the terms of the MIT License.
 */
#ifndef STATUS_VIEW_H
#define STATUS_VIEW_H


#include <Entry.h>
#include <String.h>
#include <View.h>


class BScrollView;


namespace controls {

class StatusView : public BView {
public:
							StatusView(BScrollView* scrollView);
							~StatusView();

	virtual	void			AttachedToWindow();
	virtual void			GetPreferredSize(float* _width, float* _height);
	virtual	void			ResizeToPreferred();
	virtual	void			WindowActivated(bool active);

protected:
	virtual	float			Width() = 0;
			BScrollView*	ScrollView();

private:
			void			_ValidatePreferredSize();

			BScrollView*	fScrollView;
			BSize			fPreferredSize;
};

} // namespace controls

#endif  // STATUS_VIEW_H
