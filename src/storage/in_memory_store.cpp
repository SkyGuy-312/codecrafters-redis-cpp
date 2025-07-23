#include "in_memory_store.h"

void InMemoryStore::setVariable(const std::string &key, const std::string &value)
{
    this->map[key].value = value;
}

void InMemoryStore::setVariable(const std::string &key, const std::string &value, const std::chrono::milliseconds time_ms)
{
    this->map[key] = {value, time_ms+std::chrono::steady_clock::now()};
}

std::optional<std::string> InMemoryStore::getVariable(const std::string &key) const
{
    auto var = map.find(key);
    
    if (var == map.end()) return std::nullopt;

    if ((var->second.time != std::nullopt) && (std::chrono::steady_clock::now() - var->second.time.value()).count() > 0){
            // remove this key from map as it is expired
            this->map.erase(key);
            return std::nullopt;
    }

    return var->second.value;
};