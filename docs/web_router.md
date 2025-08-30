# web_router

Source: `includes/web_router.hpp`

`web_router` implements the routing and middleware pipeline for the web framework. It manages a collection of `web_route` entries and a middleware chain that runs before route handling. The class is templated to accept custom request/response types while enforcing at compile time that those types derive from the framework base classes.

## Template parameters

- `T` (default `web_request`) — request object type; must inherit from `hh_web::web_request`.
- `G` (default `web_response`) — response object type; must inherit from `hh_web::web_response`.

The constructor uses `static_assert` to prevent instantiation with incompatible types.

## Design goals

- Provide a centralized place to register routes and middleware.
- Execute middleware in order and allow it to short-circuit request processing.
- Match incoming requests to registered routes and execute route handlers.
- Convert and propagate exceptions in a consistent manner (logging + rethrow for the server to handle).

## Members

- `std::vector<std::shared_ptr<web_route<T, G>>> routes` — ordered list of routes. Routes are matched in registration order; the first match is used.
- `std::vector<web_request_handler_t<T, G>> middlewares` — ordered middleware chain executed before route matching.

## Key methods — detailed explanations and implementation notes

### `web_router()` (constructor)

- What it does:

  - Performs compile-time validation using `static_assert` to ensure the chosen template types `T` and `G` derive from `web_request` and `web_response` respectively.
  - Initializes empty `routes` and `middlewares` containers.

- Implementation details:
  - The constructor body contains two `static_assert` statements. These checks happen at compile time and will fail compilation if an incompatible type is used.
  - No runtime allocations or other side-effects occur in the constructor itself.

### `virtual exit_code middleware_handle_request(std::shared_ptr<T> request, std::shared_ptr<G> response)`

- Purpose:

  - Run the registered middleware pipeline in registration order. Middleware can examine or modify the request/response and decide whether to continue processing.

- Implementation details (control flow):

  - Iterates over `middlewares` using a simple for-loop.
  - For each `middleware`, calls `middleware(request, response)` and stores the returned `exit_code` in a local variable `result`.
  - Evaluates `result`:
    - If `result == exit_code::EXIT`, the function returns `exit_code::EXIT` immediately — middleware decided to finish processing (often after writing a response).
    - If `result == exit_code::_ERROR`, the function returns `exit_code::_ERROR` immediately — a middleware signaled an error.
    - If `result == exit_code::CONTINUE`, the loop continues to the next middleware.
    - For any other (unexpected) value, the implementation throws `std::runtime_error("Invalid middleware, return value must of  web_hh_socket::exit_code\n")` as a defensive check.
  - If the loop finishes without `EXIT` or `_ERROR`, the function returns `exit_code::CONTINUE` indicating that route matching should proceed.

- Notes and implications:
  - Middleware is synchronous and executed in the route thread.
  - Because the implementation throws on invalid return values, middleware must always comply with the `exit_code` contract.

### `virtual bool handle_request(std::shared_ptr<T> request, std::shared_ptr<G> response)`

- Purpose:

  - Main entry point to process an incoming request through middleware and registered routes. Returns whether the request was handled.

- Implementation details (high-level flow):

  1. Calls `middleware_handle_request(request, response)` and stores the result in `middleware_result`.
     - If `middleware_result != exit_code::CONTINUE`, the function returns `true` (middleware handled or aborted the request).
  2. If middleware returned `CONTINUE`, iterates over `routes` in insertion order.
     - For each route, calls `route->match(request)`.
     - If `match` returns `true`, calls `route->handle_request(request, response)` and immediately returns `true` (first matching route is used).
  3. If the loop finishes without finding a matching route, returns `false` (request was not handled by router).

- Exception handling implementation details:

  - The whole processing block is wrapped in `try { ... }`.
  - There are two catch clauses:
    - `catch (web_exception &e)`: logs the formatted error using `logger::error` (both `e.what()` and `e.get_status_code()`/`e.get_status_message()`), then `throw;` — rethrows the caught exception for the server layer to convert into an HTTP response.
    - `catch (const std::exception &e)`: logs the unhandled exception and rethrows it as well.

- Notes and implications:
  - Rethrowing means `web_server` must catch exceptions from `web_router::handle_request` and use `web_exception` details (when available) to produce appropriate HTTP responses.
  - Because the router rethrows after logging, the logs will always show the exception once here and the server can add a response-specific log or metric.
  - Route matching is done in registration order; the first route that returns `true` from `match` will have its handlers executed. This makes route registration order significant for overlapping patterns.

