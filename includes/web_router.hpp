#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <type_traits>

#include "web_types.hpp"
#include "web_exceptions.hpp"
#include "web_request.hpp"
#include "web_response.hpp"
#include "web_methods.hpp"

namespace hh_web
{
    template <typename T, typename G>
    class web_route; // Forward declaration

    template <typename T, typename G>
    class web_server; // Forward declaration

    /**
     * @brief Template class for managing web routes and middleware chains.
     *
     * This class provides a comprehensive routing system for web applications by managing
     * collections of routes and middleware handlers. It implements a request processing
     * pipeline where middleware executes first (for cross-cutting concerns like authentication,
     * logging, CORS), followed by route-specific handlers.
     *
     * The router supports:
     * - Middleware chains for cross-cutting concerns
     * - Route matching based on HTTP method and path patterns
     * - Handler execution with proper error handling
     * - Type safety through template parameters
     *
     * The processing flow follows this sequence:
     * 1. Execute middleware handlers in registration order
     * 2. If middleware completes successfully, match and execute route handlers
     * 3. Handle exceptions and convert them to appropriate HTTP responses
     * 4. Return success/failure status to the web server
     *
     * @tparam T Type for request objects (must derive from web_request)
     * @tparam G Type for response objects (must derive from web_response)

     */
    template <typename T = web_request, typename G = web_response>
    class web_router
    {
    protected:
        /// Collection of registered routes for handling specific path patterns
        std::vector<std::shared_ptr<web_route<T, G>>> routes;

        /// Collection of middleware handlers executed before route processing
        std::vector<web_request_handler_t<T, G>> middlewares;

        /**
         * @brief Execute all registered middleware handlers in sequence.
         * @param request Shared pointer to the request object
         * @param response Shared pointer to the response object
         * @return exit_code indicating the result of middleware processing
         *
         * Executes middleware handlers in the order they were registered. Middleware
         * can perform cross-cutting concerns like authentication, logging, request
         * validation, CORS handling, etc. Each middleware can return:
         * - CONTINUE: Proceed to the next middleware or route processing
         * - EXIT: Stop processing and finalize the response
         * - ERROR: Indicate an error condition
         *
         * If any middleware returns EXIT or ERROR, processing stops immediately.
         * All middleware must return a valid exit_code or a runtime_error is thrown.
         *
         * Common middleware use cases:
         * - Authentication and authorization
         * - Request logging and metrics
         * - CORS headers
         * - Rate limiting
         * - Request body parsing and validation
         */
        virtual exit_code middleware_handle_request(std::shared_ptr<T> request, std::shared_ptr<G> response)
        {
            for (const auto &middleware : middlewares)
            {
                auto result = middleware(request, response);
                if (result == exit_code::EXIT)
                {
                    return exit_code::EXIT;
                }

                else if (result == exit_code::ERROR)
                {
                    return exit_code::ERROR;
                }

                else if (result == exit_code::CONTINUE)
                {
                    continue;
                }
                else
                {
                    throw std::runtime_error("Invalid middleware, return value must of  web_hh_socket::exit_code\n");
                }
            }

            return exit_code::CONTINUE;
        }

    public:
        /// Allow web_server to access protected members
        friend class web_server<T, G>;

        /**
         * @brief Default constructor with type safety validation.
         *
         * Creates a new web router instance with empty route and middleware collections.
         * Performs compile-time type checking to ensure T and G
         * derive from the required base classes (web_request and web_response respectively).
         *
         * The static assertions prevent template instantiation with incompatible types,
         * ensuring type safety throughout the routing system.
         */
        web_router()
        {
            static_assert(std::is_base_of<web_request, T>::value, "T must derive from web_request");
            static_assert(std::is_base_of<web_response, G>::value, "G must derive from web_response");
        }

        /**
         * @brief Handle an incoming request through the routing pipeline.
         * @param request Shared pointer to the request object
         * @param response Shared pointer to the response object
         * @return True if request was handled, false if no routes matched
         *
         * This is the main entry point for request processing. It orchestrates the
         * complete request handling pipeline:
         *
         * 1. **Middleware Processing**: Executes all registered middleware in order
         * 2. **Route Matching**: If middleware allows, attempts to match and execute routes
         * 3. **Exception Handling**: Catches and converts exceptions to HTTP responses
         * 4. **Status Reporting**: Returns whether the request was successfully handled
         *
         * The method provides comprehensive error handling for both web-specific
         * exceptions (with proper HTTP status codes) and general exceptions
         * (converted to 500 Internal Server Error).
         *
         * Return value semantics:
         * - true: Request was handled (middleware or route processed it)
         * - false: No routes matched and middleware didn't handle the request
         *
         * Exception handling:
         * - web_exception: Converted to HTTP response with proper status code
         * - std::exception: Converted to 500 Internal Server Error response
         *
         * @note This method is typically called by the web_server for each incoming request
         */
        virtual bool handle_request(std::shared_ptr<T> request, std::shared_ptr<G> response)
        {
            try
            {
                exit_code middleware_result = middleware_handle_request(request, response);
                if (middleware_result != exit_code::CONTINUE)
                {
                    return true;
                }
                // If middleware allows, try to match routes
                for (const auto &route : routes)
                {
                    if (route->match(request))
                    {
                        route->handle_request(request, response);
                        return true;
                    }
                }

                return false;
            }
            catch (web_exception &e) // Unhandled exception thrown from middleware/route handler
            {
                logger::error("Web error in router: " + std::string(e.what()));
                logger::error("Status code: " + std::to_string(e.get_status_code()) + " Message: " + e.get_status_message());
                throw;
            }
            catch (const std::exception &e) // Unhandled exception
            {
                logger::error("Unhandled exception in router: " + std::string(e.what()));
                throw;
            }
        }

        /**
         * @brief Register a new route with the router.
         * @param route Shared pointer to the route object to register
         *
         * Adds a new route to the router's collection. Routes are matched in the
         * order they are registered, so route ordering can be important for
         * overlapping patterns. The first matching route will be executed.
         *
         * The route must have a non-empty path expression, or an invalid_argument
         * exception will be thrown.
         *
         * @throws std::invalid_argument if the route path is empty
         */
        virtual void register_route(std::shared_ptr<web_route<T, G>> route)
        {
            if (route->get_path().empty())
            {
                throw std::invalid_argument("Route path cannot be empty");
            }
            routes.push_back(route);
        }

        /**
         * @brief Register a middleware handler with the router.
         * @param middleware Function object that implements middleware logic
         *
         * Adds a middleware handler to the router's middleware chain. Middleware
         * is executed in the order it's registered, before any route handlers.
         */
        virtual void register_middleware(const web_request_handler_t<T, G> &middleware)
        {
            middlewares.push_back(middleware);
        }
    };
}
