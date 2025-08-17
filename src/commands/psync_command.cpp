#include "psync_command.h"

std::string PsyncCommand::execute(const std::vector<std::string> &tokens_vector, InMemoryStore &storage, const ServerConfig &config)
{
    if (tokens_vector[1] == "?" && tokens_vector[2] == "-1"){
        return 
        "*3\r\n"
        "$11\r\n+FULLRESYNC\r\n" 
        "$40\r\n" + config.replication_info.master_replid + "\r\n"
        "$1\r\n" + std::to_string(config.replication_info.master_repl_offset) + "\r\n";
                
    }
}