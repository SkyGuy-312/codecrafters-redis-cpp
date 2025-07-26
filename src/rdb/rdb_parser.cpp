#include "rdb_parser.h"
#include <iostream>
#include <filesystem>

bool RDBParser::verifyRedisHeader(std::string &header)
{
    if (header == this->header){
        return true;
    }
    return false;
}

bool RDBParser::parseSizeEncodedValue(std::fstream &rdb_fd, uint64_t &size)
{
    constexpr uint8_t LSB_6_BIT_MASK = 0b00111111;
    constexpr uint8_t SHIFT_6_STEPS = 6;
    constexpr uint8_t SHIFT_8_STEPS = 8;
    size = 0;
    
    uint8_t byte = rdb_fd.get();
    uint8_t size_code = byte >> SHIFT_6_STEPS;

    switch (size_code){
        case static_cast<uint8_t>(RDBSizeEncoding::SIZE_REST_OF_BYTE_SYSTEM_ENDIANESS): 
            size = byte & LSB_6_BIT_MASK;
            break;
        case static_cast<uint8_t>(RDBSizeEncoding::SIZE_NEXT_14_BITS_BIG_ENDIAN): 
        {
            uint8_t next_byte = rdb_fd.get();
            size = (byte & LSB_6_BIT_MASK) << SHIFT_8_STEPS | next_byte;
            break;
        }
        case static_cast<uint8_t>(RDBSizeEncoding::SIZE_NEXT_4_BYTES_BIG_ENDIAN):
        {
            uint32_t value;
            rdb_fd.read(reinterpret_cast<char*>(&value), sizeof(uint32_t));
            size = __builtin_bswap32(value);    // GCC builtin function for swapping
            break;
        }
        case static_cast<uint8_t>(RDBSizeEncoding::SIZE_CUSTOM_STRING_ENCODING): 
            // This is handled by parseString. This function should not be called
            // for this encoding type.
        default:
            std::cerr << "Invalid RDB file structure. skipping RDB file read..." << std::endl;
            this->error = RDBParserErrors::RDB_PARSER_INVALID_FILE_STRUCTURE_ERROR;
            return false;
    }
    return true;
}


bool RDBParser::parseString(std::fstream &rdb_fd, std::string& out_str)
{
    // We need to check the first byte to see if it's a special encoding
    // without consuming it from the stream.
    uint8_t first_byte = rdb_fd.peek();
    uint8_t size_code = first_byte >> 6;

    if (size_code == static_cast<uint8_t>(RDBSizeEncoding::SIZE_CUSTOM_STRING_ENCODING)) {
        rdb_fd.get(); // Consume the byte we peeked
        uint8_t encoding_type = first_byte & 0b00111111;
        if (encoding_type == 0) { // 8-bit integer
            int8_t val;
            rdb_fd.read(reinterpret_cast<char*>(&val), sizeof(int8_t));
            out_str = std::to_string(val);
        } else if (encoding_type == 1) { // 16-bit integer
            int16_t val;
            rdb_fd.read(reinterpret_cast<char*>(&val), sizeof(int16_t));
            out_str = std::to_string(val);
        } else if (encoding_type == 2) { // 32-bit integer
            int32_t val;
            rdb_fd.read(reinterpret_cast<char*>(&val), sizeof(int32_t));
            out_str = std::to_string(val);
        } else {
            // LZF compression not supported
            return false;
        }
        return true;
    }

    // parse size encode of variable
    uint64_t size {};
    if(!parseSizeEncodedValue(rdb_fd, size)){
        return false;
    }
    // get the data string
    out_str.resize(size);
    rdb_fd.read(&out_str[0], size);
    return true;

}

bool RDBParser::parseEOFSection(std::fstream &rdb_fd)
{
    // calculate CRC for the file
    
    // for now, let this stub return true as I'm not sure how to parse CRC and confirm it
    
    return true;
}

