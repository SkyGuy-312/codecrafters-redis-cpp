#pragma once

#include "command.h"

class ReplconfCommand: public Command{
    public:
    virtual std::string execute(const std::vector<std::string>& tokens_vector, InMemoryStore& storage, const ServerConfig& config)override;
};