#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <map>
#include <mutex>
#include <fstream>
#include <sstream>

#include "libs/json/json-parser.hpp"
#include "libs/html-builder/html-builder.hpp"
#include "web-lib.hpp"

using namespace hh_json;
using hh_web::methods::DELETE;
using hh_web::methods::GET;
using hh_web::methods::POST;
using hh_web::methods::PUT;
#include "ItemStore.hpp"

int get_id_from_request(const std::shared_ptr<hh_web::web_request> &req)
{
    auto params = req->get_path_params();
    for (const auto &[key, value] : params)
    {
        if (key == "id")
        {
            try
            {
                return std::stoi(value);
            }
            catch (const std::exception &e)
            {
                throw hh_web::web_exception(
                    "Invalid ID parameter: " + value,
                    "BAD_REQUEST",
                    "get_id_from_request",
                    400,
                    "Bad Request");
            }
        }
    }
    throw hh_web::web_exception(
        "ID parameter missing",
        "BAD_REQUEST",
        "get_id_from_request",
        400,
        "Bad Request");
}

hh_web::exit_code delete_item_handler(std::shared_ptr<hh_web::web_request> req, std::shared_ptr<hh_web::web_response> res)
{
    try
    {
        int id = get_id_from_request(req);
        get_item_store().remove(id);

        // For HTTP 204 No Content:
        // 1. Set the status code
        // 2. DO NOT set Content-Type
        // 3. DO NOT set a body (even empty string)
        res->set_status(204, "No Content");

        // That's it! Don't add any content for 204 responses
        return hh_web::exit_code::EXIT;
    }
    catch (hh_web::web_exception &e)
    {
        res->set_status(e.get_status_code(), e.get_status_message());
        res->send_json("{\"error\": \"" + std::string("Item Not Found") + "\"}");
        return hh_web::exit_code::EXIT;
    }
    catch (const std::exception &e)
    {
        res->set_status(500, "Internal Server Error");
        res->send_json("{\"error\": \"Failed to delete item\"}");
        return hh_web::exit_code::EXIT;
    }
}

hh_web::exit_code CORS(std::shared_ptr<hh_web::web_request> req, std::shared_ptr<hh_web::web_response> res)
{
    // only allow => http://localhost:4000 to GET,DELETE,POST,PUT,
    // any other origin just GET
    auto origins = req->get_header("Origin");
    if (std::find(origins.begin(), origins.end(), "http://localhost:4000") != origins.end())
    {
        res->add_header("Access-Control-Allow-Origin", "http://localhost:4000");
        res->add_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res->add_header("Access-Control-Allow-Headers", "Content-Type");
        res->add_header("Access-Control-Allow-Credentials", "true");
    }
    else
    {
        res->add_header("Access-Control-Allow-Origin", "*");
        res->add_header("Access-Control-Allow-Methods", "GET, OPTIONS");
        res->add_header("Access-Control-Allow-Headers", "Content-Type");
    }

    return hh_web::exit_code::CONTINUE;
}

hh_web::exit_code get_all_items_handler(std::shared_ptr<hh_web::web_request> req, std::shared_ptr<hh_web::web_response> res)
{
    try
    {
        auto items = get_item_store().get_all();

        // Build JSON array response
        std::stringstream ss;
        ss << "[";
        for (size_t i = 0; i < items.size(); ++i)
        {
            ss << items[i].to_json();
            if (i < items.size() - 1)
            {
                ss << ",";
            }
        }
        ss << "]";

        res->set_status(200, "OK");
        res->set_content_type("application/json");
        res->set_body(ss.str());
        return hh_web::exit_code::EXIT;
    }
    catch (const std::exception &e)
    {
        res->set_status(500, "Internal Server Error");
        res->set_content_type("application/json");
        res->set_body("{\"error\": \"Failed to retrieve items\"}");
        return hh_web::exit_code::EXIT;
    }
}
hh_web::exit_code get_specific_item_handler(std::shared_ptr<hh_web::web_request> req, std::shared_ptr<hh_web::web_response> res)
{
    try
    {
        int id = get_id_from_request(req);
        auto item = get_item_store().get(id);

        res->set_status(200, "OK");
        res->set_content_type("application/json");
        res->set_body(item.to_json());
        return hh_web::exit_code::EXIT;
    }
    catch (hh_web::web_exception &e)
    {
        res->set_status(e.get_status_code(), e.get_status_message());
        res->set_content_type("application/json");
        res->set_body("{\"error\": \"" + std::string(e.what()) + "\"}");
        return hh_web::exit_code::EXIT;
    }
    catch (const std::exception &e)
    {
        res->set_status(500, "Internal Server Error");
        res->set_content_type("application/json");
        res->set_body("{\"error\": \"Failed to retrieve item\"}");
        return hh_web::exit_code::EXIT;
    }
}

