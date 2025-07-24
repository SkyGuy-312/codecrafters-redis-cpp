#pragma once
#include <vector>
#include <string>
#include <array>
#include "../storage/in_memory_store.h"
#include "../config/server_config.h"
class CommandHandler{
    private:
    InMemoryStore& storage;
    const ServerConfig& rdb_fd; 

    public:
    CommandHandler (InMemoryStore& store,const ServerConfig& config);
    std::string executeCommand(std::vector<std::string>& tokens_vector);


};

