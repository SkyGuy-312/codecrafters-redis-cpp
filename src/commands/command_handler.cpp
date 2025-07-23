#include "command_handler.h"
#include <iostream>
#include <algorithm>
CommandHandler::CommandHandler(InMemoryStore &store):storage(store)
{
}

std::string CommandHandler::executeCommand(std::vector<std::string>& tokens_vector)
{
    if (tokens_vector.empty()){
        std::cerr << "Error executing command: empty token vector" << std::endl;
        return "$-1\r\n";
    }
    
    //get command and process it
    auto command = tokens_vector[0];
    std::transform (command.begin(), command.end(), command.begin(), [](char c){
        return std::toupper(c);
    });
    #if 1
    std::cout << command << std::endl;
    #endif
    //execute the command
    if (command == "SET"){
        if (tokens_vector.size() <= 1){
            return "$-1\r\n";
        }
        this->storage.setVariable(tokens_vector[1],tokens_vector[2]);
        return "+OK\r\n";
    }
    else if(command == "GET"){
        if (tokens_vector.size() <= 1){
            return "$-1\r\n";
        }
        auto var = this->storage.getVariable(tokens_vector[1]);
        //put it in a RESP bulk string
        if (var == std::nullopt){
            return "$-1\r\n";
        }
        auto str = "$" + std::to_string(var->length()) + "\r\n" + var.value() + "\r\n"; 
        return str;
    }
    else if(command == "PING"){
        return std::string{"+PONG\r\n"};
    }
    else if(command == "ECHO"){
        if (tokens_vector.size() <= 1){
            return "$-1\r\n";
        }
        // send the command values back to client
        return "$" + std::to_string(tokens_vector[1].length()) + "\r\n" + tokens_vector[1] + "\r\n";
    }
    else if(command == "COMMAND"){
        return "+OK\r\n";
    }
    else{
        std::cerr << "Error executing command: invalid command" << std::endl;
        return "$-1\r\n";
    }


}