hh_web::exit_code create_new_item_handler(std::shared_ptr<hh_web::web_request> req, std::shared_ptr<hh_web::web_response> res)
{
    try
    {
        std::string name, description;
        double price;

        auto json = parse(req->get_body());

        name = getter::get_string(json["name"]);
        description = getter::get_string(json["description"]);
        price = getter::get_number(json["price"]);

        int id = get_item_store().create(name, description, price);
        auto item = get_item_store().get(id);

        auto x = item.to_json();

        res->set_status(201, "Created");
        res->set_content_type("application/json");
        res->set_body(item.to_json());
        return hh_web::exit_code::EXIT;
    }
    catch (hh_web::web_exception &e)
    {
        res->set_status(e.get_status_code(), e.get_status_message());
        res->set_content_type("application/json");
        JsonObject json_error;
        json_error.insert("error", maker::make_string("Failed To Create Item"));
        res->set_body(json_error.stringify());
        return hh_web::exit_code::EXIT;
    }
    catch (const std::exception &e)
    {
        res->set_status(500, "Internal Server Error");
        res->set_content_type("application/json");
        JsonObject json_error;
        json_error.insert("error", maker::make_string("Failed To Create Item, Internal Server Error"));
        res->set_body(json_error.stringify());
        return hh_web::exit_code::EXIT;
    }
}

hh_web::exit_code update_item_handler(std::shared_ptr<hh_web::web_request> req, std::shared_ptr<hh_web::web_response> res)
{
    try
    {
        int id = get_id_from_request(req);
        std::string name, description;
        double price;

        auto json = parse(req->get_body());
        name = getter::get_string(json["name"]);
        description = getter::get_string(json["description"]);
        price = getter::get_number(json["price"]);

        get_item_store().update(id, name, description, price);
        auto item = get_item_store().get(id);

        res->set_status(200, "OK");
        res->set_content_type("application/json");
        res->set_body(item.to_json());
        return hh_web::exit_code::EXIT;
    }
    catch (hh_web::web_exception &e)
    {
        res->set_status(e.get_status_code(), e.get_status_message());
        res->set_content_type("application/json");
        JsonObject json_error;
        json_error.insert("error", maker::make_string("Failed To Update Item"));
        res->set_body(json_error.stringify());
        return hh_web::exit_code::EXIT;
    }
    catch (const std::exception &e)
    {
        res->set_status(500, "Internal Server Error");
        res->set_content_type("application/json");
        JsonObject json_error;
        json_error.insert("error", maker::make_string("Failed To Update Item"));
        res->set_body(json_error.stringify());
        return hh_web::exit_code::EXIT;
    }
}

hh_web::exit_code index_handler(std::shared_ptr<hh_web::web_request> req, std::shared_ptr<hh_web::web_response> res)
{

    std::string html_doc;

    const std::string file_name = "html/index.html";

    std::ifstream file(file_name);
    if (file)
    {
        html_doc.assign((std::istreambuf_iterator<char>(file)),
                        (std::istreambuf_iterator<char>()));
    }

    res->set_status(200, "OK");
    res->send_html(html_doc);
    return hh_web::exit_code::EXIT;
}

hh_web::exit_code un_matched_route_handler(std::shared_ptr<hh_web::web_request> req, std::shared_ptr<hh_web::web_response> res)
{
    res->set_status(404, "Not Found");

    // For API requests, return JSON
    if (req->get_path().substr(0, 5) == "/api/")
    {
        res->set_content_type("application/json");
        res->set_body("{\"error\": \"Resource not found\"}");
    }
    else
    {
        // For web requests, return HTML
        std::string four04;

        std::ifstream file("html/404.html");
        if (file)
        {
            four04.assign((std::istreambuf_iterator<char>(file)),
                          (std::istreambuf_iterator<char>()));
        }

        res->send_html(four04);
    }

    return hh_web::exit_code::EXIT;
}

