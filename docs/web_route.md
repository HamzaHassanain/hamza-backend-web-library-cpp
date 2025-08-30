# web_route

Source: `includes/web_route.hpp`

`web_route` is a templated representation of a single route (HTTP method + path pattern) and the ordered list of handlers that should be executed when a request matches the route. It is a core building block of the framework's routing system and is designed to be generic so you can use custom request/response types.

## Template parameters

- `T` (default `web_request`) — request object type. Must derive from `hh_web::web_request`.
- `G` (default `web_response`) — response object type. Must derive from `hh_web::web_response`.

The class uses `static_assert` in its constructor to enforce at compile time that `T` and `G` inherit from the base `web_request` and `web_response` types respectively. This ensures route handlers always receive compatible objects and lets handlers use base APIs safely.

## Purpose and responsibilities

- Store the HTTP method (e.g. "GET", "POST") and the path expression/pattern (e.g. "/users/:id").
- Hold an ordered collection of handlers (middleware chain) that will be executed when a request matches the route.
- Perform path matching, populate `path_params` on the request, and drive handler execution.
- Be usable by the `web_router` to locate and run matching routes.

## Key members

- `std::string method` — HTTP method this route responds to.
- `std::string expression` — path pattern used for matching (may include parameter placeholders like `:id`).
- `std::vector<web_request_handler_t<T, G>> handlers` — ordered handler list. The type alias `web_request_handler_t<T, G>` is the framework's handler signature (typically a callable that accepts `std::shared_ptr<T>` and `std::shared_ptr<G>` and returns an `exit_code`).

## Constructor

Signature:

`web_route(const std::string &method, const std::string &expression, const std::vector<web_request_handler_t<T, G>> &handlers)`

Behavior and notes:

- Validates at compile time that `T` and `G` derive from the expected base types using `static_assert`.
- Validates at runtime that at least one handler is provided; throws `std::invalid_argument` if `handlers.size() == 0`.
- The implementation performs `handlers(std::move(handlers))` when initializing the member. Because the constructor parameter is a const-reference, `std::move` will not yield a true non-const rvalue that can be efficiently moved from; this works (produces a copy) but is a possible micro-optimization point: accepting the parameter by value (and then moving) or by rvalue-reference would allow the caller to move handlers cheaply.

## Matching: `match(std::shared_ptr<T> request) const`

Purpose:

- Determine whether this route should handle the provided `request` by comparing the HTTP method and pattern-matching the request path.

Behavior:

1. Calls `match_path(this->expression, request->get_path())` (a utility from `web_utilities.hpp`) which attempts to match the route expression against the request path and returns a pair `(matched, path_params)`.
2. If `matched` is true it calls `request->set_path_params(path_params)` to populate extracted path parameters onto the request object. Example: expression `/users/:id` and path `/users/123` will result in `path_params = { {"id","123"} }`.
3. Returns `true` only if both the HTTP method matches (`this->method == request->get_method()`) and `matched` is true.
4. Current implementation does not support wildcard matching or complex parameter patterns (e.g. regex). This is a potential area for future enhancement. only the simplest path parameters (e.g. `/users/:id`) are supported.

Notes:

- `match_path` encapsulates the parsing/matching semantics (parameter syntax, wildcard handling). Because `match` always calls `match_path` before checking the method, the path-parameter population will occur even if the method does not match. This is harmless but worth noting for performance-sensitive code.

## Handling requests: `handle_request(std::shared_ptr<T> request, std::shared_ptr<G> response) const`

Purpose:

- Execute the route's handler chain for a request that matched the route. This function drives the middleware/handler execution model.

Behavior:

- Iterates over `handlers` in registration order.
- For each handler, it calls the handler with `(request, response)` and inspects the returned `exit_code` value.

Expected handler return values (from `web_types.hpp`):

- `exit_code::CONTINUE` — the route processing should continue to the next handler in the chain.
- `exit_code::EXIT` — processing should stop; the route handler chain is considered finished (the response is typically complete or will be finalized by the framework).
- `exit_code::_ERROR` — an error occurred; the route processing stops and the framework should handle error propagation/response generation.

- If a handler returns an unknown value, `handle_request` throws `std::runtime_error` to indicate an invalid handler implementation.
- If the loop finishes without an explicit `EXIT` or `_ERROR`, `handle_request` returns `exit_code::EXIT` by default (meaning the route chain ended and processing stops).

Notes:

- Handlers are executed synchronously in the order they were registered. A handler can decide to short-circuit the chain by returning `EXIT`.
- Because `handlers` holds callables templated on `T` and `G`, the compile-time checks on the constructor ensure handlers will receive compatible types.

## Template usage examples

Default route with lambda handlers (using default `web_request`/`web_response`):

```cpp
using route_t = hh_web::web_route<>; // T=web_request, G=web_response

auto handler1 = [](std::shared_ptr<hh_web::web_request> req, std::shared_ptr<hh_web::web_response> res) -> hh_web::exit_code {
    // do some pre-processing
    return hh_web::exit_code::CONTINUE; // continue to next handler
};

auto handler2 = [](std::shared_ptr<hh_web::web_request> req, std::shared_ptr<hh_web::web_response> res) -> hh_web::exit_code {
    res->send_text("OK");
    return hh_web::exit_code::EXIT; // finished handling
};

std::vector<route_t::web_request_handler_t<>> handlers = { handler1, handler2 };
hh_web::web_route<> r("GET", "/health", handlers);
```

Custom request/response types:

```cpp
struct my_request : public hh_web::web_request { using web_request::web_request; /* custom fields */ };
struct my_response : public hh_web::web_response { using web_response::web_response; /* custom helpers */ };

// Handlers must use the matching shared_ptr types
auto custom_handler = [](std::shared_ptr<my_request> req, std::shared_ptr<my_response> res) -> hh_web::exit_code {
    // can access custom fields on my_request/my_response
    return hh_web::exit_code::EXIT;
};

std::vector<hh_web::web_request_handler_t<my_request, my_response>> hs = { custom_handler };
hh_web::web_route<my_request, my_response> custom_route("POST", "/items/:id", hs);
```

## Implementation notes & gotchas

- Constructor parameter `handlers` is taken as a `const &` and then `std::move` is applied when initializing the member. Because the parameter is const, the effective move may fall back to a copy on many compilers. For large handler vectors, consider changing the constructor signature to accept `std::vector<...> handlers` by value (and then move), or accept an rvalue reference, to allow callers to move resources efficiently.

- `match()` calls the path-matching routine even if the HTTP method might fail; while convenient (path params set), it adds unnecessary work when many routes differ only by method. If you have many method-variant routes and performance is critical, consider checking the method first.

- The `match_path` utility is responsible for interpreting route expression syntax (parameter markers like `:name`, wildcards). The route populates `request->set_path_params` with the extracted pairs — handlers can rely on `request->get_path_params()` to retrieve those values.

- `handle_request` throws when a handler returns an unexpected value. This is a defensive check that surfaces incorrect handler implementations early.

## Relationship with `web_router`

- `web_router` is the component that holds a collection of `web_route` instances and uses `web_route::match` to find a matching route for an incoming request. Once found, the router calls `web_route::handle_request` to execute the handlers.
- `web_route` declares `web_router<T,G>` as a friend to allow the router internal access if needed.

## Summary

`web_route` is a small, carefully-typed building block for routing: it validates template parameters at compile-time, encapsulates route metadata and handlers, performs path matching and parameter extraction, and drives the handler chain with clear semantics. It can be used with the framework defaults or customized request/response types for advanced applications.
