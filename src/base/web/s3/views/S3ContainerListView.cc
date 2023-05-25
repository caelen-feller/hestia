#include "S3ContainerListView.h"

#include "S3Service.h"
#include "S3ViewUtils.h"

#include "Logger.h"
#include <sstream>

namespace hestia {
S3ContainerListView::S3ContainerListView(S3Service* service) :
    m_service(service)
{
    LOG_INFO("Loaded S3ContainerListView");
}

HttpResponse::Ptr S3ContainerListView::on_get(const HttpRequest& request)
{
    LOG_INFO("S3ContainerListView:on_get");
    (void)request;

    std::stringstream sstr;
    sstr << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    sstr << "<ListAllMyBucketsResult>\n";
    sstr << "<Buckets>\n";

    std::vector<S3Container> containers;
    FAIL_CHECK(m_service->list(containers));

    for (const auto& container : containers) {
        sstr << "<Bucket>\n";
        sstr << "<Name>" << container.m_name << "</Name>\n";
        sstr << "<CreationDate>" << container.m_creation_time
             << "</CreationDate>\n";
        sstr << "</Bucket>\n";
    }

    sstr << "</Buckets>\n";
    sstr << "</ListAllMyBucketsResult>\n";

    auto response = HttpResponse::create();
    response->set_body(sstr.str());

    LOG_INFO("Returning body: " << response->body());

    return response;
}
}  // namespace hestia