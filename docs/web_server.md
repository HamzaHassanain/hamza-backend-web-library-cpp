# web_server

Source: `includes/web_server.hpp`

`web_server` is the high-level, multi-threaded HTTP server built on top of the project's lower-level `hh_http::http_server`. It wires together thread-pool based request processing, static file serving, router-based dynamic handling, middleware support, and exception-to-HTTP mapping. The class is templated so you can plug in custom `web_request`, `web_response`, and `web_router` types.

## Template parameters

- `T` (default `web_request`) — request object type; must derive from `hh_web::web_request`.
- `G` (default `web_response`) — response object type; must derive from `hh_web::web_response`.
- `R` (default `web_router<T, G>`) — router type; must derive from `web_router<T, G>`.

Compile-time checks (`static_assert`) in the constructor enforce these constraints so incorrect template instantiation fails at compile time.

## High-level responsibilities

- Accept raw HTTP requests from the underlying `hh_http::http_server`.
- Convert low-level `http_request`/`http_response` to framework `T`/`G` objects and dispatch them to worker threads.
- Serve static files from registered directories (with MIME type detection).
- Delegate dynamic request handling to one or more `web_router` instances.
- Provide hook points: middleware, custom 404 handler, header-received callback, and unhandled-exception callback.
- Provide lifecycle methods: `listen()` and `stop()`.

## Important members

- `thread_pool worker_pool` — pool of worker threads used to execute request handlers concurrently. Constructed with `std::thread::hardware_concurrency()` workers by default.
- `int port`, `std::string host` — listening endpoint.
- `std::vector<std::string> static_directories` — directories registered via `use_static()` for file serving.
- `std::vector<std::shared_ptr<R>> routers` — collection of routers. A default base router is created in the constructor and stored at index 0.
- Callbacks:
  - `web_listen_callback_t listen_callback` — called on successful listen (default prints host:port).
  - `web_error_callback_t error_callback` — called on low-level server errors.
  - `headers_callback` — invoked when headers are received (macro `HEADER_RECEIVED_PARAMS` describes the signature).
  - `web_unhandled_exception_callback_t<T, G> unhandled_exception_callback` — optional custom handler for unhandled web exceptions.
- `web_request_handler_t<T,G> handle_default_route` — default 404 handler; can be overridden with `use_default()`.

## Constructor and lifecycle

- ### `web_server(int port, const std::string &host = "0.0.0.0")`

  - Initializes the underlying `hh_http::http_server` and `worker_pool`.
  - Performs `static_assert` checks for template parameter correctness.
  - Creates and registers a default router `std::make_shared<R>()` in `routers[0]`.

- ### `listen(...)`

  - Optionally accept custom listen and error callbacks. Calls `hh_http::http_server::listen()` to start accepting connections.
  - When the underlying server reports successful listen, `on_listen_success()` invokes the configured `listen_callback`.

- ### `stop()`
  - Calls `hh_http::http_server::stop_server()` and stops the `worker_pool` (waits for tasks to finish and joins workers as the pool implementation requires).

## Registration / configuration methods

- #### `use_router(std::shared_ptr<web_router<T, G>> router)` — append a router to `routers`. Routers are consulted in order when handling requests.

- #### `use_static(const std::string &directory)` — register a directory to be used for static file serving. The implementation prefixes the provided directory with `CPP_PROJECT_SOURCE_DIR` (project-specific macro) before storing.

- #### `use_default(const web_request_handler_t<T, G> &handler)` — replace the default 404 handler.

- #### `use_headers_received(const std::function<void(HEADER_RECEIVED_PARAMS)> &callback)` — set a callback that will be invoked by `on_headers_received` (this allows logging, connection-closing or header-based decisions before the request body is handled).

- `use_error(web_unhandled_exception_callback_t<T, G> callback)` — set a custom handler to be invoked when an unhandled `web_exception` occurs during request processing.

## Route registration helpers (convenience)

- ### `get`, `post`, `put`, `delete_`
helper methods that create a `web_route<T,G>` for the base router (`routers[0]`) and register it. Handlers are passed as a `std::vector<web_request_handler_t<T, G>>`.

## `serve_static(std::shared_ptr<T> req, std::shared_ptr<G> res)`

