#pragma once

#include <string>
#include <vector>
#include <utility>
#include <mutex>
#include <algorithm>

#include "../libs/http-server/http-lib.hpp"

#include "web_exceptions.hpp"
#include "web_utilities.hpp"
namespace hh_web
{
    template <typename T, typename G>
    class web_router;

    template <typename T, typename G, typename R>
    class web_server;

    /**
     * @brief High-level web request wrapper with enhanced functionality.
     *
     * This class provides a high-level interface for HTTP requests by wrapping
     * the lower-level hh_http::http_request class. It adds web framework-specific
     * functionality such as path parameter extraction, query parameter parsing,
     * and convenient access to common HTTP headers.
     *
     * The class serves as an abstraction layer that simplifies request handling
     * in web applications while maintaining access to all underlying HTTP request
     * data. It provides utility methods for common web development tasks like
     * routing, parameter extraction, and header processing.
     *
     * @note Object Provides get only access to the underlying HTTP request data, EXCEPT for
     *       the path parameters, which can be modified.
     * @note It is intended to be passed as pointers between the methods, to ensure proper ownership and lifetime management.
     * @note Please NEVER Initialize web_request directly, Unless you are overriding the web_servers on_request_received method
     */
    class web_request
    {
    protected:
        /// Underlying HTTP request object
        hh_http::http_request request;

        /// Path parameters extracted from the request URI, in the form of key-value pairs
        /// http://localhost/users/:id/posts/:postID
        /// http://localhost/users/123/posts/456
        /// vector holds => {"id", "123"}, {"postID", "456"}
        std::vector<std::pair<std::string, std::string>> path_params;

        /// Modify path_params mutex
        mutable std::mutex path_params_mutex;

        /// Custom request parameters (e.g., from query string)
        std::map<std::string, std::string> request_params;

    public:
        /// Allow web_server to access private members
        template <typename T, typename G, typename R>
        friend class web_server;

        /**
         * @brief Construct web request from HTTP request.
         * @param req HTTP request object to wrap (moved)
         *
         * Creates a web request wrapper around the provided HTTP request object.
         * The HTTP request is moved to avoid unnecessary copying and to maintain
         * ownership semantics. This constructor is typically called by the web
         * server when processing incoming requests.
         */
        web_request(hh_http::http_request &&req) : request(std::move(req))
        {
        }

        // Copy operations - DELETED for resource safety and unique ownership
        web_request(const web_request &) = delete;
        web_request &operator=(const web_request &) = delete;

        // Move operations - ENABLED for ownership transfer
        web_request(web_request &&) = default;
        web_request &operator=(web_request &&) = default;

        /**
         * @brief Extract path parameters from the request URI.
         * @return Vector of name-value pairs representing path parameters
         *
         * Parses the request URI to extract path parameters (e.g., route variables).
         * This method uses web utilities to analyze the URI structure and extract
         * dynamic segments that were matched during routing.
         *
         * Example: For URI "/users/123/posts/456", this might return:
         * [{"userId", "123"}, {"postId", "456"}]
         */
        virtual std::vector<std::pair<std::string, std::string>> get_path_params() const
        {

            return path_params;
        }

        /**
         * @brief Set the path params object
         * @note please only call this method from the web_route class, otherwise you will never know the path parameters
         * @param params The new path parameters to set
         */
        virtual void set_path_params(const std::vector<std::pair<std::string, std::string>> &params)
        {
            std::lock_guard<std::mutex> lock(path_params_mutex);
            path_params = params;
        }

        /**
         * @brief Get the HTTP method of the request.
         * @return String containing the HTTP method (GET, POST, PUT, DELETE, etc.)
         *
         * Returns the HTTP method used for this request. Common methods include
         * GET for data retrieval, POST for data submission, PUT for updates,
         * DELETE for removal operations, etc.
         */
        virtual std::string get_method() const
        {
            return request.get_method();
        }

        /**
         * @brief Get the path component of the request URI.
         * @return String containing the path without query parameters
         *
         * Extracts and returns only the path portion of the URI, excluding
         * query parameters and fragments. This is useful for routing and
         * path-based request handling.
         *
         * Example: For URI "/api/users?page=1&limit=10", returns "/api/users"
         */
        virtual std::string get_path() const
        {
            return hh_web::get_path(request.get_uri());
        }

        /**
         * @brief Get the complete request URI.
         * @return String containing the full URI including query parameters
         *
         * Returns the complete URI as received in the HTTP request, including
         * path, query parameters, and any other URI components.
         */
        virtual std::string get_uri() const
        {
            return request.get_uri();
        }

        /**
         * @brief Extract query parameters from the request URI.
         * @return Vector of name-value pairs representing query parameters
         *
         * Parses the query string portion of the URI to extract all query
         * parameters as name-value pairs. This method handles URL decoding
         * and proper parameter separation.
         *
         * Example: For URI "/search?q=example&category=news&page=2", returns:
         * [{"q", "example"}, {"category", "news"}, {"page", "2"}]
         */
        virtual std::vector<std::pair<std::string, std::string>> get_query_parameters() const
        {

            return hh_web::get_query_parameters(request.get_uri());
        }

