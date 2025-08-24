#pragma once

#include <string>
#include <vector>
#include <http-server/includes/http_response.hpp>
#include <http-server/includes/http_consts.hpp>
#include <logger.hpp>
#include <atomic>
#include <mutex>
namespace hamza_web
{
    template <typename T, typename G>
    class web_server;

    /**
     * @brief High-level web response wrapper with enhanced functionality.
     *
     * This class provides a high-level interface for HTTP responses by wrapping
     * the lower-level hamza_http::http_response class. It adds web framework-specific
     * functionality such as convenient content type handling, JSON/HTML/text response
     * methods, cookie management, and automatic response finalization.
     *
     * The class serves as an abstraction layer that simplifies response creation
     * in web applications while maintaining access to all underlying HTTP response
     * capabilities. It provides utility methods for common web development tasks
     * like setting content types, sending structured data, and managing headers.
     *
     * The response automatically sets a default 200 OK status and handles response
     * finalization to prevent double-sending, making it safer and more convenient
     * to use than the raw HTTP response interface.


     * @note It is intended to be passed as pointers between the methods, to ensure proper ownership and lifetime management.
     * @note Please NEVER Initialize web_response directly, Unless you are overriding the web_servers on_request_received method     */
    class web_response
    {
    protected:
        /// Underlying HTTP response object
        hamza_http::http_response response;

        /// Flag to prevent double-sending of response
        std::atomic<bool> did_end = false;

        /// Flag to prevent double-sending of response
        std::atomic<bool> did_send = false;

        /// Mutex for modifying headers
        mutable std::mutex modify_headers_mutex;

        /// Mutex For sending response
        mutable std::mutex send_response_mutex;

        /// Mutex For ending response
        mutable std::mutex end_response_mutex;
        /**
         * @brief Internal method to end connection with the client, must only be called within web_server or it's derived classes.
         *
         * Ensures that this function is only called once by checking the did_end flag.
         */
        void end() noexcept
        {
            /// Only one thread is guaranteed to end the response,
            /// exchange works as follows:
            /// it sets the value to true and returns the old value, so if the old value was true,
            /// it means another thread has already sent the response
            if (did_end.exchange(true))
                return;
            try
            {
                std::lock_guard<std::mutex> lock(end_response_mutex);
                response.end();
            }
            catch (const std::exception &e)
            {
                // Log the error using logger
                logger::error("Error ending response: " + std::string(e.what()));
            }
        }

    public:
        /// Allow web_server to access private members
        template <typename T, typename G>
        friend class web_server;

        /**
         * @brief Private constructor for internal use by web_server.
         * @param response HTTP response object to wrap (moved)
         *
         * Creates a web response wrapper around the provided HTTP response object.
         * The HTTP response is moved to avoid unnecessary copying and to maintain
         * ownership semantics. Sets default status to 200 OK for convenience.
         */
        web_response(hamza_http::http_response &&response) : response(std::move(response))
        {
            response.set_status(200, "OK");
        }
        // Copy operations - DELETED for resource safety and unique ownership
        web_response(const web_response &) = delete;
        web_response &operator=(const web_response &) = delete;

        // Move operations - ENABLED for ownership transfer
        web_response(web_response &&) = default;
        web_response &operator=(web_response &&) = default;
        /**
         * @brief Set the HTTP status code and message.
         * @param status_code Numeric HTTP status code (e.g., 200, 404, 500)
         * @param status_message Descriptive status message (e.g., "OK", "Not Found", "Internal Server Error")
         *
         * Sets the HTTP status line for the response. Common status codes include:
         * - 200 "OK" for successful requests
         * - 201 "Created" for successful resource creation
         * - 400 "Bad Request" for client errors
         * - 404 "Not Found" for missing resources
         * - 500 "Internal Server Error" for server errors
         */
        virtual void set_status(int status_code, const std::string &status_message)
        {
            std::lock_guard<std::mutex> lock(modify_headers_mutex);
            response.set_status(status_code, status_message);
        }

        /**
         * @brief Send a JSON response with appropriate content type.
         * @param json_data String containing valid JSON data
         *
         * Convenience method for sending JSON responses. Automatically sets the
         * Content-Type header to "application/json", sets the response body to
         * the provided JSON data, and sends the response.

         */
        virtual void send_json(const std::string &json_data)
        {
            {
                std::lock_guard<std::mutex> lock(modify_headers_mutex);
                response.add_header(hamza_http::HEADER_CONTENT_TYPE, "application/json");
                response.set_body(json_data);
                response.add_header(hamza_http::HEADER_CONTENT_LENGTH, std::to_string(json_data.size()));
            }
            send();
        }

        /**
         * @brief Send an HTML response with appropriate content type.
         * @param html_data String containing valid HTML content
         *
         * Convenience method for sending HTML responses. Automatically sets the
         * Content-Type header to "text/html", sets the response body to the
         * provided HTML content, and sends the response.

         */
        virtual void send_html(const std::string &html_data)
        {
            {
                std::lock_guard<std::mutex> lock(modify_headers_mutex);
                response.add_header(hamza_http::HEADER_CONTENT_TYPE, "text/html");
                response.set_body(html_data);
                response.add_header(hamza_http::HEADER_CONTENT_LENGTH, std::to_string(html_data.size()));
            }
            send();
        }

