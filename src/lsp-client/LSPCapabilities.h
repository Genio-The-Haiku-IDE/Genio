/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef LSPCapabilities_H
#define LSPCapabilities_H

enum LSPCapability {
	kLCapCompletion           = (1U),
	kLCapDocFormatting        = (1U << 1),
	kLCapDocRangeFormatting   = (1U << 2),
	kLCapDefinition           = (1U << 3),
	kLCapDeclaration          = (1U << 4),
	kLCapImplementation       = (1U << 5),
	kLCapDocLink              = (1U << 6),
	kLCapHover                = (1U << 7),
	kLCapSignatureHelp        = (1U << 8),
	kLCapDocumentSymbol		  = (1U << 0)
};

#define kMsgCapabilitiesUpdated 'CaUp'

#endif // LSPCapabilities_H
