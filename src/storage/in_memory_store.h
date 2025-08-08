#pragma once
#include <string>
#include <unordered_map>
#include <optional>
#include <chrono>
#include <vector>
class InMemoryStore{
    private:
    struct StoreValue
    {
        std::string value;
        
        std::optional<std::chrono::time_point<std::chrono::system_clock>> time;
    };
    

    mutable std::unordered_map<std::string, StoreValue> map;

    public:
    void setVariable(const std::string& key, const std::string& value);
    void setVariable(const std::string& key, const std::string& value, const std::chrono::milliseconds time_ms);
    void setVariable(const std::string &key, const std::string &value, std::chrono::time_point<std::chrono::system_clock> abs_time);
    std::vector<std::string> getAllVariables()const;
    void resize(int size);
    std::optional<std::string> getVariable (const std::string& key)const;
};