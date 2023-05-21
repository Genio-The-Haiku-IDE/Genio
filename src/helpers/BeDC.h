//////////////////////////////////////////////////////////////////////////
// BeDC.h                                                               //
// The header file to use with BeDC                                     //
//                                                                      //
// Written By: Magnus Landahl                                           //
// E-Mail: magnus.landahl@swipnet.se                                    //
//                                                                      //
// Copyright © Magnus Landahl - Stockholm 2000                          //
//////////////////////////////////////////////////////////////////////////					

#ifndef	_BE_DC_H_
#define _BE_DC_H_

//Messages types
#define DC_MESSAGE		 						 0
#define	DC_SUCCESS		 			DC_MESSAGE + 1
#define DC_ERROR		 			DC_MESSAGE + 2
#define DC_SEPARATOR				DC_MESSAGE + 3
#define DC_CLEAR					DC_MESSAGE + 4
//---------------

//Color defines
#define DEF_COLOR								 0
#define DC_WHITE					 DEF_COLOR + 1
#define DC_BLACK					 DEF_COLOR + 2
#define DC_BLUE						 DEF_COLOR + 3
#define DC_RED						 DEF_COLOR + 4
#define DC_YELLOW					 DEF_COLOR + 5
#define DC_GREEN					 DEF_COLOR + 6
//---------------

#define BEDC_MESSAGE						'bedc'

#define BEDC_MAIN_TYPE						 	 0
#define BEDC_BMESSAGE			BEDC_MAIN_TYPE + 1
#define BEDC_PLAIN_MESSAGE		BEDC_MAIN_TYPE + 2
#define BEDC_VER							 "1.0" 

#include <TypeConstants.h>
#include <SupportDefs.h>
#include <Messenger.h>
#include <Message.h>
#include <String.h>
#include <string.h>
#include <stdio.h>
#include <Point.h>
#include <Rect.h>

class BeDC
{
	////////////////////////////////////////////////
	//               Public Procedures

	public:
		BeDC(const char *c_name = "Application", int8 color = DEF_COLOR){
			name = strdup(c_name);
			s_color = color;
		}
		
		~BeDC(){
			delete name;
		}
		
		//Send a message
		void SendMessage(const char *text, int8 type = DC_MESSAGE){
			BMessage message(BEDC_MESSAGE);
			message.AddString(	"bedc_name", 	name);
			message.AddInt8(	"bedc_type", 	type);
			message.AddString(	"bedc_text", 	text);
			message.AddInt8(	"bedc_color",	s_color);
			message.AddString(	"bedc_ver",		BEDC_VER);
			message.AddInt8(	"bedc_main_type", BEDC_PLAIN_MESSAGE);
			SendMessageToApp(&message);
		}
		//---------------
		
		//Send a format string - parameters
		void SendFormat(const char *text, ...){ 
			char buffer[1024]; 
			va_list args; 
   
			va_start(args, text); 
			vsprintf(buffer, text, args); 
			va_end(args); 
			SendMessage(buffer, DC_MESSAGE);       
		} 
// Clang doesn't like this function, and we don't use it anyway
#if 0		
		void SendFormatT(const char *text, int8 type, ...){ 
			char buffer[1024]; 
			va_list args; 
 
			va_start(args, type); 
			vsprintf(buffer, text, args); 
			va_end(args); 
			SendMessage(buffer, type); 
		} 
		//---------------
#endif		
		//Dump a BMessage
		void DumpBMessage(BMessage *Message, const char *desc = "", int8 type = DC_MESSAGE){
			Message->AddInt32(	"bedc_what", Message->what);
			Message->AddInt8(	"bedc_main_type", BEDC_BMESSAGE);
			
			Message->AddInt8(	"bedc_type", type);
			Message->AddString(	"bedc_name", name);
			Message->AddString(	"bedc_desc", desc);
			Message->AddInt8(	"bedc_color",s_color);
			Message->AddString(	"bedc_ver",	 BEDC_VER);
			
			Message->what = BEDC_MESSAGE;
			SendMessageToApp(Message);	
		}
		//---------------				
		
		//Send a separator (Add a separator)
		void AddSeparator(){
			SendMessage("Separator", DC_SEPARATOR);
		}
		//---------------
		
		//Send a clear message (Clear the window that is online)
		void Clear(){
			SendMessage("Clear", DC_CLEAR);
		}
		//---------------
		
		//Send an Int32
		void SendInt(	int32 integer, const char *desc = NULL, int8 type = DC_MESSAGE){
			str.SetTo("");
			if (desc != NULL) 	str <<  desc; 	else
								str << "Integer: ";
			str << integer;
			SendMessage((char *)str.String(),type); 
		}
		//---------------
		
		//Send an Int64
		void SendInt(	int64 integer, const char *desc = NULL, int8 type = DC_MESSAGE){
			str.SetTo("");
			if (desc != NULL) 	str <<  desc; 	else
								str << "Integer: ";
			str << integer;
			SendMessage((char *)str.String(),type); 
		}
		//---------------
		
		//Send a BPoint
		void SendPoint(	BPoint point, const char *desc = NULL, int8 type = DC_MESSAGE){
			str.SetTo("");
			if (desc != NULL) 	str <<  desc; 	else
								str << "Point: ";
			str << "X = " << point.x  << " Y = " << point.y;
			SendMessage((char *)str.String(),type); 
		}
		//---------------
		
		//Send a BRect
		void SendRect(	BRect rect, const char *desc = NULL, int8 type = DC_MESSAGE){
			str.SetTo("");
			if (desc != NULL) 	str <<  desc; 	else
								str << "Rectangle: ";
			str << "Left = " << rect.left  << " Top = " << rect.top
				<< " Right = " << rect.right << " Bottom = " << rect.bottom;
			SendMessage((char *)str.String(),type); 	
		}
		//---------------
		
		////////////////////////////////////////////
		//               Private Data
	private:
		char *name;
		int8 s_color;
		BString str;
		
		void SendMessageToApp(BMessage *Msg){
			BMessenger messenger("application/x-vnd.ml-BeDCApp");
			if (messenger.IsValid()) messenger.SendMessage(Msg);
		}	
};

#endif
