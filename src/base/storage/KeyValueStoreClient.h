#pragma once

#include "Metadata.h"
#include "ObjectStoreResponse.h"

#include <vector>

namespace hestia {
class KeyValueStoreClient {
  public:
    virtual ~KeyValueStoreClient();

    virtual void initialize(const Metadata& config) { (void)config; };

    [[nodiscard]] ObjectStoreResponse::Ptr make_request(
        const ObjectStoreRequest& request) const noexcept;

  private:
    virtual bool exists(const StorageObject& obj) const = 0;

    virtual void get(
        StorageObject& obj,
        const std::vector<std::string>& keys = {}) const = 0;

    virtual void put(
        const StorageObject& obj,
        const std::vector<std::string>& keys = {}) const = 0;

    virtual void remove(const StorageObject& obj) const = 0;

    virtual void list(
        const Metadata::Query& query,
        std::vector<StorageObject>& fetched) const = 0;

    void on_exception(
        const ObjectStoreRequest& request,
        ObjectStoreResponse* response,
        const std::string& message = {}) const;

    void on_exception(
        const ObjectStoreRequest& request,
        ObjectStoreResponse* response,
        const ObjectStoreError& error) const;
};
}  // namespace hestia