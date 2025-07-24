#include "server_config.h"
#include <array>
#include <vector>
#include <iostream>

void ServerConfig::config_parser(int argc, char *argv[])
{
    //parse each argv string (starting from 1 since program name is i = 0)
    for (int i = 1; i < argc;){
        auto str = std::string(argv[i]);
        if (str == "--dir"){
            if( i + 1 >= argc ){
                std::cerr << "Config error: invalid " << str << " flag arguments" << std::endl;
                this->error = ServerConfigErrors::CONFIG_ERROR_INVALID_FLAG_ARG;
                break;
            }
            if (!this->dir.empty()){
                std::cerr << "Config error: duplicate --dir flag" << std::endl;
                this->error = ServerConfigErrors::CONFIG_ERROR_DUPLICATE_FLAGS;
                break;
            }
            else{
                this->dir = argv[i+1];
                // go to next flag
                i += 2;
                continue;
            }
        }
        else if(str == "--dbfilename"){
            if( i + 1 >= argc ){
                std::cerr << "Config error: invalid " << str << " flag arguments" << std::endl;
                this->error = ServerConfigErrors::CONFIG_ERROR_INVALID_FLAG_ARG;
                break;
            }
            if (!this->dbfilename.empty()){
                std::cerr << "Config error: duplicate --dbfilename flag" << std::endl;
                break;
            }
            else{
                this->dbfilename = argv[i+1];
                // go to next flag
                i += 2;
                continue;
            }    
        }
        else{
            std::cerr << "Config error: invalid flags" << std::endl;
            this->error = ServerConfigErrors::CONFIG_ERROR_INVALID_FLAG;
            break;
        }       
    }
}

ServerConfig::ServerConfig(int argc, char* argv[]): dir(std::string()), dbfilename(std::string())
{
    this->config_parser(argc, argv);
}