- Flow and implementation notes:

  - Extracts the request `uri` and sanitizes it using `sanitize_path(uri)` (utility that should remove path-traversal attempts and normalize the path).
  - Iterates over `static_directories`, concatenating `dir + sanitized_path` to locate the file.
  - If no file found, responds with 404 via `res->set_status(404, "Not Found"); res->send_text("404 Not Found");` and returns.
  - If file found: reads it into a buffer, sets the response body, sets `Content-Type` using `get_mime_type_from_extension(get_file_extension_from_uri(uri))`, sets status `200 OK` and calls `res->send()`.
  - Catches exceptions and maps them to a `web_exception` with status 500, then delegates to `on_unhandled_exception(req, res, exp)`.

- Notes:
  - The implementation uses `std::ifstream` to detect and read files. Large file serving may need streaming / chunked transfer for production use.
  - MIME detection relies on utility helpers based on file extension.

## `request_handler(std::shared_ptr<T> req, std::shared_ptr<G> res)`

This is the worker-thread function that processes a single request end-to-end. Implementation steps:

1. Determine whether the request URI maps to a static resource via `is_uri_static(req->get_uri())`. If so, call `serve_static(req, res)` and mark the request as handled.
2. If not static, iterate over `routers` and call `router->handle_request(req, res)` for each until one returns `true` (handled). If none handled the request, call `handle_default_route(req, res)` (404 by default or user-supplied handler).
3. After handling, call `res->send(); res->end();` to ensure the response is transmitted and the underlying connection is terminated appropriately.
4. The whole flow is wrapped in a `try/catch` that logs exceptions and converts them into a `web_exception` with status 500. It then calls `on_unhandled_exception(req, res, exp)` to allow user custom handling, or the framework's default behavior which sends a 500 response and logs the error.

Notes and observations:

- The implementation calls `res->send()` and `res->end()` both inside the normal path and again after the `try/catch` block. This appears defensive to ensure the response is finalized even if some code path above skipped finalization.
- `request_handler` expects router handlers and middleware to be synchronous — they run on the worker thread. If asynchronous behavior is desired, handlers must manage their own threading and ensure that response finalization is coordinated.

## `on_request_received(hh_http::http_request &request, hh_http::http_response &response)`

- Purpose: This is the override invoked by the low-level HTTP server when a request is received. The method's job is to convert low-level objects to framework objects and enqueue the processing on the worker pool.

- Implementation details:

  1. Move-construct framework objects: `auto req = std::make_shared<T>(std::move(request)); auto res = std::make_shared<G>(std::move(response));` — this transfers ownership of the underlying low-level objects into the framework wrappers.
  2. Validate creation success; if failed, log and return.
  3. Validate HTTP method via `unknown_method(req->get_method())`. If invalid, respond with 400 Bad Request and `res->end()`.
  4. Enqueue a lambda that captures `this`, `req`, and `res` by value into `worker_pool.enqueue([this, req, res]() { request_handler(req, res); });`.
  5. If enqueue throws `web_exception` or `std::exception`, catch, log, convert to `web_exception` if needed, call `on_unhandled_exception(req, res, e)` and finalize the response with `res->send(); res->end();`.

- Notes:
  - Using `std::move(request)` / `response` is essential to avoid copying socket/connection handles and to transfer ownership into the worker thread.
  - `on_request_received` is intentionally lightweight — it should not do heavy work; worker threads handle request processing.

## Header and error callbacks

- `on_listen_success()` — invoked by the underlying server; calls `listen_callback()`.
- `on_exception_occurred(const std::exception &e)` — forwards to `error_callback(e)` to allow centralized error reporting.
- `on_headers_received(HEADER_RECEIVED_PARAMS)` — forwards to user-provided `headers_callback` if set. The `HEADER_RECEIVED_PARAMS` macro expands to a signature that includes the low-level connection object, headers multimap, method, uri, version, and body. Users may inspect or close the connection inside this callback.

## `on_unhandled_exception(std::shared_ptr<T> req, std::shared_ptr<G> res, web_exception &e)`

- Behavior:

  - If `unhandled_exception_callback` is set, call it and return (user handles the response).
  - Otherwise, set the response status to `e.get_status_code()` / `e.get_status_message()`, send a generic "Internal Server Error" body, log the exception via `logger::error`, and call `res->end()`.

- Notes:
  - This hook lets application code render detailed error pages, send structured JSON errors, or perform additional logging and cleanup.

