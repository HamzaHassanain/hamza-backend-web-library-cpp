#include <string>
#include <vector>
#include <map>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <web_utilities.hpp>
#include <iostream>

namespace hamza_web
{

    // Helper functions for web-related tasks

    /**
     * URL-encode implementation
     * - Iterates over characters in the input string
     * - Leaves unreserved characters (alnum and -_.~) unchanged
     * - Encodes other characters as %HH using uppercase hex
     */
    std::string url_encode(const std::string &value)
    {
        std::ostringstream escaped;
        for (const char &c : value)
        {
            if (isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.' || c == '~')
            {
                escaped << c;
            }
            else
            {
                // Use unsigned char cast to avoid sign-extension for non-ascii values
                escaped << '%' << std::uppercase << std::setw(2) << std::setfill('0') << std::hex << (static_cast<int>(static_cast<unsigned char>(c)) & 0xFF);
                // Reset formatting flags
                escaped << std::dec;
            }
        }
        return escaped.str();
    }

    /**
     * URL-decode implementation
     * - Scans for '%' sequences and converts two hex digits to a character
     * - Leaves other characters unchanged
     */
    std::string url_decode(const std::string &value)
    {
        std::string decoded;
        for (size_t i = 0; i < value.length(); ++i)
        {
            if (value[i] == '%')
            {
                if (i + 2 < value.length())
                {
                    std::string hex = value.substr(i + 1, 2);
                    char decoded_char = static_cast<char>(std::stoi(hex, nullptr, 16));
                    decoded += decoded_char;
                    i += 2;
                }
            }
            else
            {
                decoded += value[i];
            }
        }
        return decoded;
    }

    const std::vector<std::string> static_extensions = {
        // Web Documents
        "html", "htm", "xhtml", "xml",
        // Stylesheets
        "css", "scss", "sass", "less",
        // JavaScript
        "js", "mjs", "jsx", "ts", "tsx",
        // Images
        "png", "jpg", "jpeg", "gif", "bmp", "tiff", "tif",
        "svg", "webp", "ico", "cur", "avif",
        // Fonts
        "woff", "woff2", "ttf", "otf", "eot",
        // Audio
        "mp3", "wav", "ogg", "m4a", "aac", "flac",
        // Video
        "mp4", "webm", "avi", "mov", "wmv", "flv", "mkv",
        // Documents
        "pdf", "doc", "docx", "xls", "xlsx", "ppt", "pptx",
        "txt", "rtf", "odt", "ods", "odp",
        // Archives
        "zip", "rar", "7z", "tar", "gz", "bz2",
        // Data formats
        "json", "csv", "yaml", "yml", "toml",
        // Web Manifests & Config
        "manifest", "webmanifest", "map", "htaccess",
        // Other common formats
        "swf", "eps", "ai", "psd", "sketch"};

    const std::unordered_map<std::string, std::string> mime_types = {
        // Web Documents
        {"html", "text/html"},
        {"htm", "text/html"},
        {"xhtml", "application/xhtml+xml"},
        {"xml", "application/xml"},

        // Stylesheets
        {"css", "text/css"},
        {"scss", "text/x-scss"},
        {"sass", "text/x-sass"},
        {"less", "text/x-less"},

        // JavaScript
        {"js", "application/javascript"},
        {"mjs", "application/javascript"},
        {"jsx", "text/jsx"},
        {"ts", "application/typescript"},
        {"tsx", "text/tsx"},

        // Images
        {"png", "image/png"},
        {"jpg", "image/jpeg"},
        {"jpeg", "image/jpeg"},
        {"gif", "image/gif"},
        {"bmp", "image/bmp"},
        {"tiff", "image/tiff"},
        {"tif", "image/tiff"},
        {"svg", "image/svg+xml"},
        {"webp", "image/webp"},
        {"ico", "image/x-icon"},
        {"cur", "image/x-icon"},
        {"avif", "image/avif"},

        // Fonts
        {"woff", "font/woff"},
        {"woff2", "font/woff2"},
        {"ttf", "font/ttf"},
        {"otf", "font/otf"},
        {"eot", "application/vnd.ms-fontobject"},

        // Audio
        {"mp3", "audio/mpeg"},
        {"wav", "audio/wav"},
        {"ogg", "audio/ogg"},
        {"m4a", "audio/mp4"},
        {"aac", "audio/aac"},
        {"flac", "audio/flac"},

        // Video
        {"mp4", "video/mp4"},
        {"webm", "video/webm"},
        {"avi", "video/x-msvideo"},
        {"mov", "video/quicktime"},
        {"wmv", "video/x-ms-wmv"},
        {"flv", "video/x-flv"},
        {"mkv", "video/x-matroska"},

        // Documents
        {"pdf", "application/pdf"},
        {"doc", "application/msword"},
        {"docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
        {"xls", "application/vnd.ms-excel"},
        {"xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
        {"ppt", "application/vnd.ms-powerpoint"},
        {"pptx", "application/vnd.openxmlformats-officedocument.presentationml.presentation"},
        {"txt", "text/plain"},
        {"rtf", "application/rtf"},
        {"odt", "application/vnd.oasis.opendocument.text"},
        {"ods", "application/vnd.oasis.opendocument.spreadsheet"},
        {"odp", "application/vnd.oasis.opendocument.presentation"},

        // Archives
        {"zip", "application/zip"},
        {"rar", "application/vnd.rar"},
        {"7z", "application/x-7z-compressed"},
        {"tar", "application/x-tar"},
        {"gz", "application/gzip"},
        {"bz2", "application/x-bzip2"},

        // Data formats
        {"json", "application/json"},
        {"csv", "text/csv"},
        {"yaml", "application/x-yaml"},
        {"yml", "application/x-yaml"},
        {"toml", "application/toml"},

        // Web Manifests & Config
        {"manifest", "text/cache-manifest"},
        {"webmanifest", "application/manifest+json"},
        {"map", "application/json"},
        {"htaccess", "text/plain"},

        // Other common formats
        {"swf", "application/x-shockwave-flash"},
        {"eps", "application/postscript"},
        {"ai", "application/postscript"},
        {"psd", "image/vnd.adobe.photoshop"},
        {"sketch", "application/x-sketch"}};

    /**
     * @brief Get MIME type for a given file extension.
     *
     * @note
     * - Looks up the extension in the mime_types unordered_map
     * - Returns the mapped MIME type if present
     * - Returns "application/octet-stream" as a safe default otherwise
     */
    std::string get_mime_type_from_extension(const std::string &extension)
    {
        auto it = mime_types.find(extension);
        if (it != mime_types.end())
        {
            return it->second;
        }
        return "application/octet-stream";
    }

    /**
     * @brief Find an extension by MIME type.
     *
     * @note
     * - Iterates through mime_types map and returns the first key whose value matches
     * - This is O(n) and acceptable for small map sizes; if reverse lookup is frequent
     *   consider maintaining a reverse map
     */
    std::string get_file_extension_from_mime(const std::string &mime_type)
    {
        for (const auto &pair : mime_types)
        {
            if (pair.second == mime_type)
            {
                return pair.first;
            }
        }
        return "";
    }

    /**
     * @brief Extract the file extension from a URI or path.
     *
     * @note
     * - Finds the last '.' and returns the substring after it
     * - Does not validate characters after '.' and may return query/fragments if present
     */
    std::string get_file_extension_from_uri(const std::string &uri)
    {
        size_t dot_pos = uri.find_last_of('.');
        if (dot_pos != std::string::npos)
        {
            return uri.substr(dot_pos + 1);
        }
        return "";
    }

    /**
     * @brief Sanitize a requested path to mitigate directory traversal.
     *
     * @note
     * - Removes any ".." segments from the path
     * - This is a simple text-based sanitation; callers should still perform
     *   filesystem canonicalization when opening files to ensure security
     */
    std::string sanitize_path(const std::string &path)
    {
        std::string sanitized = path;
        // Remove any potentially dangerous sequences
        // For example, remove ".." to prevent directory traversal
        size_t pos;
        while ((pos = sanitized.find("..")) != std::string::npos)
        {
            sanitized.erase(pos, 2);
        }
        return sanitized;
    }

    /**
     * @brief Check if a URI points to a static resource by extension.
     *
     * @note
     * - Extracts the extension and performs a simple membership check in static_extensions
     * - Case-sensitive; callers should normalize extensions if needed
     */
    bool is_uri_static(const std::string &uri)
    {
        // Check if the URI has a static file extension
        std::string extension = get_file_extension_from_uri(uri);
        return std::find(static_extensions.begin(), static_extensions.end(), extension) != static_extensions.end();
    }

    /**
     * @brief Trim leading and trailing whitespace from a string.
     *
     * @note
     * - Uses find_first_not_of / find_last_not_of which are efficient for short strings
     */
    std::string trim(const std::string &str)
    {
        size_t first = str.find_first_not_of(" \t\n\r");
        size_t last = str.find_last_not_of(" \t\n\r");
        return (first == std::string::npos || last == std::string::npos) ? "" : str.substr(first, last - first + 1);
    }

    /**
     * @brief Extract parameter names from a route expression.
     *
     * @note
     * - Sanitizes the input
     * - Scans for ':' characters and captures the segment until the next '/'
     */
    std::vector<std::pair<std::string, std::string>> get_path_params(const std::string &uri)
    {
        std::vector<std::pair<std::string, std::string>> path_params;
        std::string path = sanitize_path(uri);
        size_t start = 0;
        while ((start = path.find(':', start)) != std::string::npos)
        {
            size_t end = path.find('/', start);
            std::string param = path.substr(start + 1, end - start - 1);
            path_params.emplace_back(param, "");
            start = (end == std::string::npos) ? std::string::npos : end + 1;
        }

        return path_params;
    }

    /**
     * @brief Extract the path component (without query) from a URI.
     */
    std::string get_path(const std::string &uri)
    {
        size_t pos = uri.find('?');
        if (pos == std::string::npos)
            return uri;
        return uri.substr(0, pos);
    }

    /**
     * @brief Parse query string into key/value pairs.
     *
     * @note
     * - Finds the query portion after '?'
     * - Splits on '&' then '=' to extract name/value
     * - Trims whitespace from name/value but does not URL-decode
     */
    std::vector<std::pair<std::string, std::string>> get_query_parameters(const std::string &uri)
    {

        std::vector<std::pair<std::string, std::string>> result;
        size_t pos = uri.find('?');
        if (pos == std::string::npos)
            return result;

        std::string query = uri.substr(pos + 1);

        // split the query string into key-value pairs
        std::istringstream query_stream(query);
        std::string pair;
        while (std::getline(query_stream, pair, '&'))
        {
            size_t equal_pos = pair.find('=');
            if (equal_pos != std::string::npos)
            {
                std::string name = hamza_web::trim(pair.substr(0, equal_pos));
                std::string value = hamza_web::trim(pair.substr(equal_pos + 1));
                result.emplace_back(name, value);
            }
        }
        return result;
    }

    /**
     * @brief Match a route expression against a request path.
     *
     * - Support exact matches, named parameters (":param") and wildcard ("*")
     */
    std::pair<bool, std::vector<std::pair<std::string, std::string>>> match_path(const std::string &expression, const std::string &path)
    {
        // Fast path: exact string match (covers cases including root "/")
        if (path == expression)
        {
            return {true, {}};
        }

        // Normalize function: remove redundant leading/trailing slashes (but keep "/" as-is)
        auto normalize = [](const std::string &s)
        {
            if (s.empty())
                return std::string();
            // If it's exactly "/", keep it
            if (s == "/")
                return std::string("/");
            size_t start = 0;
            while (start < s.size() && s[start] == '/')
                ++start;
            size_t end = s.size();
            while (end > start + 1 && s[end - 1] == '/')
                --end;
            if (start >= end)
                return std::string();
            return s.substr(start, end - start);
        };

        std::string expr = normalize(expression);
        std::string p = normalize(path);

        // If both are empty after normalization, treat as root
        if (expr.empty() && p.empty())
            return {true, {}};

        // Split into segments by '/'
        auto split_segments = [](const std::string &s)
        {
            std::vector<std::string> segs;
            if (s.empty())
                return segs;
            size_t pos = 0;
            while (pos < s.size())
            {
                size_t next = s.find('/', pos);
                if (next == std::string::npos)
                {
                    segs.emplace_back(s.substr(pos));
                    break;
                }
                segs.emplace_back(s.substr(pos, next - pos));
                pos = next + 1;
            }
            return segs;
        };

        std::vector<std::string> expr_segs = split_segments(expr);
        std::vector<std::string> path_segs = split_segments(p);

        std::vector<std::pair<std::string, std::string>> path_params;

        size_t ei = 0, pi = 0;
        while (ei < expr_segs.size() && pi < path_segs.size())
        {
            const std::string &es = expr_segs[ei];
            const std::string &ps = path_segs[pi];

            if (!es.empty() && es == "*")
            {
                // wildcard '*' captures the remainder of the path
                std::string remainder;
                for (size_t k = pi; k < path_segs.size(); ++k)
                {
                    if (!remainder.empty())
                        remainder += '/';
                    remainder += path_segs[k];
                }
                if (!remainder.empty())
                    path_params.emplace_back(std::string("*"), url_decode(remainder));
                return {true, path_params};
            }

            if (!es.empty() && es[0] == ':')
            {
                // named parameter - capture this segment value
                std::string name = es.substr(1);
                path_params.emplace_back(name, url_decode(ps));
                ++ei;
                ++pi;
                continue;
            }

            // exact segment match required
            if (es != ps)
                return {false, {}};

            ++ei;
            ++pi;
        }

        // If there are remaining expression segments
        if (ei < expr_segs.size())
        {
            // If exactly one remaining segment and it's '*', it matches the rest (which may be empty)
            if (ei + 1 == expr_segs.size() && expr_segs[ei] == "*")
            {
                // wildcard at end captures empty remainder
                return {true, path_params};
            }
            // Otherwise no match because expression expects more segments than path provides
            return {false, {}};
        }

        // If there are remaining path segments but expression exhausted, no match
        if (pi < path_segs.size())
            return {false, {}};

        // All segments matched
        return {true, path_params};
    }
};
