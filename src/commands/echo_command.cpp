#include "echo_command.h"

std::string EchoCommand::execute(const std::vector<std::string> &tokens_vector, InMemoryStore &storage, const ServerConfig &config)
{
    if (tokens_vector.size() <= 1){
        return NOK;
    }
    return "$" + std::to_string(tokens_vector[1].length()) + "\r\n" + tokens_vector[1] + "\r\n";
}