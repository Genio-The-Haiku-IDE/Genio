#include <iostream>
#include <fstream>
#include <thread>
#include "FileWrapper.h"
#include<unistd.h> 

int main() {
	
	
	
	std::string lDir  = "/boot/data/genio/pipe/data/";
	

	std::string strRootUri = std::string("file://") + lDir ;    
   

	FileWrapper::Initialize(strRootUri.c_str());
    
    int res;
    while (scanf("%d", &res)) {

        if (res == 1) {
			
            break;
        }

		if (res == 2) {
			
			std::string file = strRootUri + "simple.cpp";
			FileWrapper	fW(file.c_str());
			fW.didOpen("");
			std::string content = "int x = 1";
			fW.didChange(content.c_str(), content.length(), 0, 0, 0, 0);
			sleep(1);
			fW.didChange("0", 1, 0, 9, 0, 9);
			sleep(1);
			fW.didChange(";", 1, 0, 10, 0, 10);
			sleep(1);
			fW.didChange("\n", 1, 0, 11, 0, 11);
			sleep(1);	
			fW.didChange("int y=1;", 8, 1, 0, 1, 0);
			sleep(1);		
			fW.didClose();
		}
    }
    
    FileWrapper::Dispose();
    
    return 0;
}
