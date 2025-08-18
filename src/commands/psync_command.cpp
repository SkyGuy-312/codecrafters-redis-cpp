#include "psync_command.h"

std::string PsyncCommand::execute(const std::vector<std::string> &tokens_vector, InMemoryStore &storage, const ServerConfig &config)
{
    if (tokens_vector[1] == "?" && tokens_vector[2] == "-1" && tokens_vector.size() == 3){

        std::string tmp = "+FULLRESYNC " + config.replication_info.master_replid + " " + std::to_string(config.replication_info.master_repl_offset) + "\r\n";

        return tmp;
                
    }
    return NOK;
}