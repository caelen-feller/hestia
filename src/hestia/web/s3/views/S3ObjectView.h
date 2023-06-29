#pragma once

#include "S3ObjectAdapter.h"
#include "WebView.h"

namespace hestia {
class S3Service;

class S3ObjectView : public WebView {
  public:
    S3ObjectView(S3Service* service);

    HttpResponse::Ptr on_get(
        const HttpRequest& request, const User& user) override;

    HttpResponse::Ptr on_put(
        const HttpRequest& request, const User& user) override;

    HttpResponse::Ptr on_delete(
        const HttpRequest& request, const User& user) override;

    HttpResponse::Ptr on_head(
        const HttpRequest& request, const User& user) override;

  private:
    S3Service* m_service{nullptr};
    std::unique_ptr<S3ObjectAdapter> m_object_adatper;
};
}  // namespace hestia