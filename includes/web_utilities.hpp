
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <utility>
namespace hamza_web
{
    extern const std::vector<std::string> static_extensions;
    extern const std::unordered_map<std::string, std::string> mime_types;

    std::string url_encode(const std::string &value);
    std::string url_decode(const std::string &value);

    std::string get_mime_type_from_extension(const std::string &extension);
    std::string get_file_extension_from_mime(const std::string &mime_type);
    std::string get_file_extension_from_uri(const std::string &uri);
    std::string sanitize_path(const std::string &path);
    std::string trim(const std::string &str);

    bool is_uri_static(const std::string &uri);
    std::vector<std::pair<std::string, std::string>> get_query_parameters(const std::string &uri);
    std::vector<std::pair<std::string, std::string>> get_path_params(const std::string &uri);
    std::string get_path(const std::string &uri);
    bool match_path(const std::string &expression, const std::string &rhs);
}
