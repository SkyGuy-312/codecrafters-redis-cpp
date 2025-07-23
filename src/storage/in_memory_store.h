#pragma once
#include <string>
#include <unordered_map>
#include <optional>
#include <chrono>
class InMemoryStore{
    private:
    struct StoreValue
    {
        std::string value;
        std::optional<std::chrono::time_point<std::chrono::steady_clock>> time;
    };
    

    mutable std::unordered_map<std::string, StoreValue> map;

    public:
    void setVariable(const std::string& key, const std::string& value);
    void setVariable(const std::string& key, const std::string& value, const std::chrono::milliseconds time_ms);
    std::optional<std::string> getVariable (const std::string& key)const;
};