/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef DiagnosticsPanel_H
#define DiagnosticsPanel_H


#include <SupportDefs.h>
#include <ColumnListView.h>


class DiagnosticsPanel : public BColumnListView {
public:

		DiagnosticsPanel();
		
		void UpdateDiagnostics(BMessage* msg);
		
		virtual void MessageReceived(BMessage* msg);
		virtual void	AttachedToWindow();

private:

};


#endif // DiagnosticsPanel_H
