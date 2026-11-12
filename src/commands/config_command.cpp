#include "config_command.h"

std::string ConfigCommand::execute(const std::vector<std::string> &tokens_vector, InMemoryStore &storage, const ServerConfig &config)
{
    if (tokens_vector.size() < 2){
        return NOK;
    }
    auto sub_command = tokens_vector[1];
    std::transform(sub_command.begin(), sub_command.end(),
        sub_command.begin(), [](char c) { return std::toupper(c);});
    if (sub_command == "GET"){
        if (tokens_vector.size() <= 2){
            return NOK;
        }
        if (tokens_vector[2] == "dir"){
            return "*2\r\n" + static_cast<std::string>("$3\r\n") + "dir\r\n" + "$" + std::to_string(config.dir.size()) + "\r\n" + config.dir + "\r\n";
        }
        else if (tokens_vector[2] == "dbfilename"){
            return "*2\r\n" + static_cast<std::string>("$10\r\n") + "dbfilename\r\n" + "$" + std::to_string(config.dbfilename.size()) + "\r\n" + config.dbfilename + "\r\n"; 
        }
        else {
            return NOK;
        }
        
    }
    else{
        return NOK;
    }
}