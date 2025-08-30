# web_exception

Source: `includes/web_exceptions.hpp`

The `web_exception` class provides web-framework-specific error handling built on top of the project's socket exception type. It carries HTTP status code and status message information in addition to the base `socket_exception` metadata (message, type, function). The class is intended to represent errors that occur while processing HTTP requests, routing, or generating responses and to make it easy to construct meaningful HTTP error responses for clients.

## Design goals

- Provide a single exception type for web-layer errors that includes HTTP status semantics.
- Preserve rich diagnostic information from the underlying `socket_exception` (message, type, function) while adding HTTP status code/message for responses.
- Make common cases (generic 500 error) simple to construct while allowing full customization when needed.

## Key characteristics

- Inherits from `hh_socket::socket_exception`.
- Stores HTTP `status_code` (default 500) and `status_message` (default "Internal Server Error").
- Several convenience constructors for different levels of information (message-only, message+HTTP status, message+type+function, full customization).
- Accessors `get_status_code()` and `get_status_message()` expose HTTP metadata.
- Provides a `what()` method (overrides base) that returns a formatted string including HTTP status and the underlying `socket_exception` details.

## Members

- `int status_code` — HTTP status code to use when serializing the error for an HTTP response. Default: `500`.
- `std::string status_message` — Human-readable HTTP status phrase. Default: `"Internal Server Error"`.

## Constructors & destructor

### `explicit web_exception(const std::string &message)`

- Purpose: Construct a `web_exception` with an explanatory message only.
- Behavior: Calls base `socket_exception(message, "WEB_EXCEPTION", "web_function")` to provide a sensible default type and function. Uses default HTTP status 500 and message.

### `explicit web_exception(const std::string &message, int status_code, const std::string &status_message)`

- Purpose: Construct a `web_exception` with a custom HTTP status code and message.
- Behavior: Initializes the base `socket_exception` with the provided message but leaves type/function empty in the current implementation; sets `status_code` and `status_message` to the supplied values.

### `explicit web_exception(const std::string &message, const std::string &type, const std::string &function)`

- Purpose: Construct a `web_exception` while specifying base exception `type` and `function` information.
- Behavior: Calls the `socket_exception` constructor with message, type and function. HTTP status remains the default 500.

### `explicit web_exception(const std::string &message, const std::string &type, const std::string &function, int status_code = 500, std::string status_message = "Internal Server Error")`

- Purpose: Full customization for message, base exception metadata, and HTTP status information.
- Behavior: Calls base `socket_exception(message, type, function)` and sets `status_code` and `status_message` to the provided values (defaults shown above).

### Destructor

- Uses the compiler-generated default destructor.

## Public API (function-level detail)

#### `std::string get_status_message() const noexcept`

- Returns the HTTP status message associated with this exception (e.g., "Not Found", "Internal Server Error").
- Marked `noexcept` and safe to call from error-handling code.

#### `int get_status_code() const noexcept`

- Returns the numeric HTTP status code associated with this exception (e.g., `404`, `500`).
- Marked `noexcept`.

#### `std::string what() noexcept override`

- Returns a formatted human-readable string that includes the HTTP status code and message along with the underlying `socket_exception` details.
- Note: the signature returns `std::string` (the header implements `what()` returning a `std::string`) rather than the usual `const char*` used by `std::exception`. Callers should account for this when interacting with standard exception handling code.

## Examples

### Throw a generic server error from a handler

```cpp
throw hh_web::web_exception("template rendering failed");
```

This will produce a `web_exception` with HTTP 500 / "Internal Server Error" and base type `WEB_EXCEPTION` / function `web_function`.

### Throw an error with custom HTTP status

```cpp
throw hh_web::web_exception("resource not found", 404, "Not Found");
```

This sets the HTTP status information so a caller can use the values to populate an HTTP response.

### Provide diagnostic type/function information

```cpp
throw hh_web::web_exception("invalid route", "ROUTER_ERROR", "route_dispatch");
```

This preserves richer diagnostic metadata in the underlying `socket_exception` while keeping the default HTTP 500 status.

### Full customization

```cpp
throw hh_web::web_exception("validation failed", "VALIDATION", "validate_input", 422, "Unprocessable Entity");
```

Creates an exception with both detailed diagnostic fields and an appropriate HTTP status.

## Notes

- The class is designed for use inside the web framework: handlers and middleware can catch `hh_web::web_exception` and convert it to an HTTP response using `get_status_code()`/`get_status_message()` plus the message and base exception diagnostics.
- Be aware that the `what()` implementation in this header returns a `std::string` rather than the conventional `const char*`. If code expects `std::exception::what()` semantics, make sure to call the appropriate overloads or convert the returned string.
- Some constructors initialize the base `socket_exception` with empty type/function values; if diagnostic type/function information is important for your logs, prefer the constructors that accept `type` and `function` explicitly.
- Accessor methods are `noexcept` which makes them safe to call from error handlers and destructors.

## See also

- `includes/web_exceptions.hpp` — source for the class definition and inline documentation.
- `includes/web_request.hpp` and `includes/web_response.hpp` — how exceptions may be used when creating HTTP error responses.
