#pragma once
#include <vector>
#include <string>
#include <array>
#include "../storage/in_memory_store.h"
class CommandHandler{
    private:
    InMemoryStore& storage;
    std::array<std::string, 4> commands = {
        "PING",
        "ECHO",
        "SET",
        "GET"
    };

    public:
    CommandHandler (InMemoryStore& store);
    std::string executeCommand(std::vector<std::string>& tokens_vector);


};

