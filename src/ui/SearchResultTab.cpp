/*
 * Copyright 2024, Andrea Anzani 
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "SearchResultTab.h"
#include "SearchResultPanel.h"
#include <LayoutBuilder.h>

SearchResultTab::SearchResultTab(BTabView* tabView)
	: BGroupView(B_VERTICAL, 0.0f)
	, fSearchResultPanel(nullptr)
	, fTabView(tabView)
{
	fSearchResultPanel = new SearchResultPanel(fTabView);
	BLayoutBuilder::Group<>(this, B_HORIZONTAL, 0.0f)
		.Add(fSearchResultPanel, 3.0f)
		.AddGroup(B_VERTICAL, 0.0f)
			.SetInsets(B_USE_SMALL_SPACING)
			/*.Add(fStdoutEnabled)
			.Add(fStderrEnabled)
			.Add(fWrapEnabled)
			.Add(fBannerEnabled)*/
			.AddGlue()
			/*.Add(fClearButton)
			.Add(fStopButton)*/
		.End()
	.End();
}

void
SearchResultTab::StartSearch(BString command, BString projectPath)
{
	fSearchResultPanel->StartSearch(command, projectPath);
}
