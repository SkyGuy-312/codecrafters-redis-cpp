#include "in_memory_store.h"

void InMemoryStore::setVariable(const std::string &key, const std::string &value)
{
    this->map[key].value = value;
}

void InMemoryStore::setVariable(const std::string &key, const std::string &value, const std::chrono::milliseconds time_ms)
{
    this->map[key] = {value, std::chrono::time_point<std::chrono::system_clock>(time_ms)};
}

std::vector<std::string> InMemoryStore::getAllVariables() const
{   
    std::vector <std::string> keys{};
    for (const auto& pair : this->map){
        keys.push_back(pair.first);
    }
    return keys;
}

void InMemoryStore::resize(int size)
{
    this->map.reserve(size);
}

std::optional<std::string> InMemoryStore::getVariable(const std::string &key) const
{
    auto var = map.find(key);
    
    if (var == map.end()) return std::nullopt;

    if (var->second.time.has_value() && std::chrono::system_clock::now() > var->second.time.value()){
            // remove this key from map as it is expired
            this->map.erase(key);
            return std::nullopt;
    }

    return var->second.value;
};