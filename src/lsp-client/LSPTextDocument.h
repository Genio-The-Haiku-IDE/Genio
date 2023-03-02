/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
 
#ifndef LSPTextDocument_H
#define LSPTextDocument_H

#include "MessageHandler.h"


class LSPTextDocument : public MessageHandler {
public:
    LSPTextDocument(std::string fileURI) { fFilenameURI = fileURI; }
    
    std::string	GetFilenameURI() { return fFilenameURI; }

protected:

	std::string fFilenameURI;


};


#endif // LSPTextDocument_H
