# web_request

Source: `includes/web_request.hpp`

`web_request` is a high-level wrapper around the lower-level `hh_http::http_request`. It adds framework-specific convenience methods (path/query parsing, header helpers, parameters storage) while preserving ownership and move semantics. The class is intended to be created and managed by the framework's `web_server` (see note below).

## Design goals

- Provide a convenient, typed interface for handlers to inspect requests.
- Preserve ownership semantics (move-only or move-enabled) to avoid accidental copies.
- Offer helpers for common tasks: extracting path params, query parameters, common headers, and managing per-request parameters.

## Key characteristics

- Wraps an `hh_http::http_request` (member `request`).
- Stores mutable `path_params` (vector of name/value pairs) guarded by a mutex.
- Stores an application-visible `request_params` map for custom values that handlers may set.
- Deleted copy constructor/assignment. Move construction/assignment are allowed.
- Intended to be created by `web_server` only; handlers should receive pointers/references to it.

## Members

- `hh_http::http_request request` — underlying low-level request object.
- `std::vector<std::pair<std::string, std::string>> path_params` — extracted path parameters.
- `std::mutex path_params_mutex` — protects `path_params` when modified.
- `std::map<std::string, std::string> request_params` — user-settable parameter bag.

## Constructors & lifecycle

- `web_request(hh_http::http_request &&req)` — move-constructor which takes ownership of the underlying HTTP request. This constructor is intended to be called by `web_server` when a new connection/request arrives.
- Copy operations are deleted to enforce single ownership; move operations are defaulted to allow safe transfer between internal components.

Note: Do not instantiate `web_request` directly in application code. The framework's `web_server` constructs this object and may perform additional initialization (e.g., parsing and setting `path_params`) before handing it to user handlers. If you need custom request behavior, derive from `web_request` and configure the server to instantiate your type.

## Public API (function-level detail)

All listed member functions are `virtual` to allow overriding in derived request types.

- `std::vector<std::pair<std::string, std::string>> get_path_params() const`

  - Returns the `path_params` vector (name/value pairs) extracted by the router. Implementation simply returns the stored `path_params`. Access is const but modifications must be done using `set_path_params` (protected by mutex).

- `void set_path_params(const std::vector<std::pair<std::string, std::string>> &params)`

  - Sets the `path_params` vector. Intended to be called by the routing layer (`web_route` / `web_server`) while holding the appropriate context. The method locks `path_params_mutex` to perform the assignment safely.

- `std::string get_method() const`

  - Proxy to `request.get_method()` — returns the HTTP method string (e.g., `"GET"`, `"POST"`).

- `std::string get_path() const`

  - Returns only the path component of the URI (no query string) by calling `hh_web::get_path(request.get_uri())`. The underlying implementation uses `web_utilities` helpers to split and normalize the URI.

- `std::string get_uri() const`

  - Returns the full request URI via `request.get_uri()` (path + query + fragment if present).

- `std::vector<std::pair<std::string, std::string>> get_query_parameters() const`

  - Parses the query string portion of the URI and returns all parameters as vector of name/value pairs. Delegates to `hh_web::get_query_parameters(request.get_uri())` which handles URL decoding and splitting.

- `std::string get_query_parameter(const std::string &key) const`

  - Convenience lookup: iterates the vector returned by `get_query_parameters()` and returns the first matching value or empty string if absent.

- `std::string get_version() const`

  - Proxy to `request.get_version()` — returns HTTP version (e.g., `"HTTP/1.1"`).

- `std::vector<std::string> get_header(const std::string &name) const`

  - Proxy to `request.get_header(name)` which returns all header values for a normalized header name. The underlying `http_request` normalizes header names (upper-case) for consistent lookup.

- `std::vector<std::pair<std::string, std::string>> get_headers() const`

  - Proxy to `request.get_headers()` that returns all headers as name/value pairs.

- `std::string get_body() const`

  - Proxy to `request.get_body()` that returns the request body as a string.

- `std::vector<std::string> get_content_type() const`

  - Convenience: `request.get_header(hh_http::HEADER_CONTENT_TYPE)`.

- `std::vector<std::string> get_cookies() const`

  - Convenience: `request.get_header(hh_http::HEADER_COOKIE)`.

- `std::vector<std::string> get_authorization() const`

  - Convenience: `request.get_header(hh_http::HEADER_AUTHORIZATION)`.

- `bool keep_alive() const`

  - Returns true when the `Connection` header indicates `Keep-Alive` (case-insensitive). Implementation collects `Connection` header values, upper-cases them and searches for `"KEEP-ALIVE"`.

- `void set_param(const std::string &key, const std::string &value)`

  - Store an application-visible parameter in `request_params` map. Useful for middleware and handlers to pass small bits of data.

- `std::string get_param(const std::string &key) const`

  - Returns the value for a key in `request_params` or empty string if not present.

- `std::map<std::string, std::string> get_params() const`

  - Returns a copy of `request_params` map.

- `void clear_params()`

  - Clears the `request_params` map.

- `void remove_param(const std::string &key)`
  - Removes a parameter from `request_params`.

## Underlying implementation notes

- `web_request` relies heavily on the underlying `hh_http::http_request` for parsing and storage of raw HTTP metadata. The wrapper provides convenience and normalization for application code.
- Path and query extraction functions call into `web_utilities.hpp` helpers (`get_path`, `get_query_parameters`) so that parsing logic is centralized and consistent across the framework.
- Header lookup uses the normalization performed by the low-level `http_request` (headers stored in upper-case); callers should either use the helper constants (e.g., `hh_http::HEADER_CONTENT_TYPE`) or pass header names in a consistent case.
- Thread-safety: only `path_params` modifications are explicitly protected by a mutex. `request_params` is a plain map — if you share `web_request` across threads you must synchronize access to it in your application.

## Examples

Handler read-only inspection example:

```cpp
void handler(hh_web::web_request *req, hh_web::web_response *res) {
    auto method = req->get_method();
    auto path = req->get_path();
    auto q = req->get_query_parameter("q");
    auto ct = req->get_content_type();
    // build response ...
}
```

Setting a request parameter in middleware and reading it in handler:

```cpp
void middleware_before(hh_web::web_request *req) {
    req->set_param("user_id", "123");
}

void handler(hh_web::web_request *req, hh_web::web_response *res) {
    auto user = req->get_param("user_id");
    // use user
}
```

Custom request type example (allowed):

```cpp
struct my_request : public hh_web::web_request {
    using web_request::web_request; // inherit constructor
    std::string tenant;
};

// Configure your server to templates to use your custom request type.
```

## Important notes

- Do not create `web_request` instances directly in application code. The `web_server` is responsible for constructing and initializing request objects (including setting `path_params`).
- It is acceptable to derive from `web_request` to provide custom behavior/data and configure the server to instantiate your derived type.
- If you must share the `web_request` across threads, ensure proper synchronization for `request_params` and any additional fields you add.
