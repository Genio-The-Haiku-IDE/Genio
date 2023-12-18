/*
 * Copyright 2024, My Name <my@email.address>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "MainWindow.h"

#include <Application.h>


MainWindow::MainWindow()
	:
	BWindow(BRect(100, 100, 500, 400), "My window", B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE)
{
}


MainWindow::~MainWindow()
{
}


void
MainWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		default:
		{
			BWindow::MessageReceived(message);
			break;
		}
	}
}


bool
MainWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}
