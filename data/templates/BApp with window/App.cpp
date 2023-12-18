/*
 * Copyright 2024, My Name <my@email.address>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "App.h"
#include "MainWindow.h"


const char* kApplicationSignature = "application/x-vnd.MyName-MyApp";


App::App()
	:
	BApplication(kApplicationSignature)
{
	MainWindow* mainwin = new MainWindow();
	mainwin->Show();
}


App::~App()
{
}


int
main()
{
	App* app = new App();
	app->Run();
	delete app;
	return 0;
}
