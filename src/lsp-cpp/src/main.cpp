#include <iostream>
#include <fstream>
#include <thread>
#include "client.h"


int main() {
	
	bool initialized = false;
	
	std::string lDir  = "/boot/data/genio/pipe/data/";
	std::string lFile = "app.cpp";

	std::string strRootUri = std::string("file://") + lDir ;
	std::string fullFile = std::string("file://") + lDir + lFile;
	
	string_ref rootUri = strRootUri.c_str();
    string_ref file    = fullFile.c_str();
    
	ProcessLanguageClient client("clangd");
	
    MapMessageHandler my;
    
     
    std::thread thread([&] {
        client.loop(my);
    });
	my.bindResponse("initialize", [&](json& j){
		
		std::ifstream f((lDir + lFile).c_str());
		std::stringstream buffer;
		buffer << f.rdbuf();
		fprintf(stderr, "********* INIT ******\n"); 
		client.DidOpen(file, buffer.str());
        client.Sync();
        initialized = true;
	});
    
	client.Initialize(rootUri);
    
    int res;
    while (scanf("%d", &res)) {
    	if (!initialized)
    		continue;

        if (res == 1) {
            client.Exit();
            thread.detach();
            return 0;
        }
//        if (res == 2) {
//            client.Initialize();
//        }
//        if (res == 3) {
//            client.DidOpen(file, text);
//            client.Sync();
//        }
        if (res == 4) {
            client.Formatting(file);
        }
		if (res == 5) {
			Position pos;
			pos.line = 2;
			pos.character = 4;
			CompletionContext context;

			client.Completion(file, pos, context);
		}
    }
    return 0;
}
