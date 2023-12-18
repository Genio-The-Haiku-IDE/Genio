/*
 * Copyright 2024, My Name <my@email.address>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include <InputServerFilter.h>
#include <Message.h>
#include <Messenger.h>
#include <String.h>
#include <Window.h>


class InputFilter : public BInputServerFilter
{
public:
							InputFilter();
	virtual 				~InputFilter();

	virtual status_t		InitCheck();
	virtual filter_result 	Filter(BMessage* message, BList* outList);

private:
			bool 			fInitScrolling;
			BPoint 			fCurrentMousePosition;
};


InputFilter::InputFilter()
{
}


InputFilter::~InputFilter()
{
}


status_t
InputFilter::InitCheck()
{
	return B_OK;
}


filter_result
InputFilter::Filter(BMessage* message, BList* outList)
{
	return B_DISPATCH_MESSAGE;
}


extern "C" BInputServerFilter*
instantiate_input_filter()
{
	return new(std::nothrow) InputFilter();
}
