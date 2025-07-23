#include "parser.h"
#include <iostream>

std::vector<std::string> RespParser::dataParser(char data_buffer[], int data_size)
{
    // declare tokens vector
    std::vector<std::string> tokens;

    // start RESP parsing:
    // 1. convert byte buffer to string
    std::string data(data_buffer, data_size);

    int pos{};
    if (data[0] != '*')
    {
        std::cerr << "cannot find size of input, aborting...\n";
        return tokens;
    }
    else
    {
        // 2. create position index, with getting array input size as first position:
        pos = data.find("\r\n");
        if (pos == std::string::npos)
        {
            std::cerr << "error parsing upcoming string: invalid RESP structure\n";
            return tokens;
        }

        // 3. create tokens array from it
        int tokens_vec_len{};
        try
        {
            tokens_vec_len = std::stoi(data.substr(1, pos - 1));
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << '\n';
            std::cerr << "Invalid RESP structure: invalid input size" << std::endl;
            return tokens;
        }

#if 0
        std::cout << "Tokens: [";
        for (size_t t = 0; t < tokens.size(); ++t)
        {
            std::cout << "\"" << tokens[t] << "\"";
            if (t != tokens.size() - 1)
                std::cout << ", ";
        }
        std::cout << "]\n";
#endif

        // 4. parse the bulk string
        pos += std::string("\r\n").length(); // now this should data[pos] = '$'
        for (int i = 0; i < tokens_vec_len; i++)
        {
            // get the next element in data buffer
            // a. get the string length
            if (data[pos] != '$')
            {
                std::cerr << "Invalid RESP structure: invalid input size" << std::endl;
                return std::vector<std::string>{};
            }
            auto next_pos = data.find("\r\n", pos);
            auto str_len = data.substr(pos + 1, next_pos - (pos + 1));

            int str_len_int{};
            try
            {
                str_len_int = std::stoi(str_len);
            }
            catch (const std::exception &e)
            {
                std::cerr << e.what() << '\n';
                std::cerr << "INVALID RESP structure: invalid input no. " << i << " string length" << std::endl;
                return std::vector<std::string>{};
            }

            pos = next_pos + std::string("\r\n").length();

            // b. get the string itself
            tokens.push_back(data.substr(pos, str_len_int));

            // c. move position index
            pos += str_len_int + std::string("\r\n").length();
        }

#if 0
        std::cout << "Tokens: [";
        for (size_t t = 0; t < tokens.size(); ++t)
        {
            std::cout << "\"" << tokens[t] << "\"";
            if (t != tokens.size() - 1)
                std::cout << ", ";
        }
        std::cout << "]\n";
#endif
        return tokens;
    }
}