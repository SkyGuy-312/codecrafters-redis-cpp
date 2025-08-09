#include "keys_command.h"

std::string KeysCommand::execute(const std::vector<std::string> &tokens_vector, InMemoryStore &storage, const ServerConfig &config)
{
    if (tokens_vector.size() == 2 && tokens_vector[1] =="*"){
        std::vector<std::string> keys = storage.getAllVariables();
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