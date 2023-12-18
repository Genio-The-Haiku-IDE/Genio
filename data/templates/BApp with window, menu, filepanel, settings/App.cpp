/*
 * Copyright 2024, My Name 
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "App.h"
#include "MainWindow.h"

#include <AboutWindow.h>
#include <Catalog.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Application"

const char* kApplicationSignature = "application/x-vnd.MyName-MyApp";


App::App()
	:
	BApplication(kApplicationSignature)
{
	MainWindow* mainWindow = new MainWindow();
	mainWindow->Show();
}


App::~App()
{
}


void
App::AboutRequested()
{
	BAboutWindow* about
		= new BAboutWindow(B_TRANSLATE_SYSTEM_NAME("MyApp"), kApplicationSignature);
	about->AddDescription(B_TRANSLATE("A nice description of this application."));
	about->AddCopyright(2024, "My name");
	about->Show();
}


int
main()
{
	App* app = new App();
	app->Run();
	delete app;
	return 0;
}
