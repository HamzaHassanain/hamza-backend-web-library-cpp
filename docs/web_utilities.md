# web_utilities

Source: `includes/web_utilities.hpp` and `src/web_utilities.cpp`

`web_utilities` contains helper functions for parsing URIs, query parameters, path matching, simple security checks, and MIME detection. These utilities centralize common parsing logic used by routers, request/response wrappers, and the server's static file serving.

## Key constants

- `static_extensions` — list of file extensions considered static resources (html, css, js, images, fonts, etc.). Used by `is_uri_static()`.
- `mime_types` — unordered_map mapping extension to MIME type for setting Content-Type when serving static files.

## Functions and their implementations

### `std::string url_encode(const std::string &value)`

- Encodes a string according to RFC 3986. Leaves unreserved characters (alnum and -\_.~) unchanged. Encodes other characters as `%HH` using uppercase hex.

### `std::string url_decode(const std::string &value)`

- Decodes percent-encoded sequences (`%HH`) back to characters. Scans for '%' and converts two hex digits to a character.

### `std::string get_mime_type_from_extension(const std::string &extension)`

- Looks up `extension` in `mime_types` and returns mapped MIME type or `application/octet-stream` if unknown.

### `std::string get_file_extension_from_mime(const std::string &mime_type)`

- Iterates `mime_types` and returns the first extension matching `mime_type`. O(n) scan; if reverse lookup is frequent maintain reverse map.

### `std::string get_file_extension_from_uri(const std::string &uri)`

- Returns substring after last '.' in the URI. Does not strip query strings or fragments; callers may need to sanitize.

### `std::string sanitize_path(const std::string &path)`

- Textual sanitation that removes ".." sequences to reduce directory traversal risk. Does not canonicalize symlinks; callers should canonicalize paths before opening files.

### `bool is_uri_static(const std::string &uri)`

- Determines if URI likely refers to a static resource by checking extension membership in `static_extensions`.

### `std::string trim(const std::string &str)`

- Removes leading and trailing whitespace using `find_first_not_of` and `find_last_not_of`.

### `std::vector<std::pair<std::string, std::string>> get_path_params(const std::string &uri)`

- Parses route expressions containing `:param` tokens and returns the parameter names. It does not resolve values; value extraction is handled by `match_path` during routing.

### `std::vector<std::pair<std::string, std::string>> get_query_parameters(const std::string &uri)`

- Finds the query portion after `?`, splits on `&` and `=`, trims whitespace, and returns name/value pairs. Note: current implementation does not URL-decode values; callers may call `url_decode` as needed.

### `std::pair<bool, std::vector<std::pair<std::string, std::string>>> match_path(const std::string &expression, const std::string &path)`

- Path matching algorithm supports:

  - Exact matches
  - Named parameters (`:param`) which match a single segment and are URL-decoded
  - Wildcard `*` which matches the remainder of the path

- Implementation notes:
  - Normalizes leading/trailing slashes (preserving root `/` behavior).
  - Splits into segments and iterates in lock-step; captures parameter values when expression segment begins with `:`.
  - Supports trailing wildcard `*` to match remainder (including empty remainder).

### `bool unknown_method(const std::string &method)`

- Checks whether `method` is one of the known methods enumerated in `web_methods.hpp` (GET, POST, PUT, DELETE, PATCH, HEAD, OPTIONS).

### `bool body_has_malicious_content(const std::string &body, bool XSS = true, bool SQL = true, bool CMD = true)`

- Performs heuristic checks for common XSS, SQL injection, and command injection patterns.
- Tokenizes the body and checks tokens against pattern lists for suspicious substrings. Also looks for long runs of special characters.
- The function is heuristic and should not be relied upon as a replacement for proper input validation or encoding.

## Security considerations

- `sanitize_path` and `is_uri_static` are simple helpers; for robust file serving the server must canonicalize paths (`realpath`/`std::filesystem::canonical`) and verify file is within permitted directories.
- `body_has_malicious_content` is heuristic. Use proper validation, prepared statements for DB calls, and output encoding for HTML to mitigate XSS.

## Examples

Parsing query parameters:

```cpp
auto params = hh_web::get_query_parameters("/search?q=foo&page=2");
// params => {{"q","foo"}, {"page","2"}}
```

Path matching with parameters:

```cpp
auto [matched, params] = hh_web::match_path("/users/:id/posts/:postId", "/users/123/posts/456");
// matched == true, params => {{"id","123"}, {"postId","456"}}
```

Checking static URI:

```cpp
bool is_static = hh_web::is_uri_static("/assets/app.js"); // true
```