RDBParser::RDBParser(InMemoryStore &store):storage(store)
{
    this->error = RDBParserErrors::RDB_PARSER_NO_ERROR;
    this->active_section = RDBOpcode::IDLE;
    this->is_timestamped = false;
    this->pair_counts_left = 0;
}

void RDBParser::parse(const std::string &filepath)
{


    using namespace std::filesystem;

    //opening rdb file, check if it is currupted or doesn't exist
    path source = filepath;

    std::fstream rdb_fd {source, std::ios_base::binary | std::ios_base::in};
    if (!rdb_fd.is_open()){
        std::cerr << source.filename() << " does not exist! treating it as new empty EDB file..." << std::endl;
        this->error = RDBParserErrors::RDB_PARSER_FILE_NOT_EXIST;
        return;
    }
    if (rdb_fd.bad()){
        std::cerr << source.filename() << " is currupted, skipping RDB file..." << std::endl;
        this->error = RDBParserErrors::RDB_PARSER_FILE_CURRUPTED;
        return;
    }

    // first line should always be header
    std::string hdr{};
    hdr.resize(HEADER_SIZE);
    rdb_fd.read(&hdr[0],HEADER_SIZE);
    if (!this->verifyRedisHeader(hdr)){
        std::cerr << "Invalid RDB file header! skipping RDB file..." << std::endl;
        this->error = RDBParserErrors::RDB_PARSER_INVALID_HEADER_ERROR;
        return;
    }

    std::string key {};
    std::string value {};

    std::optional<std::chrono::milliseconds> expiry_time;

    // the parsing loop
    while (rdb_fd.peek() != EOF){
        uint8_t opcode_byte = rdb_fd.get();

        // Handle special, non-value opcodes first.
        switch(static_cast<RDBOpcode>(opcode_byte)){
            case RDBOpcode::EXPIRE_TIME_MS: {
                uint64_t expiry_ms_val;
                rdb_fd.read(reinterpret_cast<char*>(&expiry_ms_val), sizeof(uint64_t));
                expiry_time = std::chrono::milliseconds(expiry_ms_val);
                continue; // A timestamp is always followed by a key-value pair.
            }
            case RDBOpcode::EXPIRE_TIME_S: {
                uint32_t expiry_s_val;
                rdb_fd.read(reinterpret_cast<char*>(&expiry_s_val), sizeof(uint32_t));
                expiry_time = std::chrono::seconds(expiry_s_val);
                continue;
            }
            case RDBOpcode::SELECT_DB: {
                uint64_t db_num_size;
                if (!parseSizeEncodedValue(rdb_fd, db_num_size)) return;
                // We just consume the db number for now.
                continue;
            }
            case RDBOpcode::RESIZE_DB: {
                uint64_t db_size, expires_size;
                if (!parseSizeEncodedValue(rdb_fd, db_size)) return;
                if (!parseSizeEncodedValue(rdb_fd, expires_size)) return;
                continue;
            }
            case RDBOpcode::AUX: {
                std::string aux_key, aux_val;
                if (!parseString(rdb_fd, aux_key)) return;
                if (!parseString(rdb_fd, aux_val)) return;
                continue;
            }
            case RDBOpcode::EOF_MARKER:
                parseEOFSection(rdb_fd);
                return; // End of file
        }

        // If we get here, the opcode must be a value type.
        auto value_type = static_cast<RDBValueType>(opcode_byte);
        if (value_type != RDBValueType::STRING) {
            std::cout << (int) value_type << std::endl;
            std::cerr << "Invalid or unsupported value type in RDB file: " << static_cast<int>(opcode_byte) << std::endl;
            return;
        }

        std::string key, value;
        if (!parseString(rdb_fd, key)) return;
        if (!parseString(rdb_fd, value)) return;

        if (expiry_time.has_value()) {
            this->storage.setVariable(key, value, expiry_time.value());
            expiry_time.reset(); // Consume the expiry time, it's been used.
        } else {
            this->storage.setVariable(key, value);
        }
    }
}
