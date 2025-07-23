#pragma once
#include <string>
#include <unordered_map>
#include <optional>
class InMemoryStore{
    private:
    std::unordered_map<std::string, std::string> map;

    public:
    void setVariable(const std::string& key, const std::string& value);
    std::optional<std::string> getVariable (const std::string& key)const;
};