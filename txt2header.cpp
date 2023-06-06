/*
 * Copyright 2016-2021 Stefano Ceccherini
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include <iostream>
#include <fstream>
#include <string>


static void
PrepareVariable(std::istream& inFile, std::ostream& outFile)
{
	try {
		std::string line;
		while (std::getline(inFile, line)) {
			std::string::iterator s;
			for (s = line.begin(); s != line.end(); s++) {
				unsigned char c = *s;
				// Skip CR/LF
				if (c != 0x0A && c != 0x09) {
					outFile << "'\\x" << std::hex << (unsigned)c << "'";
					outFile << ",";
					outFile << std::endl;
				}
			}
		}
	} catch (...) {
	}
	
	outFile << "'\\0'";
}


int main()
{
	std::istream& inputFile = std::cin;
	std::ostream& outputFile = std::cout;
		
	PrepareVariable(inputFile, outputFile);
}
