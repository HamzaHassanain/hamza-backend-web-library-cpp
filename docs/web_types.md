# web_types

Source: `includes/web_types.hpp`

This header defines framework-level types and function aliases used throughout the web framework: the `exit_code` enum and several std::function aliases for request/response callbacks and hooks.

## Types

- `enum class exit_code`

  - EXIT = 1
  - CONTINUE = 0
  - \_ERROR = -1

  Purpose: handler return values (middleware and route handlers) use `exit_code` to control routing and pipeline flow.

  - `CONTINUE` — proceed to next middleware/handler or route matching.
  - `EXIT` — stop processing immediately; the response is considered complete or will be finalized by the callee.
  - `_ERROR` — signal an error; processing should stop and upstream error handling should occur.

  Notes:

  - Handlers and middleware must return one of these values; the router and route code defensively throw if an invalid value is returned.

## Function aliases and callbacks

- `using http_request_callback_t = std::function<void(hh_http::http_request &, hh_http::http_response &)>;`

  - Signature used by the low-level HTTP server integration when exposing raw request/response callbacks.

- `using web_listen_callback_t = std::function<void()>;`

  - Callback invoked on successful listen.

- `using web_error_callback_t = std::function<void(const std::exception &)>;`

  - Callback used to report low-level server exceptions to application code.

- `template <typename T = web_request, typename G = web_response>
using web_unhandled_exception_callback_t = std::function<void(std::shared_ptr<T>, std::shared_ptr<G>, const web_exception &)>;`

  - Hook for application code to handle unhandled `web_exception`s with typed request/response objects.
  - Default template parameters allow using framework defaults.

- `template <typename T = web_request, typename G = web_response>
using web_request_handler_t = std::function<exit_code(std::shared_ptr<T>, std::shared_ptr<G>)>;`
  - Canonical handler signature used for both middleware and route handlers.
  - Handlers receive shared_ptrs to request/response objects, are expected to be synchronous, and must return `exit_code`.

## Examples

Middleware example:

```cpp
hh_web::web_request_handler_t<> logger = [](std::shared_ptr<hh_web::web_request> req, std::shared_ptr<hh_web::web_response> res) -> hh_web::exit_code {
    logger::info(req->get_method() + " " + req->get_uri());
    return hh_web::exit_code::CONTINUE;
};
```

Route handler example:

```cpp
hh_web::web_request_handler_t<> hello = [](std::shared_ptr<hh_web::web_request> req, std::shared_ptr<hh_web::web_response> res) -> hh_web::exit_code {
    res->send_text("Hello");
    return hh_web::exit_code::EXIT;
};
```

Unhandled exception callback example:

```cpp
hh_web::web_unhandled_exception_callback_t<> on_exc = [](std::shared_ptr<hh_web::web_request> req, std::shared_ptr<hh_web::web_response> res, const hh_web::web_exception &e){
    res->set_status(e.get_status_code(), e.get_status_message());
    res->send_text(e.what());
};
```
