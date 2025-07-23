#pragma once
#include <string>
#include <vector>

class RespParser{
public:
    static std::vector<std::string> dataParser(char data_buffer[], int data_size);
private:
// making the constructor private prevents anyone from creating an instance of the class
RespParser() = default;
};