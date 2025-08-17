#pragma once

#include <string>
#include <vector>
#include <memory>
#include <web_types.hpp>
#include <web_exceptions.hpp>
#include <web_methods.hpp>
#include <iostream>

namespace hamza_web
{
    template <typename RequestType, typename ResponseType>
    class web_route; // Forward declaration

    template <typename RequestType, typename ResponseType>
    class web_server; // Forward declaration

    template <typename RequestType = web_request, typename ResponseType = web_response>
    class web_router
    {
        std::vector<std::shared_ptr<web_route<RequestType, ResponseType>>> routes;
        std::vector<web_request_handler_t<RequestType, ResponseType>> middlewares;

    public:
        friend class web_server<RequestType, ResponseType>;

        web_router() = default;

        web_router(const web_router &) = delete;
        web_router &operator=(const web_router &) = delete;
        web_router(web_router &&) = default;
        web_router &operator=(web_router &&) = default;

        bool handle_request(std::shared_ptr<RequestType> request, std::shared_ptr<ResponseType> response)
        {
            try
            {
                for (const auto &middleware : middlewares)
                {
                    auto result = middleware(request, response);
                    if (result == EXIT)
                    {
                        response->end();
                        return true;
                    }

                    if (result == ERROR)
                    {
                        throw web_general_exception("Middleware returned an error");
                    }

                    if (result != CONTINUE)
                    {
                        return true;
                    }
                }

                for (const auto &route : routes)
                {
                    if (route->match(request->get_path(), request->get_method()))
                    {
                        auto &handlers = route->handlers;
                        for (const auto &handler : handlers)
                        {
                            auto resp = handler(request, response);
                            if (resp == EXIT)
                            {
                                response->end();
                                return true;
                            }
                            if (resp == ERROR)
                            {
                                throw web_general_exception("Handler returned an error");
                            }
                        }
                    }
                }

                return false;
            }
            catch (const web_general_exception &e)
            {
                std::cerr << "Error handling request: " << e.what() << std::endl;
                response->set_status(e.get_status_code(), e.get_status_message());
                response->text(e.what());
                response->end();
                return true;
            }
        }

        void register_route(std::shared_ptr<web_route<RequestType, ResponseType>> route)
        {
            if (route->get_path().empty())
            {
                throw std::invalid_argument("Route path cannot be empty");
            }
            routes.push_back(route);
        }

        void register_middleware(const web_request_handler_t<RequestType, ResponseType> &middleware)
        {
            middlewares.push_back(middleware);
        }
    };
}

#include <web_route.hpp> // Include after the template declaration