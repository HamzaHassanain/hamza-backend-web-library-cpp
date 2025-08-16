#pragma once
#include <functional>
#include <memory>
#include <initializer_list>
#include <http-server/includes/http_objects.hpp>
#include <http-server/includes/exceptions.hpp>
#include <web_response.hpp>
#include <web_request.hpp>
#include <web_exceptions.hpp>
namespace hamza_web
{
    using http_request_callback_t = std::function<void(hamza_http::http_request &, hamza_http::http_response &)>;
    using web_listen_success_callback_t = std::function<void()>;
    using web_error_callback_t = std::function<void(std::shared_ptr<web_general_exception>)>;

    template <typename RequestType = web_request, typename ResponseType = web_response>
    using web_request_handler_t = std::function<int(std::shared_ptr<RequestType>, std::shared_ptr<ResponseType>)>;

    const int EXIT = 1;
    const int CONTINUE = 0;
    const int ERROR = -1;
};