### `virtual void add_route(std::shared_ptr<web_route<T, G>> route)`

- Purpose:

  - Register a new route with the router.

- Implementation details:

  - Validates that `route->get_path()` is not empty. If the path is empty, throws `std::invalid_argument("Route path cannot be empty")`.
  - Calls `routes.push_back(route)` to add the route at the end of the collection.

- Notes:
  - Because routes are matched in insertion order, calling `add_route` later will make the route lower priority than earlier registrations.

### `virtual void use(const web_request_handler_t<T, G> &middleware)`

- Purpose:

  - Register a middleware handler to be executed before route matching.

- Implementation details:

  - Appends the provided middleware callable to the `middlewares` vector via `middlewares.push_back(middleware)`.

- Notes:
  - The middleware callable must conform to the expected handler signature and must always return a valid `exit_code`.

### Convenience route registration helpers: `get`, `post`, `put`, `delete_`

- Purpose:

  - Provide ergonomic functions to create and register a `web_route` for the specific HTTP method.

- Implementation details:

  - Each helper constructs a `std::make_shared<web_route<T, G>>(method, path, handlers)` and passes it to `add_route(...)`.
  - Example: `get(path, handlers)` constructs a route with method string "GET".

- Notes:
  - These helpers accept a `std::vector<web_request_handler_t<T, G>> handlers` parameter and forward it to the `web_route` constructor. As discussed in `web_route` docs, moving handler vectors from the caller can be optimized by changing parameter passing convention if necessary.

## Error handling specifics

- Contract: middleware and handlers must return a valid `exit_code`. If they do not, `web_router` or `web_route` will throw `std::runtime_error`.
- `web_exception` thrown by middleware or handlers is caught by `handle_request`, logged, and rethrown to the server layer. This preserves the HTTP semantics embedded in `web_exception` (status code and message) for the server to serialize.
- Generic `std::exception` is also caught, logged, and rethrown, and the server should map this to a 500 Internal Server Error response if not otherwise handled.

## Concurrency and thread-safety notes

- `web_router` itself does not use synchronization. The design assumes the `web_server` will call `handle_request` concurrently from worker threads if desired.
- Registration methods (`add_route`, `use`, and the convenience helpers) mutate internal vectors and are not synchronized in the implementation. If your application registers routes/middleware while requests are being processed concurrently, you must externally synchronize access to `web_router` or perform registration during server startup before any requests are handled.

## Performance notes

- The router performs linear scans over middleware and routes. For applications with large numbers of routes, consider organizing routes into a more efficient structure (trie, radix tree) or partitioning routers by prefix.
- `handle_request` uses early exits: middleware can short-circuit and the first matching route stops further matching. Place high-priority and specific routes earlier in registration order.

## Examples

Registering middleware and routes:

```cpp
hh_web::web_router<> router;

// Middleware: logging
router.use([](std::shared_ptr<hh_web::web_request> req, std::shared_ptr<hh_web::web_response> res) -> hh_web::exit_code {
    logger::info("Incoming: " + req->get_method() + " " + req->get_uri());
    return hh_web::exit_code::CONTINUE;
});

// Route handlers
auto h1 = [](std::shared_ptr<hh_web::web_request> req, std::shared_ptr<hh_web::web_response> res) -> hh_web::exit_code {
    res->send_text("Hello");
    return hh_web::exit_code::EXIT;
};

router.get("/hello", std::vector<hh_web::web_request_handler_t<>>{h1});
```

Custom request/response types example:

```cpp
struct my_request : public hh_web::web_request { using web_request::web_request; };
struct my_response : public hh_web::web_response { using web_response::web_response; };

hh_web::web_router<my_request, my_response> r;

r.use([](std::shared_ptr<my_request> req, std::shared_ptr<my_response> res) -> hh_web::exit_code {
    // typed middleware
    return hh_web::exit_code::CONTINUE;
});
```

## Relationship with `web_server`

- `web_router` is typically owned by `web_server`. The server constructs request/response objects, passes them to `web_router::handle_request`, and is responsible for catching rethrown exceptions and producing final HTTP responses.
- `web_router` declares `web_server` as a friend to allow the server to access internals when necessary.
