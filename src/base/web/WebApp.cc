#include "WebApp.h"

#include "ApplicationMiddleware.h"
#include "RequestContext.h"
#include "UrlRouter.h"

#include "UserService.h"

#include "Logger.h"

namespace hestia {
WebApp::WebApp(UserService* user_service) : m_user_service(user_service) {}

WebApp::Ptr WebApp::create(UserService* user_service)
{
    return std::make_unique<WebApp>(user_service);
}

WebApp::~WebApp() {}

void WebApp::add_middleware(ApplicationMiddleware::Ptr middleware)
{
    m_middleware.push_back(std::move(middleware));
}


void WebApp::on_event(
    RequestContext* request_context, HttpEvent event) const noexcept
{
    if (!m_url_router) {
        request_context->set_response(
            on_view_not_found(request_context->get_request()));
    }

    LOG_INFO("Requested path: " + request_context->get_request().get_path());
    LOG_INFO(
        "Requested method: "
        + request_context->get_request().get_method_as_string());

    auto view =
        m_url_router->get_view(request_context->get_request().get_path());
    if (view == nullptr) {
        LOG_INFO("View not found");
        request_context->set_response(
            on_view_not_found(request_context->get_request()));
        return;
    }

    auto response = HttpResponse::create();
    if (m_middleware.empty()) {
        try {
            response = view->on_event(
                request_context->get_request(), event,
                request_context->get_auth_context());
        }
        catch (const std::exception& e) {
            LOG_ERROR("Unhandled exception in view: " << e.what());
        }
        catch (...) {
            LOG_ERROR("Uknown exception in view");
            response = HttpResponse::create(500, "Internal Server Error");
        }
    }
    else {
        try {
            response = on_middleware_layer(
                view, request_context->get_auth_context(), 0,
                request_context->get_request(), event);
        }
        catch (const std::exception& e) {
            LOG_ERROR(
                "Unhandled exception in view or middleware: " << e.what());
            response = HttpResponse::create(500, "Internal Server Error");
        }
        catch (...) {
            LOG_ERROR("Uknown exception in view or middleware");
            response = HttpResponse::create(500, "Internal Server Error");
        }
    }

    if (response->error()) {
        if (const auto stream_state = request_context->clear_stream();
            !stream_state.ok()) {
            LOG_ERROR("Bad stream state while handling response error.");
        }
    }

    std::size_t content_length = response->body().size();
    if (request_context->get_stream()->has_content()) {
        content_length += request_context->get_stream()->get_source_size();
    }
    if (content_length > 0) {
        response->header().set_item(
            "Content-Length", std::to_string(content_length));
    }

    request_context->set_response(std::move(response));
}

HttpResponse::Ptr WebApp::on_view_not_found(const HttpRequest& request) const
{
    (void)request;
    return HttpResponse::create(404, "Not Found");
}

HttpResponse::Ptr WebApp::on_middleware_layer(
    WebView* view,
    AuthorizationContext& auth,
    std::size_t working_idx,
    const HttpRequest& request,
    HttpEvent event) const
{
    if (working_idx == m_middleware.size()) {
        return view->on_event(request, event, auth);
    }
    else {
        auto response_provider = [this, view, &auth, working_idx,
                                  event](const HttpRequest& request) {
            return on_middleware_layer(
                view, auth, working_idx + 1, request, event);
        };
        return m_middleware[working_idx]->call(
            request, auth, event, response_provider);
    }
}

void WebApp::set_url_router(UrlRouter::Ptr router)
{
    m_url_router = std::move(router);
}
}  // namespace hestia