#pragma once

#include "HsmObjectStoreClient.h"
#include "HsmObjectStoreClientFactory.h"

#include <memory>

namespace hestia {

class HttpObjectStoreClient;
class HsmObjectStoreClientManager;
class DistributedHsmService;

class DistributedHsmObjectStoreClient : public HsmObjectStoreClient {
  public:
    using Ptr = std::unique_ptr<DistributedHsmObjectStoreClient>;

    DistributedHsmObjectStoreClient(
        std::unique_ptr<HsmObjectStoreClientManager> client_manager,
        std::unique_ptr<HttpObjectStoreClient> http_client = nullptr);

    static Ptr create(
        std::unique_ptr<HttpObjectStoreClient> http_client     = nullptr,
        const std::vector<std::filesystem::path>& plugin_paths = {});

    virtual ~DistributedHsmObjectStoreClient();

    void do_initialize(
        const std::string& cache_path, DistributedHsmService* hsm_service);

    [[nodiscard]] HsmObjectStoreResponse::Ptr make_request(
        const HsmObjectStoreRequest& request,
        Stream* stream = nullptr) const noexcept override;

  private:
    void copy(const HsmObjectStoreRequest& request) const override
    {
        (void)request;
    };

    void get(
        const HsmObjectStoreRequest& request,
        StorageObject& object,
        Stream* stream) const override
    {
        (void)request;
        (void)object, (void)stream;
    };

    void move(const HsmObjectStoreRequest& request) const override
    {
        (void)request;
    };

    void put(
        const HsmObjectStoreRequest& request, Stream* stream) const override
    {
        (void)request;
        (void)stream;
    };

    void remove(const HsmObjectStoreRequest& request) const override
    {
        (void)request;
    };

    DistributedHsmService* m_hsm_service{nullptr};
    std::unique_ptr<HttpObjectStoreClient> m_http_client;
    std::unique_ptr<HsmObjectStoreClientManager> m_client_manager;
};
}  // namespace hestia