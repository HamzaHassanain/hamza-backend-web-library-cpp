#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <type_traits>
#include <web_types.hpp>
#include <web_exceptions.hpp>
#include <web_utilities.hpp>

namespace hamza_web
{
    template <typename RequestType, typename ResponseType>
    class web_router; // Forward declaration

    /**
     * @brief Template class representing a web route with request handlers.
     *
     * This class encapsulates a web route definition that consists of an HTTP method,
     * a path expression (which may include wildcards or parameters), and a collection
     * of request handlers. It provides the foundation for URL routing in web applications
     * by matching incoming requests against route patterns and executing associated handlers.
     *
     * The class is templated to support custom request and response types while maintaining
     * type safety through compile-time checks. It supports move semantics for efficient
     * route management and prevents copying to ensure unique route ownership.
     *
     * Routes can handle complex path patterns including:
     * - Static paths: "/api/users"
     * - Parameterized paths: "/api/users/:id"
     * - With Query parameters: "/api/users?id=123"
     * - Multiple handlers for middleware chains
     *
     * @tparam RequestType Type for request objects (must derive from web_request)
     * @tparam ResponseType Type for response objects (must derive from web_response)
     *
     * Example usage:
     * @code
     * // Create handlers
     * auto auth_middleware = [](const web_request& req, web_response& res) {
     *     // Authentication logic
     * };
     * auto user_handler = [](const web_request& req, web_response& res) {
     *     res.send_json("{\"user\": \"data\"}");
     * };
     *
     * // Create route with multiple handlers
     * std::vector<web_request_handler_t<web_request, web_response>> handlers = {
     *     auth_middleware, user_handler
     * };
     * web_route<> route("GET", "/api/users/:id", handlers);
     * @endcode
     */
    template <typename RequestType = web_request, typename ResponseType = web_response>
    class web_route
    {
        /// HTTP method for this route (GET, POST, PUT, DELETE, etc.)
        std::string method;

        /// Path expression/pattern for route matching (may include parameters)
        std::string expression;

        /// Collection of request handlers executed in sequence for this route
        std::vector<web_request_handler_t<RequestType, ResponseType>> handlers;

    public:
        /// Allow web_router to access private members
        friend class web_router<RequestType, ResponseType>;

        /**
         * @brief Construct a web route with method, path expression, and handlers.
         * @param method HTTP method this route responds to (e.g., "GET", "POST", "PUT", "DELETE")
         * @param expression Path pattern for route matching (e.g., "/api/users/:id")
         * @param handlers Vector of request handlers to execute for this route
         *
         * Creates a new web route that will match requests with the specified HTTP method
         * and path pattern. The handlers are executed in sequence when a request matches
         * this route, allowing for middleware chains and complex request processing.
         *
         * The constructor performs compile-time type checking to ensure RequestType and
         * ResponseType derive from the base web_request and web_response classes respectively.
         * It also validates that at least one handler is provided.
         *
         * @throws std::invalid_argument if no handlers are provided
         */
        web_route(const std::string &method, const std::string &expression, const std::vector<web_request_handler_t<RequestType, ResponseType>> &handlers)
            : method(method), expression(expression), handlers(std::move(handlers))
        {
            static_assert(std::is_base_of<web_request, RequestType>::value, "RequestType must derive from web_request");
            static_assert(std::is_base_of<web_response, ResponseType>::value, "ResponseType must derive from web_response");
            if (this->handlers.size() == 0)
            {
                throw std::invalid_argument("At least one handler must be provided");
            }
        }

        // Copy operations - DELETED for resource safety and unique ownership
        web_route(const web_route &) = delete;
        web_route &operator=(const web_route &) = delete;

        // Move operations - ENABLED for ownership transfer
        web_route(web_route &&) = default;
        web_route &operator=(web_route &&) = default;

        /**
         * @brief Get the path expression/pattern for this route.
         * @return String containing the path pattern used for route matching
         *
         * Returns the path expression that defines what URLs this route will match.
         * This includes any parameter placeholders or wildcard patterns that were
         * specified when the route was created.
         */
        std::string get_path() const
        {
            return expression;
        }

        /**
         * @brief Get the HTTP method for this route.
         * @return String containing the HTTP method (GET, POST, PUT, DELETE, etc.)
         *
         * Returns the HTTP method that this route is configured to handle.
         * Only requests with matching methods will be processed by this route.
         */
        std::string get_method() const
        {
            return method;
        }

        /**
         * @brief Check if this route matches the given method and path.
         * @param method HTTP method to match against
         * @param path Request path to match against
         * @return True if the route matches, false otherwise
         *
         * Performs route matching by comparing the provided HTTP method and path
         * against this route's configured method and path expression. The method
         * comparison is exact, while the path comparison supports pattern matching
         * including parameters and wildcards.
         *
         * This method uses the match_path utility function to handle complex
         * path patterns and parameter extraction during the matching process.
         */
        virtual bool match(const std::string &method, const std::string &path) const
        {
            return this->method == method && match_path(this->expression, path);
        }

        /// Default virtual destructor for proper cleanup in inheritance hierarchies
        ~web_route() = default;
    };
}