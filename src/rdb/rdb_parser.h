#pragma once
#include "../storage/in_memory_store.h"
#include "../rdb/rdb_parser.h"
#include <fstream>


// expiries for keys
union TimeValues {
    uint64_t milliseconds;
    uint32_t seconds;

    TimeValues& operator=(uint64_t other) {
        // Copy both fields to ensure correctness
        this->seconds = other;
        this->milliseconds = other;
        return *this;
    }

};

// parsing errors for easier debugging if needed
enum class RDBParserErrors: uint8_t{
    RDB_PARSER_NO_ERROR,
    RDB_PARSER_INVALID_HEADER_ERROR,
    RDB_PARSER_INVALID_FILE_STRUCTURE_ERROR,
    RDB_PARSER_FILE_NOT_EXIST,
    RDB_PARSER_FILE_CURRUPTED,
};

// opcodes supported by parser
enum class RDBOpcode: uint8_t{
    IDLE,
    AUX = 0xFA,
    SELECT_DB = 0xFE,
    RESIZE_DB = 0xFB,
    EXPIRE_TIME_MS = 0xFC,
    EXPIRE_TIME_S = 0xFD,
    EOF_MARKER = 0xFF,
};

// value types supported by parser
enum class RDBValueType: uint8_t{
    STRING = 0x00,
    LIST = 0x01,
    SET = 0x02,
    SORTED_SET = 0x03,
    HASH = 0x04 
};

enum class RDBSizeEncoding: uint8_t{
    SIZE_REST_OF_BYTE_SYSTEM_ENDIANESS = 0b00,
    SIZE_NEXT_14_BITS_BIG_ENDIAN = 0b01,
    SIZE_NEXT_4_BYTES_BIG_ENDIAN = 0b10,
    SIZE_CUSTOM_STRING_ENCODING = 0b11
};

class RDBParser{

    static constexpr int HEADER_SIZE = 9;
    static constexpr int SELECT_DB_SIZE = 2;
    const std::string header = "REDIS0011";
    RDBParserErrors error = RDBParserErrors::RDB_PARSER_NO_ERROR;
    InMemoryStore& storage;
    uint64_t pair_counts_left;
    uint8_t is_timestamped;
    RDBOpcode active_section;
    TimeValues time;
    
    bool verifyRedisHeader(std::string& header);
    bool parseSizeEncodedValue(std::fstream &rdb_fd, uint64_t& size);
    bool parseString(std::fstream &rdb_fd, std::string& out_str);
    bool parseEOFSection(std::fstream& rdb_fd);
    
    

public:
    RDBParser(InMemoryStore& store);
    void parse(const std::string& filepath);
    static std::string getEmptyRDB();
};