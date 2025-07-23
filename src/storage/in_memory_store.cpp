#include "in_memory_store.h"

void InMemoryStore::setVariable(const std::string &key, const std::string &value)
{
    this->map[key] = value;
}

std::optional<std::string> InMemoryStore::getVariable(const std::string &key) const
{
    auto var = map.find(key);
    if (var != map.end()) return var->second;
    else return std::nullopt;
};