hh_web::exit_code json_cheacker(std::shared_ptr<hh_web::web_request> req, std::shared_ptr<hh_web::web_response> res)
{
    try
    {
        if (hh_web::body_has_malicious_content(req->get_body()))
        {
            hh_web::logger::error("Malicious content detected");
            hh_web::logger::error("Body:\n" + req->get_body());
            hh_web::logger::error("\n\n");
            throw hh_web::web_exception("Malicious content detected", 500, "Internal Server Error");
        }
        return hh_web::exit_code::CONTINUE;
    }
    catch (hh_web::web_exception &e)
    {
        res->set_status(e.get_status_code(), e.get_status_message());
        res->set_content_type("application/json");
        JsonObject json_error;
        json_error.insert("error", maker::make_string("Malicious content detected"));
        res->set_body(json_error.stringify());

        return hh_web::exit_code::EXIT;
    }
    catch (const std::exception &e)
    {
        res->set_status(400, "Bad Request");
        res->set_content_type("application/json");
        res->set_body("{\"error\": \"Invalid JSON format\"}");
        return hh_web::exit_code::EXIT;
    }

    return hh_web::exit_code::EXIT;
}

// Main application entry point
int main()
{
    try
    {
        int port = 3000;
        std::string host = "0.0.0.0";
        hh_web::logger::absolute_path_to_logs = "/home/hamza/Documents/Learnings/Projects/hamza-web-framwork/logs/";
        hh_web::logger::enabled_logging = true;
        hh_http::config::MAX_BODY_SIZE = 1024 * 64;
        hh_http::config::MAX_HEADER_SIZE = 1024 * 4;
        hh_http::config::MAX_IDLE_TIME_SECONDS = std::chrono::seconds(20);

        hh_http::epoll_config::BACKLOG_SIZE = 1024 * 1024;       // Set maximum number of pending connections
        hh_http::epoll_config::MAX_FILE_DESCRIPTORS = 1024 * 64; // Set maximum number of open files (open connections at the same time)
        hh_http::epoll_config::TIMEOUT_MILLISECONDS = 1000;      // Set timeout for epoll wait

        // Create server instance
        auto server = std::make_shared<hh_web::web_server<>>(port, host);

        // Create API router
        auto api_router = std::make_shared<hh_web::web_router<>>();

        // CORS middleware
        api_router->use(CORS);

        // Define routes for items API
        using V = std::vector<hh_web::web_request_handler_t<>>;
        using hh_web::web_route;

        // GET /api/items - Get all items
        auto all_items_route = std::make_shared<web_route<>>(GET, "/api/items", V({get_all_items_handler}));

        api_router->add_route(all_items_route);

        // GET /api/items/:id - Get specific item
        auto specific_item_route = std::make_shared<web_route<>>(GET, "/api/items/:id", V({get_specific_item_handler}));

        api_router->add_route(specific_item_route);

        // POST /api/items - Create new item
        auto create_new_item_route = std::make_shared<web_route<>>(POST, "/api/items", V({json_cheacker, create_new_item_handler}));
        api_router->add_route(create_new_item_route);

        // PUT /api/items/:id - Update item
        auto update_item_route = std::make_shared<web_route<>>(PUT, "/api/items/:id", V({json_cheacker, update_item_handler}));
        api_router->add_route(update_item_route);

        // DELETE /api/items/:id - Delete item
        auto delete_item_route = std::make_shared<web_route<>>(DELETE, "/api/items/:id", V({delete_item_handler}));

        api_router->add_route(delete_item_route);

        auto index = std::make_shared<web_route<>>(GET, "/", V({index_handler}));

        api_router->add_route(index);

        // Register router with server
        server->use_router(api_router);

        // Register static files directory
        server->use_static("static");

        // Custom 404 handler
        server->use_default(un_matched_route_handler);

        server->use_headers_received([](HEADER_RECEIVED_PARAMS)
                                     { hh_web::logger::info("Headers received");
                                                    hh_web::logger::info(method + " " + uri + " " + version);
                                                        for (const auto &[key, value] : headers)
                                                        {
                                                            hh_web::logger::info("Header: " + key + " = " + value);
                                                        } });

        server->listen(
            []()
            {
                std::cout << "Server is now running!" << std::endl;
                std::cout << "Visit http://localhost:3000 in your browser for API documentation" << std::endl;
            },
            [](const std::exception &e)
            {
                std::cerr << "Server error: " << e.what() << std::endl;
            });

        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
