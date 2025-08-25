#pragma once
#include <functional>
#include <memory>
#include <initializer_list>

#include "../libs/http-server/http-lib.hpp"
#include "web_response.hpp"
#include "web_request.hpp"
#include "web_exceptions.hpp"

namespace hamza_web
{
    enum class exit_code
    {
        EXIT = 1,
        CONTINUE = 0,
        ERROR = -1
    };
    using http_request_callback_t = std::function<void(hamza_http::http_request &, hamza_http::http_response &)>;
    using web_listen_callback_t = std::function<void()>;
    using web_error_callback_t = std::function<void(const std::exception &)>;

    template <typename T = web_request, typename G = web_response>
    using web_unhandled_exception_callback_t = std::function<void(std::shared_ptr<T>, std::shared_ptr<G>, const web_exception &)>;

    template <typename T = web_request, typename G = web_response>
    using web_request_handler_t = std::function<exit_code(std::shared_ptr<T>, std::shared_ptr<G>)>;

};