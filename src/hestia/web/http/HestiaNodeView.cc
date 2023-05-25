#include "HestiaNodeView.h"

#include "HestiaNode.h"
#include "HestiaNodeAdapter.h"
#include "HestiaService.h"

#include "StringUtils.h"

#include "Logger.h"

namespace hestia {
HestiaNodeView::HestiaNodeView(HestiaService* hestia_service) :
    WebView(), m_hestia_service(hestia_service)
{
    (void)m_hestia_service;
}

HttpResponse::Ptr HestiaNodeView::on_get(const HttpRequest& request)
{
    auto response = HttpResponse::create();

    auto full_path     = request.get_path();
    auto relative_path = StringUtils::remove_prefix(full_path, m_path);

    LOG_INFO("Full path: " + full_path);
    LOG_INFO("View path: " + m_path);
    LOG_INFO("Relative path: " + relative_path);

    if (relative_path.empty() || relative_path == "/") {
        std::vector<HestiaNode> nodes;
        m_hestia_service->get(nodes);
        response->set_body(HestiaNodeAdapter::to_json(nodes));
    }
    return response;
}

HttpResponse::Ptr HestiaNodeView::on_put(const HttpRequest& request)
{
    LOG_INFO("Into nodeview put");
    auto response = HttpResponse::create();

    auto full_path     = request.get_path();
    auto relative_path = StringUtils::remove_prefix(full_path, m_path);

    if (relative_path.empty() || relative_path == "/") {
        if (!request.body().empty()) {
            HestiaNode node;
            LOG_INFO("Trying to serialize: " << request.body());
            HestiaNodeAdapter::from_json(request.body(), node);
            m_hestia_service->put(node);

            std::string body;
            HestiaNodeAdapter::to_json(node, body);
            response->set_body(body);
        }
    }
    return response;
}

HttpResponse::Ptr HestiaNodeView::on_delete(const HttpRequest& request)
{
    (void)request;
    return HttpResponse::create();
}

HttpResponse::Ptr HestiaNodeView::on_head(const HttpRequest& request)
{
    (void)request;
    return HttpResponse::create();
}

}  // namespace hestia