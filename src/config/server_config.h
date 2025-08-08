#pragma once
#include <string>

enum class ServerConfigErrors{
    CONFIG_NO_ERROR,
    CONFIG_ERROR_DUPLICATE_FLAGS,
    CONFIG_ERROR_INVALID_FLAG,
    CONFIG_ERROR_INVALID_FLAG_ARG,
};



struct ServerConfig
{
    std::string dir;
    std::string dbfilename;
    int port;
    ServerConfigErrors error{ServerConfigErrors::CONFIG_NO_ERROR};
    std::string role;
    std::string master_host;
    int master_port;
    void config_parser(int argc, char* argv[]);
    ServerConfig(int argc, char* argv[]);
};