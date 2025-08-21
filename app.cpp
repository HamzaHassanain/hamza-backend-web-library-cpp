#include <bits/stdc++.h>
#include <web_types.hpp>
#include <web_server.hpp>
#include <web_route.hpp>
#include <web_router.hpp>
#include <web_exceptions.hpp>
#include <html-builder/includes/element.hpp>
#include <html-builder/includes/document.hpp>
#include <html-builder/includes/document_parser.hpp>

using namespace hamza_web;
using namespace hamza_html_builder;
using H = web_request_handler_t<>;

std::map<std::string, std::string> cashed_files;
std::map<std::string, std::string> params = {
    {"heroTitle", "Welcome to My Portfolio"},
    {"heroDescription", "Discover my projects and skills."},
    {"aboutText", "I'm a passionate developer."},
    {"aboutExtraText", "I love creating web applications."},
    {"email", "hamza@example.com"},
    {"github", "https://github.com/hamza"},
    {"linkedin", "https://linkedin.com/in/hamza"},
    {"title", "Hamza's Portfolio"},
    {"subtitle", "Showcasing My Work"}};

struct Project
{
    std::string name;
    std::string description;
    std::vector<std::string> tech_stack;
};

struct Skill
{
    std::string name;
    std::string category;
};

std::vector<Project> projects = {
    {"Algorithm Visualizer", "Interactive platform for visualizing sorting and graph algorithms.", {"JavaScript", "Canvas API"}},
    {"Portfolio Website", "My personal portfolio showcasing my work.", {"HTML", "CSS", "JavaScript"}},
    {"Chat Application", "Real-time chat application with WebSocket support.", {"Node.js", "WebSocket"}},
};

std::string join(const std::vector<std::string> &vec, const std::string &delimiter)
{
    std::ostringstream oss;
    for (size_t i = 0; i < vec.size(); ++i)
    {
        oss << vec[i];
        if (i < vec.size() - 1)
            oss << delimiter;
    }
    return oss.str();
}
H index_handler = []([[maybe_unused]] std::shared_ptr<web_request> req, std::shared_ptr<hamza_web::web_response> res) -> hamza_web::exit_code
{
    try
    {
        std::vector<std::string> needed_files = {
            "html/body.html",
            "html/footer.html",
            "html/header.html",
            "html/head.html",
            "html/project.html",
        };
        for (const auto &file : needed_files)
        {
            std::string &content = cashed_files[file];
            if (content.empty())
            {
                std::ifstream ifs(file);
                if (!ifs)
                {
                    throw web_general_exception("Failed to open " + file);
                }
                std::stringstream buffer;
                buffer << ifs.rdbuf();
                content = buffer.str();
            }
        }
        auto &body_str = cashed_files["html/body.html"];
        auto &footer_str = cashed_files["html/footer.html"];
        auto &header_str = cashed_files["html/header.html"];
        auto &head_str = cashed_files["html/head.html"];

        document doc;
        auto head_elm = parse_html_string(head_str)[0];
        auto header_elm = parse_html_string(header_str)[0];
        auto body_elm = parse_html_string(body_str)[0];
        auto footer_elm = parse_html_string(footer_str)[0];

        element projects_elm;

        std::string project_template = cashed_files["html/project.html"];

        auto project_elm = parse_html_string(project_template)[0];

        for (const auto &project : projects)
        {
            element tech_stack_container;
            for (const auto &tech : project.tech_stack)
            {
                tech_stack_container.add_child(std::make_shared<element>(element("span", tech, {{"class", "tech-tag"}})));
            }
            auto tmep = project_elm->copy();
            tmep.set_text_params_recursive({{"project_name", project.name},
                                            {"project_description", project.description},
                                            {"project_tech_html_string", tech_stack_container.to_string()}});
            projects_elm.add_child(std::make_shared<element>(tmep));
        }

        head_elm->set_text_params_recursive(params);
        header_elm->set_text_params_recursive(params);
        body_elm->set_text_params_recursive(params);
        body_elm->set_text_params_recursive({{"projects_html_string", projects_elm.to_string()}});
        footer_elm->set_text_params_recursive(params);

        doc.add_child(head_elm);
        doc.add_child(header_elm);
        doc.add_child(body_elm);
        doc.add_child(footer_elm);

        res->set_status(200, "OK");
        res->send_html((doc.to_string()));
        return hamza_web::exit_code::EXIT;
    }
    catch (const std::exception &e)
    {
        std::cout << "Error: " << e.what() << std::endl;
        res->set_status(500, "Internal Server Error");
        res->send_text("Error: " + std::string(e.what()));
        return hamza_web::exit_code::ERROR;
    }
};
// implement stress handler
H stress_handler = []([[maybe_unused]] std::shared_ptr<web_request> req, std::shared_ptr<hamza_web::web_response> res) -> hamza_web::exit_code
{
    // Handle stress test requests
    // std::cout << "Received stress request on thread " << std::this_thread::get_id() << std::endl;
    // std::cout << "Handled stress request on thread " << std::this_thread::get_id() << std::endl;
    res->send_json("{\"status\": \"success\", \"message\": \"Stress test request handled successfully\"}");
    return hamza_web::exit_code::EXIT;
};
// implement stress handler
H stress_handler2 = []([[maybe_unused]] std::shared_ptr<web_request> req, std::shared_ptr<hamza_web::web_response> res) -> hamza_web::exit_code
{
    // Handle stress test requests
    // std::cout << "Received stress request on thread " << std::this_thread::get_id() << std::endl;
    // std::cout << "Handled stress request on thread " << std::this_thread::get_id() << std::endl;
    res->send_json("{\"status\": \"success\", \"message\": \"Stress 2222222222222222222222\"}");
    return hamza_web::exit_code::EXIT;
};