## Thread-safety and concurrency considerations

- `web_server` relies on `worker_pool` for concurrency. The class does not internally synchronize router or static directory registration: register routers and static directories before calling `listen()` or otherwise ensure external synchronization.
- Request/response objects (`T`/`G`) are created per-request and passed to a single worker thread; they are not thread-safe by default unless explicitly designed so.

## Examples

Basic server with a route and a static dir:

```cpp
int main() {
    hh_web::web_server<> srv(8080);
    srv.use_static("/static");
    srv.get("/hello", { [](auto req, auto res) -> hh_web::exit_code { res->send_text("Hello World"); return hh_web::exit_code::EXIT; } });
    srv.listen();
}
```

Custom request/response and router types:

```cpp
struct my_request : public hh_web::web_request { using web_request::web_request; /* custom fields */ };
struct my_response : public hh_web::web_response { using web_response::web_response; /* custom helpers */ };
using my_router = hh_web::web_router<my_request, my_response>;
using my_server = hh_web::web_server<my_request, my_response, my_router>;

my_server srv(8080);
// register typed routes and middleware on srv.routers[0] or via srv.get/post helpers
```

Custom unhandled-exception hook:

```cpp
srv.use_error([](std::shared_ptr<my_request> req, std::shared_ptr<my_response> res, hh_web::web_exception &e){
    res->set_status(e.get_status_code(), e.get_status_message());
    res->set_content_type("application/json");
    res->set_body("{\"error\":\"" + e.what() + "\"}");
    res->send();
    res->end();
});
```

## Extending web_server for custom functionality

The `web_server` class is designed for extension. You can inherit from it to add application-specific functionality while preserving the core HTTP server behavior.

### Example: Custom web server with authentication and logging

