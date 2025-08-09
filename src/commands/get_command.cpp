#include "get_command.h"

std::string GetCommand::execute(const std::vector<std::string>& tokens_vector, InMemoryStore &storage, const ServerConfig &config)
{
    if (tokens_vector.size() <= 1){
        return NOK;
    }
    auto var = storage.getVariable(tokens_vector[1]);
    //put it in a RESP bulk string
    if (var == std::nullopt){
        return NOK;
    }
    auto str = "$" + std::to_string(var->length()) + "\r\n" + var.value() + "\r\n"; 
    return str;
}