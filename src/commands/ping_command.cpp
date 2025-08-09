#include "ping_command.h"

std::string PingCommand::execute(const std::vector<std::string> &tokens_vector, InMemoryStore &storage, const ServerConfig &config)
{
    return "+PONG\r\n";
}