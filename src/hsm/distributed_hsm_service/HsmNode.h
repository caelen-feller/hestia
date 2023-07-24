#pragma once

#include "HsmObjectStoreClientBackend.h"
#include "OwnableModel.h"

namespace hestia {

class HsmNode : public OwnableModel {
  public:
    HsmNode();

    HsmNode(const std::string& id);

    HsmNode(const HsmNode& other);

    using Ptr = std::unique_ptr<HsmNode>;

    const std::vector<HsmObjectStoreClientBackend>& backends() const;

    void add_backend(const HsmObjectStoreClientBackend& backend);

    static std::string get_type();

    bool is_controller() const;

    const std::string& host() const;

    unsigned port() const;

    void set_port(unsigned port);

    void set_host_address(const std::string& address);

    void set_app_type(const std::string& app_type);

    void set_is_controller(bool controller);

    void set_version(const std::string& version);

    void set_backends(const std::vector<HsmObjectStoreClientBackend>& backends);

    HsmNode& operator=(const HsmNode& other);

  private:
    void init();

    static constexpr const char s_model_type[]{"hsm_node"};
    BooleanField m_is_controller{"is_controller", false};
    StringField m_host_address{"host_address"};
    UIntegerField m_port{"port"};
    StringField m_version{"version"};
    StringField m_app_type{"app_type"};
    SequenceField<std::vector<HsmObjectStoreClientBackend>> m_backends{
        "backends"};
};
}  // namespace hestia