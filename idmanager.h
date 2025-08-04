#ifndef IDMANAGER_H
#define IDMANAGER_H
#include <cstdint>
#include <vector>
#include <unordered_map>
using i32 = int32_t;
using u8 = uint8_t;

struct IDManager
{
    i32 lookup(u8 *start, u8 *end);
    i32 operator[](std::string_view name);
    std::string_view operator[](i32 id);

private:
    std::vector<std::vector<u8>> id_to_string;
    std::unordered_map<std::string_view, size_t> string_to_id;
};

#endif