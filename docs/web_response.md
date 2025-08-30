# web_response

Source: `includes/web_response.hpp`

`web_response` is a high-level wrapper around the low-level `hh_http::http_response`. It provides convenient methods for constructing and sending HTTP responses (JSON/HTML/text), header and cookie helpers, and safe single-send/end semantics to prevent double-sends and race conditions. The class is intended to be instantiated and managed by the framework's `web_server` only.

## Design goals

- Simplify common response tasks (setting status, content type, body, cookies).
- Prevent double-sends and ensure safe finalization when used across threads.
- Provide a clear API for handlers to send responses without dealing with low-level details.

## Key characteristics

- Wraps an `hh_http::http_response` (member `response`).
- Tracks send/end state via `std::atomic<bool> did_send` and `did_end` to prevent duplicate operations.
- Protects header/body modification with mutexes (`modify_headers_mutex`, `send_response_mutex`, `end_response_mutex`).
- Deleted copy constructor/assignment; move operations defaulted.
- Provides convenience send methods that set appropriate headers (`send_json`, `send_html`, `send_text`).
- Intended to be created by `web_server`; do not construct directly in application code. Derivation for custom behavior is allowed.

## Members

- `hh_http::http_response response` — underlying low-level response object.
- `std::atomic<bool> did_end` — indicates whether the connection has been ended.
- `std::atomic<bool> did_send` — indicates whether the response has been sent.
- `std::mutex modify_headers_mutex` — protects header/body modification operations.
- `std::mutex send_response_mutex` — protects send operation sequencing.
- `std::mutex end_response_mutex` — protects end operation sequencing.

## Constructors & lifecycle

- `web_response(hh_http::http_response &&response)` — moves the underlying response into the wrapper and sets default status to `200 OK`. Intended to be called by `web_server`.
- Copy operations are deleted to prevent accidental duplication; move operations are defaulted to allow internal transfers.

Note: Do not instantiate `web_response` directly in application code. The server performs necessary initialization and finalization steps.

## Public API (function-level detail)

All methods are `virtual` so you can override behavior in derived response types.

- `void set_status(int status_code, std::string status_message = "")`

  - Sets the HTTP status code and message for the response. If `status_message` is empty, the implementation selects a default message category based on the status code range (2xx: OK, 3xx: Redirection, 4xx: Client Error, 5xx: Internal Server Error). The method locks `modify_headers_mutex` while updating status.

- `void send_json(const std::string &json_data)`

  - Convenience method to set `Content-Type: application/json`, set the response body and `Content-Length`, then call `send()` to transmit the response.

- `void send_html(const std::string &html_data)`

  - Convenience method to set `Content-Type: text/html`, set the response body and `Content-Length`, then call `send()`.

- `void send_text(const std::string &text_data)`

  - Convenience method to set `Content-Type: text/plain`, set the response body and `Content-Length`, then call `send()`.

- `void add_header(const std::string &key, const std::string &value)`

  - Adds an HTTP header to the response. Multiple headers with the same name may be added. Locks `modify_headers_mutex` during modification.

- `void add_trailer(const std::string &key, const std::string &value)`

  - Adds a trailer to the response (used with chunked transfer encoding). Locks `modify_headers_mutex` while updating trailers.

- `void add_cookie(const std::string &name, const std::string &cookie, const std::string &attributes = "")`

  - Adds a `Set-Cookie` header for the provided name/value and optional attributes (e.g., `Path=/; Secure; HttpOnly`). The implementation formats the header correctly and uses `response.add_header("Set-Cookie", ...)`.

- `void set_content_type(const std::string &content_type)`

  - Convenience to set `Content-Type` header.

- `void set_body(const std::string &body)`

  - Sets the response body on the underlying response object. Locks `modify_headers_mutex` during the operation.

- `void send(const std::string &body = "") noexcept`

  - Finalizes and transmits the response. The method ensures it runs only once by using `did_send.exchange(true)` and returns early on subsequent calls. It checks `did_end` to avoid sending on an already-ended connection.

  - Behavior details (from implementation):
    - Optionally accepts a `body` parameter and sets it if provided.
    - Ensures mandatory headers are set when missing: `Connection`, `Content-Type`, `Content-Length`.
    - Adds `Connection: close` when appropriate to close the connection after sending.
    - Locks `modify_headers_mutex` while checking/setting headers and body.
    - Performs the send via the underlying `hh_http::http_response` send mechanism inside a `try/catch` block to capture and log exceptions.

- `void set_keep_alive(bool keep_alive)`

  - Sets appropriate headers for persistent connections when `keep_alive` is true (implementation guarded by `modify_headers_mutex`).

- `void set_header(const std::string &name, const std::string &value)`
  - Replaces existing values for a header with a single new value (clears previous entries). Uses `modify_headers_mutex`.

## Underlying implementation notes

- The class uses atomic flags combined with mutexes to be safe when handlers or middleware might call `send()` or `end()` from multiple threads. `did_send` prevents duplicate sends while `did_end` prevents operations on closed connections.
- When sending, default headers are added if missing (connection management, content type, content length). This helps handlers that only set the body and expect the framework to provide safe defaults.
- Cookie handling simply uses `Set-Cookie` headers; complex cookie serialization should be done by the application when needed.
- The actual network transmission is delegated to the underlying `hh_http::http_response` object. Any exceptions thrown during transmission are caught and logged to avoid crashing the server.

## Examples

Simple JSON response:

```cpp
void handler(hh_web::web_request *req, hh_web::web_response *res) {
    std::string body = R"({"status":"ok"})";
    res->send_json(body);
}
```

Sending custom status and body:

```cpp
void handler(hh_web::web_request *req, hh_web::web_response *res) {
    res->set_status(201, "Created");
    res->set_content_type("application/json");
    res->set_body("{\"id\":123}");
    res->send();
}
```

Setting cookies and headers:

```cpp
void handler(hh_web::web_request *req, hh_web::web_response *res) {
    res->add_cookie("session", "abcd1234", "Path=/; HttpOnly; Secure");
    res->add_header("X-Custom", "value");
    res->send_text("OK");
}
```

Custom response type example (allowed):

```cpp
struct my_response : public hh_web::web_response {
    using web_response::web_response; // inherit constructor
    void send_json_with_meta(const std::string &json) {
        add_header("X-Meta", "1");
        send_json(json);
    }
};

// Configure your server to templates to use your custom response type.
```

## Important notes

- Do not construct `web_response` directly in application code. The `web_server` must create and initialize response objects so that connection lifetime and underlying transport resources are managed correctly.
- It is acceptable to derive from `web_response` to implement custom helpers and behaviors and configure the server to instantiate your derived types.
- Always avoid calling `send()` or `end()` multiple times; the class guards against double-send but repeated calls may indicate a logic error in application code.