        virtual std::string get_query_parameter(const std::string &key) const
        {
            auto params = get_query_parameters();
            for (const auto &param : params)
            {
                if (param.first == key)
                {
                    return param.second;
                }
            }
            return "";
        }

        /**
         * @brief Get the HTTP version of the request.
         * @return String containing the HTTP version (e.g., "HTTP/1.1", "HTTP/2")
         *
         * Returns the HTTP protocol version used by the client for this request.
         * This information can be useful for implementing version-specific
         * features or optimizations.
         */
        virtual std::string get_version() const
        {
            return request.get_version();
        }

        /**
         * @brief Get all values for a specific header.
         * @param name Header name to search for (case-insensitive)
         * @return Vector of strings containing all values for the specified header
         *
         * Retrieves all values associated with the given header name. HTTP allows
         * multiple values for the same header, so this method returns a vector
         * containing all occurrences.
         *
         * Example: For "Accept-Language: en-US,en;q=0.9,fr;q=0.8", returns
         * a vector with the complete value string.
         */
        virtual std::vector<std::string> get_header(const std::string &name) const
        {
            return request.get_header(name);
        }

        /**
         * @brief Get all headers as name-value pairs.
         * @return Vector of name-value pairs representing all HTTP headers
         *
         * Returns all HTTP headers present in the request as a vector of
         * name-value pairs. This provides access to the complete header
         * collection for processing or forwarding.
         */
        virtual std::vector<std::pair<std::string, std::string>> get_headers() const
        {
            return request.get_headers();
        }

        /**
         * @brief Get the request body content.
         * @return String containing the complete request body
         *
         * Returns the entire body of the HTTP request as a string. For POST
         * and PUT requests, this typically contains form data, JSON, XML,
         * or other payload formats. For GET requests, the body is usually empty.
         */
        virtual std::string get_body() const
        {
            return request.get_body();
        }

        /**
         * @brief Get the Content-Type header values.
         * @return Vector of strings containing Content-Type header values
         *
         * Convenience method to retrieve the Content-Type header, which indicates
         * the media type of the request body. Common values include
         * "application/json", "application/x-www-form-urlencoded", "text/plain", etc.
         */
        virtual std::vector<std::string> get_content_type() const
        {
            return request.get_header(hh_http::HEADER_CONTENT_TYPE);
        }

        /**
         * @brief Get the Cookie header values.
         * @return Vector of strings containing Cookie header values
         *
         * Convenience method to retrieve all Cookie headers sent by the client.
         * Cookies are commonly used for session management, user preferences,
         * and tracking. The returned values contain the raw cookie strings
         * that may need further parsing.
         */
        virtual std::vector<std::string> get_cookies() const
        {
            return request.get_header(hh_http::HEADER_COOKIE);
        }

        /**
         * @brief Get the Authorization header values.
         * @return Vector of strings containing Authorization header values
         *
         * Convenience method to retrieve Authorization headers used for
         * authentication. Common schemes include "Basic", "Bearer", "Digest", etc.
         * The returned values contain the complete authorization strings
         * including the scheme and credentials.
         */
        virtual std::vector<std::string> get_authorization() const
        {
            return request.get_header(hh_http::HEADER_AUTHORIZATION);
        }

        virtual bool keep_alive() const
        {
            auto req_connection_values = get_header(hh_http::HEADER_CONNECTION);

            // transform all the values to upper case
            for (auto &s : req_connection_values)
            {
                std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c)
                               { return std::toupper(c); });
            }

            std::string keep_alive = "KEEP-ALIVE";
            if (std::find(req_connection_values.begin(), req_connection_values.end(), keep_alive) != req_connection_values.end())
            {
                return true;
            }

            return false;
        }
        /**
         * @brief Add a custom request parameter.
         * @note This parameter is ment to be set by the user of the request.
         * @param key The parameter name.
         * @param value The parameter value.
         */
        virtual void set_param(const std::string &key, const std::string &value)
        {
            request_params[key] = value;
        }

        /**
         * @brief Get a request parameter.
         *
         * @param key The parameter name.
         * @return std::string The parameter value.
         */
        virtual std::string get_param(const std::string &key) const
        {
            auto it = request_params.find(key);
            if (it != request_params.end())
            {
                return it->second;
            }
            return "";
        }

        /**
         * @brief Get all request parameters.
         *
         * @return std::map<std::string, std::string> A map of all request parameters.
         */
        virtual std::map<std::string, std::string> get_params() const
        {
            return request_params;
        }

        /**
         * @brief Clear all request parameters.
         *
         */
        virtual void clear_params()
        {
            request_params.clear();
        }

        /**
         * @brief Remove a request parameter.
         *
         * @param key The parameter name.
         */
        virtual void remove_param(const std::string &key)
        {
            request_params.erase(key);
        }
    };
}