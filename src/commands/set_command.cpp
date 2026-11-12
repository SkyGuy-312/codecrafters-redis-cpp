#include "set_command.h"
#include <iostream>

std::string SetCommand::execute(const std::vector<std::string> &tokens_vector, InMemoryStore &storage, const ServerConfig &config)
{
    if ((tokens_vector.size() != 3) && (tokens_vector.size() != 5)){
        return NOK;
    }
    if (tokens_vector.size() == 5){
        auto sub_command = tokens_vector[3];
        std::transform(sub_command.begin(), sub_command.end(),
            sub_command.begin(), [](char c) { return std::toupper(c);});

        long long time_ms;

        try{
            time_ms = std::stoll(tokens_vector[4]);
            if (time_ms <= 0){
                time_ms = 0;
            }
        }
        catch (const std::exception &e){
            std::cerr << "Error Executing Command: invalid time value" << std::endl;
            return NOK;
        }
        if (sub_command == "PX"){
        storage.setVariable(tokens_vector[1], tokens_vector[2], static_cast<std::chrono::milliseconds>(time_ms));
        return OK;
        }
        else if(sub_command == "EX"){
        storage.setVariable(tokens_vector[1], tokens_vector[2], static_cast<std::chrono::milliseconds>(time_ms)*1000);
        return OK;
        }
        else{
        std::cerr << "Error Executing Command: invalud sub-command" << std::endl;
        return NOK;
        }

    }
    else if (tokens_vector.size() == 3){
        storage.setVariable(tokens_vector[1],tokens_vector[2]);
        return OK;
    }
    return NOK;
}