        /**
         * @brief Send a plain text response with appropriate content type.
         * @param text_data String containing plain text content
         *
         * Convenience method for sending plain text responses. Automatically sets the
         * Content-Type header to "text/plain", sets the response body to the
         * provided text content, and sends the response.

         */
        virtual void send_text(const std::string &text_data)
        {
            {
                std::lock_guard<std::mutex> lock(modify_headers_mutex);
                response.add_header(hamza_http::HEADER_CONTENT_TYPE, "text/plain");
                response.set_body(text_data);
                response.add_header(hamza_http::HEADER_CONTENT_LENGTH, std::to_string(text_data.size()));
            }
            send();
        }

        /**
         * @brief Add an HTTP header to the response.
         * @param key Header name (e.g., "Cache-Control", "X-Custom-Header")
         * @param value Header value
         *
         * Adds a custom HTTP header to the response. Headers are sent before
         * the response body and provide metadata about the response. Multiple
         * headers with the same name can be added.

         */
        virtual void add_header(const std::string &key, const std::string &value)
        {
            std::lock_guard<std::mutex> lock(modify_headers_mutex);
            response.add_header(key, value);
        }

        /**
         * @brief Add an HTTP trailer to the response.
         * @param key Trailer name
         * @param value Trailer value
         *
         * Adds a trailer header to the response. Trailers are headers that
         * are sent after the response body, typically used with chunked
         * transfer encoding to provide metadata calculated during body generation.
         */
        virtual void add_trailer(const std::string &key, const std::string &value)
        {
            std::lock_guard<std::mutex> lock(modify_headers_mutex);
            response.add_trailer(key, value);
        }

        /**
         * @brief Add a cookie to the response.
         * @param name Cookie name
         * @param cookie Cookie value
         * @param attributes Optional cookie attributes (e.g., "Path=/; Secure; HttpOnly")
         *
         * Convenience method for setting cookies in the response. Automatically
         * formats the Set-Cookie header with the provided name, value, and optional
         * attributes. Common attributes include:
         * - Path=/path - Cookie path scope
         * - Domain=.example.com - Cookie domain scope
         * - Secure - Only send over HTTPS
         * - HttpOnly - Not accessible via JavaScript
         * - SameSite=Strict|Lax|None - CSRF protection
         * - Max-Age=seconds - Cookie lifetime

         */
        virtual void add_cookie(const std::string &name, const std::string &cookie, const std::string &attributes = "")
        {
            std::lock_guard<std::mutex> lock(modify_headers_mutex);
            std::string value = cookie;
            if (!attributes.empty())
                value += "; " + attributes;

            response.add_header("Set-Cookie", name + "=" + value);
        }

        /**
         * @brief Set the Content-Type header.
         * @param content_type MIME type for the response content
         *
         * Convenience method for setting the Content-Type header, which indicates
         * the media type of the response body. Common content types include:
         * - "text/html" for HTML content
         * - "application/json" for JSON data
         * - "text/plain" for plain text
         * - "application/xml" for XML data
         * - "image/jpeg", "image/png" for images
         * - "application/pdf" for PDF files
         */
        virtual void set_content_type(const std::string &content_type)
        {
            std::lock_guard<std::mutex> lock(modify_headers_mutex);
            response.add_header(hamza_http::HEADER_CONTENT_TYPE, content_type);
        }

        /**
         * @brief Set the response body content.
         * @param body String containing the response body data
         *
         * Sets the content that will be sent as the HTTP response body.
         * This method only sets the body data; you must call send() or one
         * of the send_* convenience methods to actually transmit the response.
         */
        virtual void set_body(const std::string &body)
        {
            std::lock_guard<std::mutex> lock(modify_headers_mutex);
            response.set_body(body);
        }

        /**
         * @brief Send the response to the client.
         *
         * Finalizes and sends the HTTP response with all configured headers,
         * status, and body content. Automatically adds a "Connection: close"
         * header to properly terminate the HTTP connection.
         *
         * This method should be called after setting all desired headers,
         * status, and body content. Once called, the response cannot be
         * modified further.
         *
         * Ensures that this function is only called once by checking the did_send flag.
         *
         * @note This method is automatically called by send_json(), send_html(),
         * and send_text() convenience methods.
         * @note You can call this method only once, as subsequent calls will be ignored.
         */
        virtual void send() noexcept
        {
            /// Only one thread is guaranteed to send the response,
            /// exchange works as follows:
            /// it sets the value to true and returns the old value, so if the old value was true,
            /// it means another thread has already sent the response
            if (did_send.exchange(true))
                return;

            {
                /// Get the lock of the modify_headers_mutex, to ensure that another thread hasn't modified the headers
                std::lock_guard<std::mutex> lock(modify_headers_mutex);
                if (response.get_header(hamza_http::HEADER_CONNECTION).empty())
                {
                    response.add_header(hamza_http::HEADER_CONNECTION, "close");
                }
                if (response.get_header(hamza_http::HEADER_CONTENT_LENGTH).empty())
                {
                    response.add_header(hamza_http::HEADER_CONTENT_LENGTH, std::to_string(response.get_body().size()));
                }
            }

            try
            {
                std::lock_guard<std::mutex> lock(send_response_mutex);
                response.send();
            }
            catch (const std::exception &e)
            {
                logger::error("Error sending response: " + std::string(e.what()));
                end();
            }
        }
    };
}