#include "idmanager.h"
#include <iostream>

i32 IDManager::operator[](std::string_view name)
{
    // if this name is already in our string_to_id map -> return it's id as an i32
    if (string_to_id.contains(name))
    {
        return static_cast<i32>(string_to_id[name]);
    }
    // This name does not have an ID
    // create a string and place it in the back of id_to_string (vector of u8 vectors)
    id_to_string.emplace_back(              //<-- emplace back is implicitly creating a vector<u8> since id_to_string = vector<vector<u8>>
        (u8 *)name.data(),                  // <-- name.data() gives a const char* pointer to the start of name, we then cast it as u8*
        (u8 *)(name.data() + name.size())); // <-- Thus, by adding the size we then get a u8* to the end + 1
    // ^ so basically, we have two pointers: a start and end
    // Then, emplace_back calls the std::vector constructor
    // std::vector has a constructor which takes in a start and end ptr
    // This means emplace_back creates the new_vector in place inside of id_to_string avoiding additional overhead by using push_back

    auto &vec = id_to_string.back(); // <-- reference to the newly added string
    //^ auto just means let the compiler infer the type for me

    string_to_id[std::string_view((char *)vec.data(), vec.size())] = id_to_string.size() - 1; // <-- make a string view, remember string view takes a start ptr and a size
    // map the string_view to its id which in this case is just it's size
    // return the size
    // std::cout << "id_to_string:\n";
    // for (size_t i = 0; i < id_to_string.size(); ++i)
    // {
    //     const auto &vec = id_to_string[i];
    //     std::string s((char *)vec.data(), vec.size()); // convert vector<u8> to printable string
    //     std::cout << "  [" << i << "]: " << s << ": " << string_to_id[std::string_view((char *)vec.data(), vec.size())] << "\n";
    // }
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