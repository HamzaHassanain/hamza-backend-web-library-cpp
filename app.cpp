#include <bits/stdc++.h>
#include <web_types.hpp>
#include <web_server.hpp>
#include <web_route.hpp>
#include <web_router.hpp>
#include <web_exceptions.hpp>
#include <web_helpers.hpp>
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
H index_handler = []([[maybe_unused]] std::shared_ptr<web_request> req, std::shared_ptr<hamza_web::web_response> res) -> int
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
                    throw web_internal_server_error_exception("Failed to open " + file);
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
        // body_elm->set_text_params_recursive({{"skills_html_string", skills_elm.to_string()}});
        footer_elm->set_text_params_recursive(params);
        doc.add_child(head_elm);
        doc.add_child(header_elm);
        doc.add_child(body_elm);
        doc.add_child(footer_elm);

        res->set_status(200, "OK");
        res->html((doc.to_string()));
        res->end();

        return hamza_web::EXIT;
    }
    catch (const std::exception &e)
    {
        std::cout << "Error: " << e.what() << std::endl;
        res->set_status(500, "Internal Server Error");
        res->text("Error: " + std::string(e.what()));
        res->end();
        return hamza_web::EXIT;
    }
};

H middleware = [](std::shared_ptr<web_request> req, std::shared_ptr<web_response> res) -> int
{
    throw hamza_web::web_unauthorized_exception("Cannot Access Resource, Unauthorized");
};

int main()
{
    try
    {
        web_server<> server("127.0.0.1", 8080);

        auto index_route = std::make_shared<web_route<>>("/", methods::GET, std::vector<H>{index_handler});
        auto other_route = std::make_shared<web_route<>>("/other", methods::GET, std::vector<H>{index_handler});

        auto router = std::make_shared<web_router<>>();
        auto other_router = std::make_shared<web_router<>>();

        router->register_route(index_route);

        other_router->register_route(other_route);

        other_router->register_middleware(middleware);

        server.register_static("static");
        server.register_router(router);
        server.register_router(other_router);

        server.listen();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}