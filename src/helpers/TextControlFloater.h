// TextControlFloater.h
// * PURPOSE
//   Display an editable text field in a simple pop-up window
//   (which must automatically close when the user hits 'enter'
//   or the window loses focus).
// * TO DO +++++
//   escape key -> cancel
//
// * HISTORY
//   e.moon		23aug99		Begun

#ifndef __TextControlFloater_H__
#define __TextControlFloater_H__

#include <Messenger.h>
#include <Window.h>

class BTextControl;
class BFont;

//#include "cortex_defs.h"


class TextControlFloater: public BWindow {
	
	typedef	BWindow _inherited;
	
public:
	virtual 			~TextControlFloater();
						TextControlFloater(BRect frame, alignment align, const BFont* font,
											const char*	text, const BMessenger&	target,
											BMessage* message, BMessage* cancelMessage=0);

	virtual void 		WindowActivated(bool activated);

	virtual bool 		QuitRequested();
		
	virtual void 		MessageReceived(BMessage* message);
		
private:
	BTextControl* 		m_control;

	BMessenger			m_target;	
	const BMessage*		m_message;
	BMessage*			m_cancelMessage;
	
	// true if a message has been sent indicating the
	// user modified the text
	bool				m_sentUpdate;
};


#endif /*__TextControlFloater_H__*/