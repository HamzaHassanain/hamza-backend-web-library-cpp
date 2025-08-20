#pragma once
#include <functional>
#include <memory>
#include <initializer_list>
#include <http-server/includes/http_request.hpp>
#include <http-server/includes/http_response.hpp>
#include <http-server/includes/exceptions.hpp>
#include <web_response.hpp>
#include <web_request.hpp>
#include <web_exceptions.hpp>
namespace hamza_web
{
    enum class exit_code
    {
        EXIT = 1,
        CONTINUE = 0,
        ERROR = -1
    };
    using http_request_callback_t = std::function<void(hamza_http::http_request &, hamza_http::http_response &)>;
    using web_listen_success_callback_t = std::function<void()>;
    using web_error_callback_t = std::function<void(std::shared_ptr<hamza_web::web_general_exception>)>;

    template <typename RequestType = web_request, typename ResponseType = web_response>
    using web_request_handler_t = std::function<exit_code(std::shared_ptr<RequestType>, std::shared_ptr<ResponseType>)>;

};