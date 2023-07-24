#pragma once

#include "IPhobosInterfaceImpl.h"

#if HAS_PHOBOS

namespace hestia {
class PhobosInterfaceImpl : public IPhobosInterfaceImpl {
  public:
    void get(const StorageObject& obj, int fd) override;

    void put(const StorageObject& obj, int fd) override;

    void get_metadata(StorageObject& obj) override;

    bool exists(const StorageObject& obj) override;

    void remove(const StorageObject& object) override;

    void list(
        const KeyValuePair& query, std::vector<StorageObject>& found) override;
};
}  // namespace hestia

#endif