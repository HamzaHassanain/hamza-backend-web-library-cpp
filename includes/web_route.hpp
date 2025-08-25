#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <type_traits>
#include "web_types.hpp"
#include "web_exceptions.hpp"
#include "web_utilities.hpp"

namespace hamza_web
{
    template <typename T, typename G>
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
     * type safety through compile-time checks.
     *
     * Routes can handle complex path patterns including:
     * - Static paths: "/api/users"
     * - Parameterized paths: "/api/users/:id"
     * - With Query parameters: "/api/users?id=123"
     * - Multiple handlers for middleware chains
     *
     * @tparam T Type for request objects (must derive from web_request)
     * @tparam G Type for response objects (must derive from web_response)

     */
    template <typename T = web_request, typename G = web_response>
    class web_route
    {
    protected:
        /// HTTP method for this route (GET, POST, PUT, DELETE, etc.)
        std::string method;

        /// Path expression/pattern for route matching (may include parameters)
        std::string expression;

        /// Collection of request handlers executed in sequence for this route
        std::vector<web_request_handler_t<T, G>> handlers;

    public:
        /// Allow web_router to access private members
        friend class web_router<T, G>;

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
         * The constructor performs compile-time type checking to ensure T and
         * G derive from the base web_request and web_response classes respectively.
         * It also validates that at least one handler is provided.
         *
         * @throws std::invalid_argument if no handlers are provided
         */
        web_route(const std::string &method, const std::string &expression, const std::vector<web_request_handler_t<T, G>> &handlers)
            : method(method), expression(expression), handlers(std::move(handlers))
        {
            static_assert(std::is_base_of<web_request, T>::value, "T must derive from web_request");
            static_assert(std::is_base_of<web_response, G>::value, "G must derive from web_response");
            if (this->handlers.size() == 0)
            {
                throw std::invalid_argument("At least one handler must be provided");
            }
        }

        /**
         * @brief Get the path expression/pattern for this route.
         * @return String containing the path pattern used for route matching
         *
         * Returns the path expression that defines what URLs this route will match.
         * This includes any parameter placeholders or wildcard patterns that were
         * specified when the route was created.
         */
        virtual std::string get_path() const
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
        virtual std::string get_method() const
        {
            return method;
        }

        /**
         * @brief Check if this route matches the given method and path.
         * @param request Shared pointer to the request object
         * @return True if the route matches, false otherwise
         *
         * Performs route matching by comparing the provided HTTP method and path
         * against this route's configured method and path expression, then sets the path parameters if
         * existing ones are found.
         *
         * @note This function is called by the web_router class to determine
         * if a request matches this route.
         */
        virtual bool match(std::shared_ptr<T> request) const
        {
            auto [matched, path_params] = match_path(this->expression, request->get_path());
            if (matched)
            {
                request->set_path_params(path_params);
            }
            return this->method == request->get_method() && matched;
        }

        /**
         * @brief Match request against routes and execute matching route handlers.
         * @param request Shared pointer to the request object
         * @param response Shared pointer to the response object
         * @return exit_code indicating the result of route processing
         *
         * Executes all handlers associated with that route in the order they were registered.
         *
         * Route handlers can return:
         * - EXIT: Stop processing and finalize the response
         * - ERROR: Indicate an error condition
         *
         *@note This function is called by the web_router class if a matching route is found.
         *@note This is a const member function, meaning it does not modify the state of the web_route instance.
         */
        virtual exit_code handle_request(std::shared_ptr<T> request, std::shared_ptr<G> response) const
        {
            for (const auto &handler : handlers)
            {
                auto resp = handler(request, response);
                if (resp == exit_code::EXIT)
                {
                    return exit_code::EXIT;
                }
                else if (resp == exit_code::ERROR)
                {
                    return exit_code::ERROR;
                }
                else if (resp == exit_code::CONTINUE)
                {
                    continue;
                }
                else
                {
                    throw std::runtime_error("Invalid route handler, return value must of  web_hamza_socket::exit_code\n");
                }
            }

            return exit_code::EXIT;
        }

        /// Default virtual destructor for proper cleanup in inheritance hierarchies
        ~web_route() = default;
    };
}