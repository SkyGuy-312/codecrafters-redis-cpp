#include "command_handler.h"
#include <iostream>
#include <algorithm>
CommandHandler::CommandHandler(InMemoryStore& store,const ServerConfig& config):storage(store), cfg_fd(config)
{
}

std::string CommandHandler::executeCommand(std::vector<std::string>& tokens_vector)
{
    const std::string OK = "+OK\r\n";
    const std::string NOK = "$-1\r\n";

    if (tokens_vector.empty()){
        // std::cerr << "Error executing command: empty token vector" << std::endl;
        
        return NOK;
    }
    
    //get command and process it
    auto command = tokens_vector[0];
    std::transform (command.begin(), command.end(), command.begin(), [](char c){
        return std::toupper(c);
    });
    #if 0
    std::cout << command << std::endl;
    #endif
    
    //execute the command
    if (command == "SET"){
        if ((tokens_vector.size() != 3) && (tokens_vector.size() != 5)){
            return NOK;
        }
        if (tokens_vector.size() == 5){
            auto sub_command = tokens_vector[3];
            std::transform(sub_command.begin(), sub_command.end(),
             sub_command.begin(), [](char c) { return std::toupper(c);});

             if (sub_command == "PX"){
                auto time_ms = std::stoll(tokens_vector[4]);
                this->storage.setVariable(tokens_vector[1], tokens_vector[2], static_cast<std::chrono::milliseconds>(time_ms));
                return "+OK\r\n";
             }
             else if(sub_command == "EX"){
                auto time_ms = std::stoll(tokens_vector[4]);
                this->storage.setVariable(tokens_vector[1], tokens_vector[2], static_cast<std::chrono::milliseconds>(time_ms)*1000);
                return "+OK\r\n";
             }
             else{
                std::cerr << "Error Executing Command: invalud sub-command" << std::endl;
                return NOK;
             }

        }
        else if (tokens_vector.size() == 3){
            this->storage.setVariable(tokens_vector[1],tokens_vector[2]);
            return "+OK\r\n";
        }
        return NOK;
    }
    else if(command == "GET"){
        if (tokens_vector.size() <= 1){
            return NOK;
        }
        auto var = this->storage.getVariable(tokens_vector[1]);
        //put it in a RESP bulk string
        if (var == std::nullopt){
            return NOK;
        }
        auto str = "$" + std::to_string(var->length()) + "\r\n" + var.value() + "\r\n"; 
        return str;
    }
    else if(command == "PING"){
        return std::string{"+PONG\r\n"};
    }
    else if(command == "ECHO"){
        if (tokens_vector.size() <= 1){
            return NOK;
        }
        // send the command values back to client
        return "$" + std::to_string(tokens_vector[1].length()) + "\r\n" + tokens_vector[1] + "\r\n";
    }
    else if(command == "COMMAND"){
        return "+OK\r\n";
    }
    else if(command == "CONFIG"){
        auto sub_command = tokens_vector[1];
        std::transform(sub_command.begin(), sub_command.end(),
            sub_command.begin(), [](char c) { return std::toupper(c);});
        if (sub_command == "GET"){
            if (tokens_vector.size() <= 2){
                return NOK;
            }
            if (tokens_vector[2] == "dir"){
                return "*2\r\n" + static_cast<std::string>("$3\r\n") + "dir\r\n" + "$" + std::to_string(this->cfg_fd.dir.size()) + "\r\n" + this->cfg_fd.dir + "\r\n";
            }
            else if (tokens_vector[2] == "dbfilename"){
                return "*2\r\n" + static_cast<std::string>("$10\r\n") + "dbfilename\r\n" + "$" + std::to_string(this->cfg_fd.dbfilename.size()) + "\r\n" + this->cfg_fd.dbfilename + "\r\n"; 
            }
            else {
                return NOK;
            }
            
        }
        else{
            return NOK;
        }
    }
    else if(command == "KEYS"){
        if (tokens_vector.size() == 2 && tokens_vector[1] =="*"){
            std::vector<std::string> keys = this->storage.getAllVariables();
            std::string response{};
            response += "*" + std::to_string(keys.size()) + "\r\n";
            for (auto &key : keys){
                response += "$" + std::to_string(key.length()) + "\r\n" + key + "\r\n";
            }
            return response;
        }
        else{
            return NOK;
        }
    }
    else if(command == "INFO"){
        if (tokens_vector.size() != 2){
            return NOK;
        }
        auto sub_command = tokens_vector[1];
        std::transform(sub_command.begin(), sub_command.end(),
            sub_command.begin(), [](char c) { return std::toupper(c);});
        if (sub_command == "REPLICATION"){
            std::string str = "# Replication\r\nrole:" + this->cfg_fd.role + "\r\n";
            return "$" + std::to_string(str.size()) + "\r\n" + str + "\r\n";
        }
        else{
            std::cerr << "Invalid subcommand" << std::endl;
        }   
        
        return NOK;
    }
    else{
        std::cerr << "Error executing command: invalid command" << std::endl;
        return NOK;
    }


}
