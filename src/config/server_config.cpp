#include "server_config.h"
#include <array>
#include <vector>
#include <iostream>
#include <random>

void ServerConfig::config_parser(int argc, char *argv[])
{
    //parse each argv string (starting from 1 since program name is i = 0)
    bool port_flag = 0;
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
        else if(str == "--port"){
            if( i + 1 >= argc ){
                std::cerr << "Config error: invalid " << str << " flag arguments" << std::endl;
                this->error = ServerConfigErrors::CONFIG_ERROR_INVALID_FLAG_ARG;
                break;
            }
            if (port_flag){
                std::cerr << "Config error: duplicate --port flag" << std::endl;
                this->error = ServerConfigErrors::CONFIG_ERROR_DUPLICATE_FLAGS;
                break;
            }
            else{
                try
                {
                    this->port = std::stoi(argv[i+1]);

                }
                catch(const std::exception& e)
                {
                    std::cerr << e.what() << '\n';
                    std::cerr << "Config error: invalid " << str << " flag arguments" << std::endl;
                    this->error = ServerConfigErrors::CONFIG_ERROR_INVALID_FLAG_ARG;
                }
                // indicate port is parsed
                port_flag = 1;
                // go to next flag
                i += 2;
                continue;
            }
        }
        else if(str == "--replicaof"){
            if (i + 1 > argc){
                std::cerr << "Config error: invalid " << str << " flag arguments" << std::endl;
                this->error = ServerConfigErrors::CONFIG_ERROR_INVALID_FLAG_ARG;
                break;
            }
            if(!this->replication_info.master_host.empty() || this->replication_info.master_port != 0){
                std::cerr << "Config error: duplicate --replicaof flag" << std::endl;
                this->error = ServerConfigErrors::CONFIG_ERROR_INVALID_FLAG_ARG;
                break;
            }
            std::string target (argv[i+1]);
            
            this->replication_info.master_host = target.substr(0,target.find(" "));
            std::cout << "Host is " << this->replication_info.master_host << std::endl;
            try
            {
                this->replication_info.master_port = std::stoi(target.substr(target.find(" ") + 2));
            }
            catch(const std::exception& e)
            {
                std::cerr << e.what() << '\n';
                std::cerr <<"Config error: invalid port number for master server" << std::endl;
                this->error = ServerConfigErrors::CONFIG_ERROR_INVALID_FLAG_ARG;
            }
            this->replication_info.role = "slave";
            i+=2;
            continue;
        }
        else{
            std::cerr << "Config error: invalid flags" << std::endl;
            this->error = ServerConfigErrors::CONFIG_ERROR_INVALID_FLAG;
            break;
        }       
    }
}

ServerConfig::ServerConfig(int argc, char* argv[]): 
dir(std::string()), dbfilename(std::string()), port(6379), 
replication_info()
{
    this->config_parser(argc, argv);
}

Replication::Replication():
role("master"),
master_port(0),
master_repl_offset(0)
{
    // simple random alphanumeric generator for the replid 
    static const char alphanum[] =
        "0123456789"
        "abcdefghijklmnopqrstuvwxyz";
    std::string replid;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, sizeof(alphanum) - 2);
    for (int i = 0; i < 40; ++i) {
        replid += alphanum[dis(gen)];
    }
    master_replid = replid;
}
