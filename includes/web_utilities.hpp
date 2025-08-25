#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <utility>

#include "web_methods.hpp"

namespace hamza_web
{
    /**
     * @brief List of known static file extensions that should be treated as static resources.
     *
     * This list is used by is_uri_static() to determine whether a request URI refers
     * to a static asset (CSS, JS, images, fonts, etc.) and therefore should be
     * served from the static file directories instead of being routed to handlers.
     */
    extern const std::vector<std::string> static_extensions;

    /**
     * @brief Mapping from file extension to MIME type.
     *
     * This map is used to convert file extensions to the appropriate Content-Type
     * header when serving static files. If an extension is not found the default
     * MIME type "application/octet-stream" should be used.
     */
    extern const std::unordered_map<std::string, std::string> mime_types;

    /**
     * @brief Extract query parameters from a URI.
     * @param uri Full request URI
     * @return Vector of name-value pairs representing query parameters
     *
     * This function parses the query string portion of the URI and extracts
     * all query parameters as name-value pairs. It handles URL decoding
     * and proper parameter separation.
     */
    std::vector<std::pair<std::string, std::string>> get_query_parameters(const std::string &uri);

    /**
     * @brief URL-encode a string according to RFC 3986.
     * @param value Input string to encode
     * @return Encoded string where reserved characters are percent-encoded
     */
    std::string url_encode(const std::string &value);

    /**
     * @brief Decode a percent-encoded URL string.
     * @param value Percent-encoded input string
     * @return Decoded string
     */
    std::string url_decode(const std::string &value);

    /**
     * @brief Get MIME type for a given file extension.
     * @param extension File extension without the leading dot (e.g., "js", "html")
     * @return MIME type string or "application/octet-stream" if unknown
     */
    std::string get_mime_type_from_extension(const std::string &extension);

    /**
     * @brief Find a file extension associated with a MIME type.
     * @param mime_type MIME type string (e.g., "application/json")
     * @return Extension string without dot or empty string if not found
     */
    std::string get_file_extension_from_mime(const std::string &mime_type);

    /**
     * @brief Extract file extension from a URI or filename.
     * @param uri URI or path string
     * @return Extension without the dot, or empty string if none found
     */
    std::string get_file_extension_from_uri(const std::string &uri);

    /**
     * @brief Sanitize a requested path to mitigate directory traversal.
     * @param path Raw path from the request URI
     * @return Sanitized path with dangerous sequences removed
     *
     * This function performs simple sanitization such as removing ".." components.
     * It does not resolve symbolic links or perform filesystem canonicalization;
     * callers that need full security should canonicalize on the server side before
     * opening files.
     */
    std::string sanitize_path(const std::string &path);

    /**
     * @brief Trim leading and trailing whitespace from a string.
     * @param str Input string
     * @return Trimmed string
     */
    std::string trim(const std::string &str);

    /**
     * @brief Check whether a URI points to a static resource by extension.
     * @param uri Request URI
     * @return true if URI extension is in the static_extensions list
     */
    bool is_uri_static(const std::string &uri);

    /**
     * @brief Extract parameter names from a route expression.
     * @param uri Route expression or URI containing parameter placeholders (e.g., "/users/:id")
     * @return Vector of pairs {param_name, value} where value is empty when only names are extracted
     *
     * Note: This utility only extracts parameter names when the input contains
     * placeholders like ":id". Resolving parameter values from an actual request
     * path requires matching against the route expression and is typically done
     * by the router during request handling.
     */
    std::vector<std::pair<std::string, std::string>> get_path_params(const std::string &uri);

    /**
     * @brief Extract the path component (without query) from a URI.
     * @param uri Full request URI
     * @return Path portion of the URI (everything before '?')
     */
    std::string get_path(const std::string &uri);

    /**
     * @brief Match a route expression against a request path.
     * @param expression Route expression (may include ":param" and "*" wildcard)
     * @param rhs Actual request path to test
     * @return true if the expression matches the path, false otherwise
     *
     * The rule set implemented by match_path supports:
     * - Exact segment match
     * - Named parameters using leading ':' (matches a single segment)
     */
    std::pair<bool, std::vector<std::pair<std::string, std::string>>> match_path(const std::string &expression, const std::string &rhs);

    /**
     * @brief Check if an HTTP method is unknown.
     * @param method HTTP method string (e.g., "GET", "POST")
     * @return true if the method is unknown, false otherwise
     */
    bool unknown_method(const std::string &method);
}
