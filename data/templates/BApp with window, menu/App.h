/*
 * Copyright 2024, My Name 
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef APP_H
#define APP_H


#include <Application.h>


class App : public BApplication
{
public:
							App();
	virtual					~App();

	virtual void			AboutRequested();

private:
};

#endif // APP_H

