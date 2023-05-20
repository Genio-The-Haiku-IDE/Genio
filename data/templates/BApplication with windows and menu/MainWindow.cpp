#include "MainWindow.h"

#include <Application.h>
#include <Menu.h>
#include <MenuItem.h>
#include <View.h>


enum Messages {
	MENU_FILE_NEW,
	MENU_FILE_OPEN
};

MainWindow::MainWindow(void)
	:	BWindow(BRect(100,100,500,400),"Main Window",B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
{
	BRect r(Bounds());
	r.bottom = 20;
	fMenuBar = new BMenuBar(r,"menubar");
	
	BMenu* fileMenu = new BMenu("File");
	fileMenu->AddItem(new BMenuItem("New", new BMessage(MENU_FILE_NEW), 'N'));
	fileMenu->AddItem(new BMenuItem("Open", new BMessage(MENU_FILE_OPEN), 'O'));
	
	fMenuBar->AddItem(fileMenu);
	
	AddChild(fMenuBar);
}


void
MainWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		case MENU_FILE_NEW:
		{
		}
		break;
		
		case MENU_FILE_OPEN:
		{
		}
		break;
		
		default:
		{
			BWindow::MessageReceived(msg);
			break;
		}
	}
}


bool
MainWindow::QuitRequested(void)
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}