```cpp
#include "web_server.hpp"
#include <unordered_set>
#include <chrono>

// Custom request type with user information
struct auth_request : public hh_web::web_request {
    using web_request::web_request;
    std::string user_id;
    std::string session_token;
    bool is_authenticated = false;
};

// Custom response type with additional security headers
struct secure_response : public hh_web::web_response {
    using web_response::web_response;

    void send_secure_json(const std::string &json_data) {
        add_header("X-Content-Type-Options", "nosniff");
        add_header("X-Frame-Options", "DENY");
        add_header("X-XSS-Protection", "1; mode=block");
        send_json(json_data);
    }
};

// Custom server with authentication and request logging
class my_app_server : public hh_web::web_server<auth_request, secure_response> {
private:
    std::unordered_set<std::string> valid_sessions;
    std::mutex sessions_mutex;

public:
    my_app_server(int port, const std::string &host = "0.0.0.0")
        : web_server(port, host) {

        // Add authentication middleware to the base router
        routers[0]->use([this](std::shared_ptr<auth_request> req, std::shared_ptr<secure_response> res) -> hh_web::exit_code {
            return authenticate_request(req, res);
        });

        // Add request logging middleware
        routers[0]->use([this](std::shared_ptr<auth_request> req, std::shared_ptr<secure_response> res) -> hh_web::exit_code {
            log_request(req);
            return hh_web::exit_code::CONTINUE;
        });
    }

    // Custom method to add valid session tokens
    void add_session(const std::string &token) {
        std::lock_guard<std::mutex> lock(sessions_mutex);
        valid_sessions.insert(token);
    }

    void remove_session(const std::string &token) {
        std::lock_guard<std::mutex> lock(sessions_mutex);
        valid_sessions.erase(token);
    }

protected:
    // Override request handler to add custom pre/post processing
    void request_handler(std::shared_ptr<auth_request> req, std::shared_ptr<secure_response> res) override {
        auto start_time = std::chrono::high_resolution_clock::now();

        // Call parent implementation
        web_server::request_handler(req, res);

        // Log response time
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        logger::info("Request processed in " + std::to_string(duration.count()) + "ms");
    }

    // Custom authentication middleware
    hh_web::exit_code authenticate_request(std::shared_ptr<auth_request> req, std::shared_ptr<secure_response> res) {
        // Skip authentication for public endpoints
        if (req->get_path().starts_with("/public/") || req->get_path() == "/login") {
            return hh_web::exit_code::CONTINUE;
        }

        // Extract session token from Authorization header
        auto auth_headers = req->get_authorization();
        if (auth_headers.empty()) {
            res->set_status(401, "Unauthorized");
            res->send_secure_json("{\"error\":\"Missing authorization header\"}");
            return hh_web::exit_code::EXIT;
        }

        std::string token = auth_headers[0];
        if (token.starts_with("Bearer ")) {
            token = token.substr(7); // Remove "Bearer " prefix
        }

        // Validate session token
        {
            std::lock_guard<std::mutex> lock(sessions_mutex);
            if (valid_sessions.find(token) == valid_sessions.end()) {
                res->set_status(401, "Unauthorized");
                res->send_secure_json("{\"error\":\"Invalid session token\"}");
                return hh_web::exit_code::EXIT;
            }
        }

        // Set authentication info on request
        req->session_token = token;
        req->is_authenticated = true;
        req->user_id = extract_user_id_from_token(token); // Your implementation

        return hh_web::exit_code::CONTINUE;
    }

    // Custom request logging
    void log_request(std::shared_ptr<auth_request> req) {
        std::string log_msg = req->get_method() + " " + req->get_uri();
        if (req->is_authenticated) {
            log_msg += " [User: " + req->user_id + "]";
        }
        logger::info("Request: " + log_msg);
    }

    // Override unhandled exception handler for custom error responses
    void on_unhandled_exception(std::shared_ptr<auth_request> req, std::shared_ptr<secure_response> res, hh_web::web_exception &e) override {
        // Log the exception with request context
        logger::error("Exception for " + req->get_method() + " " + req->get_uri() +
                     " [User: " + (req->is_authenticated ? req->user_id : "anonymous") + "]: " + e.what());

        // Send structured error response
        res->set_status(e.get_status_code(), e.get_status_message());
        res->send_secure_json("{\"error\":\"" + std::string(e.what()) + "\",\"code\":" + std::to_string(e.get_status_code()) + "}");
        res->end();
    }

private:
    std::string extract_user_id_from_token(const std::string &token) {
        // Implementation depends on your token format (JWT, custom, etc.)
        return "user_from_" + token.substr(0, 8); // Simplified example
    }
};

// Usage example
int main() {
    my_app_server server(8080);

    // Add some valid sessions (in real app, these would come from login endpoint)
    server.add_session("valid_token_123");
    server.add_session("another_token_456");

    // Register protected routes
    server.get("/api/profile", {
        [](std::shared_ptr<auth_request> req, std::shared_ptr<secure_response> res) -> hh_web::exit_code {
            // This handler knows the user is authenticated
            std::string profile_json = "{\"user_id\":\"" + req->user_id + "\",\"authenticated\":true}";
            res->send_secure_json(profile_json);
            return hh_web::exit_code::EXIT;
        }
    });

    // Register public route
    server.get("/public/health", {
        [](std::shared_ptr<auth_request> req, std::shared_ptr<secure_response> res) -> hh_web::exit_code {
            res->send_secure_json("{\"status\":\"healthy\"}");
            return hh_web::exit_code::EXIT;
        }
    });

    // Start server
    server.listen();
    return 0;
}
```

### Key extension points

1. **Custom request/response types**: Extend `web_request` and `web_response` to add application-specific data and methods.

2. **Override virtual methods**:

   - `request_handler()` for custom request processing flow
   - `serve_static()` for custom static file handling
   - `on_unhandled_exception()` for custom error handling
   - `on_headers_received()` for early request processing

3. **Custom middleware**: Add application-wide middleware to the base router for authentication, logging, CORS, etc.

4. **Custom routers**: Create specialized routers for different parts of your application (API vs admin routes).

5. **Hook into callbacks**: Use `use_error()`, `use_headers_received()`, etc. to customize server behavior without subclassing.

This approach allows you to build application-specific functionality while leveraging the framework's core HTTP handling, threading, and routing capabilities.

## Gotchas & recommendations

- Register routes, middlewares, and static directories during startup before calling `listen()` to avoid concurrent mutation issues.
- For large static files prefer streaming APIs rather than reading the whole file into memory.
- Handlers and middleware must respect the `exit_code` contract and either `send()`/`end()` or leave finalization to the server's request flow. Avoid double-sending responses.
- Ensure `worker_pool` size is tuned for expected concurrency and blocking behavior of handlers (IO-bound vs CPU-bound).
