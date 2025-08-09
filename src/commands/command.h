#pragma once
#include <vector>
#include <string>
#include "../storage/in_memory_store.h"
#include "../config/server_config.h"

#define OK      "+OK\r\n"
#define NOK     "-1\r\n"

class Command{
    public:
    virtual ~Command() = default;
    virtual std::string execute(const std::vector<std::string>& tokens_vector, InMemoryStore& storage, const ServerConfig& config) = 0;
};