#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include "../storage/in_memory_store.h"
#include "../config/server_config.h"
#include "command.h"
#include "ping_command.h"
#include "set_command.h"
#include "get_command.h"
#include "echo_command.h"
#include "info_command.h"
#include "keys_command.h"
#include "config_command.h"

class CommandHandler{
    private:
    InMemoryStore& storage;
    const ServerConfig& cfg_fd; 
    std::unordered_map<std::string, std::unique_ptr<Command>> commands;

    public:
    CommandHandler (InMemoryStore& store,const ServerConfig& config);
    std::string executeCommand(std::vector<std::string>& tokens_vector);


};

