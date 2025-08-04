#include "idmanager.h"

i32 IDManager::operator[](std::string_view name)
{
    if (string_to_id.contains(name))
    {
        return static_cast<i32>(string_to_id[name]);
    }
    id_to_string.emplace_back(
        (u8 *)name.data(),
        (u8 *)(name.data() + name.size()));

    auto &vec = id_to_string.back();
    string_to_id[std::string_view((char *)vec.data(), vec.size())] = id_to_string.size() - 1;
    return id_to_string.size() - 1;
}

std::string_view IDManager::operator[](i32 id)
{
    auto &vec = id_to_string.at(id);
    return std::string_view((char *)vec.data(), vec.size());
}

i32 IDManager::lookup(u8 *start, u8 *end)
{
    return (*this)[std::string_view((char *)start, end - start)];
}