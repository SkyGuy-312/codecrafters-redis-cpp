#include "info_command.h"
#include <iostream>
std::string InfoCommand::execute(const std::vector<std::string> &tokens_vector, InMemoryStore &storage, const ServerConfig &config)
{
    if (tokens_vector.size() != 2){
        return NOK;
    }
    auto sub_command = tokens_vector[1];

    std::transform(sub_command.begin(), sub_command.end(),
        sub_command.begin(), [](char c) { return std::toupper(c);});
    if (sub_command == "REPLICATION"){
        std::string str = "# Replication\r\n" 
                            "role:" + config.replication_info.role + "\r\n" +
                            "master_replid:" + config.replication_info.master_replid + "\r\n" +
                            "master_repl_offset:" + std::to_string(config.replication_info.master_repl_offset) + "\r\n";
        return "$" + std::to_string(str.size()) + "\r\n" + str + "\r\n";
    }
    else{
        return NOK;
    }   
    
    return NOK;
}