H stress_handler_id = []([[maybe_unused]] std::shared_ptr<web_request> req, std::shared_ptr<hamza_web::web_response> res) -> hamza_web::exit_code
{
    auto params = req->get_path_params();
    if (params.empty())
    {
        res->set_status(400, "Bad Request");
        res->send_text("Missing required path parameter: id");
        return hamza_web::exit_code::ERROR;
    }
    auto id = params[0].second; // Assuming the first parameter is the id
    res->send_json("{\"status\": \"success\", \"message\": \"Stress test id: " + id + "\"}");
    return hamza_web::exit_code::EXIT;
};

H stress_handler_id_name = []([[maybe_unused]] std::shared_ptr<web_request> req, std::shared_ptr<hamza_web::web_response> res) -> hamza_web::exit_code
{
    auto params = req->get_path_params();
    if (params.size() < 2)
    {
        res->set_status(400, "Bad Request");
        res->send_text("Missing required path parameters: id and name");
        return hamza_web::exit_code::ERROR;
    }
    auto id = params[0].second;   // Assuming the first parameter is the id
    auto name = params[1].second; // Assuming the second parameter is the name
    std::cerr << "id: " << id << ", name: " << name << std::endl;

    res->send_json("{\"status\": \"success\", \"message\": \"Stress test id: " + id + ", name: " + name + "\"}");
    return hamza_web::exit_code::EXIT;
};

std::function<bool()> get_random_01 = []() -> bool
{
    return rand() % 2 == 0;
};

H auth_middleware = []([[maybe_unused]] std::shared_ptr<web_request> req, std::shared_ptr<hamza_web::web_response> res) -> hamza_web::exit_code
{
    // Example authentication check
    // In a real application, you would check headers, tokens, etc.
    bool authenticated = get_random_01();

    if (!authenticated)
    {
        res->set_status(401, "Unauthorized");
        res->send_text("Unauthorized access");
        return hamza_web::exit_code::EXIT;
    }

    return hamza_web::exit_code::CONTINUE; // Continue to the next handler if authenticated
};

int main()
{
    try
    {
        auto server = std::make_unique<web_server<>>("127.0.0.1", 8080);

        auto index_route = std::make_shared<web_route<>>(methods::GET, "/", std::vector<H>{index_handler});
        auto stress_route_GET = std::make_shared<web_route<>>(methods::GET, "/stress", std::vector<H>{stress_handler});
        auto stress_route_2 = std::make_shared<web_route<>>(methods::GET, "/stress2", std::vector<H>{stress_handler2});
        auto stress_with_params = std::make_shared<web_route<>>(methods::GET, "/stress/:id", std::vector<H>{stress_handler_id});
        auto stress_with_params2 = std::make_shared<web_route<>>(methods::GET, "/stress/:id/:name", std::vector<H>{stress_handler_id_name});

        auto router = std::make_shared<web_router<>>();
        auto index_router = std::make_shared<web_router<>>();

        index_router->register_middleware(auth_middleware);
        index_router->register_route(index_route);

        router->register_route(stress_route_GET);
        router->register_route(stress_route_2);
        router->register_route(stress_with_params2);
        router->register_route(stress_with_params);

        server->register_static("static");
        server->register_router(router);
        server->register_router(index_router);

        server->listen();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}