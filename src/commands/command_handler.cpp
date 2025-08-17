#include "command_handler.h"
#include <iostream>
#include <algorithm>

#define OK      "+OK\r\n"
#define NOK     "$-1\r\n"

CommandHandler::CommandHandler(InMemoryStore& store,const ServerConfig& config):storage(store), cfg_fd(config)
{
    this->commands["PING"] = std::make_unique<PingCommand>();
    this->commands["SET"]  = std::make_unique<SetCommand>();
    this->commands["GET"]  = std::make_unique<GetCommand>();
    this->commands["KEYS"]  = std::make_unique<KeysCommand>();
    this->commands["INFO"]  = std::make_unique<InfoCommand>();
    this->commands["CONFIG"]  = std::make_unique<ConfigCommand>();
    this->commands["ECHO"]  = std::make_unique<EchoCommand>();
    this->commands["REPLCONF"] = std::make_unique<ReplconfCommand>();
    this->commands["PSYNC"] = std::make_unique<PsyncCommand>();
}

std::string CommandHandler::executeCommand(std::vector<std::string>& tokens_vector)
{
    if (tokens_vector.empty()){
        return NOK;
    }
    
    //get command and process it
    auto command = tokens_vector[0];
    std::transform (command.begin(), command.end(), command.begin(), [](char c){
        return std::toupper(c);
    });
    
    //execute the command
    auto it = this->commands.find(command);
    if (it != commands.end()){
        return it->second->execute(tokens_vector, storage, cfg_fd);
    }
    else{
        return NOK;
    }
}
