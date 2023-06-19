#pragma once

#include "HsmObjectStoreClient.h"
#include "HsmObjectStoreClientBackend.h"

#include "HsmObjectStoreClientPlugin.h"
#include "ObjectStoreClientPlugin.h"
#include "PluginLoader.h"

#include <filesystem>
#include <set>
#include <string>
#include <unordered_map>

namespace hestia {

class ObjectStorePluginHandler {
  public:
    using Ptr = std::unique_ptr<ObjectStorePluginHandler>;

    ObjectStorePluginHandler(
        const std::vector<std::filesystem::path>& search_paths);

    bool has_plugin(const HsmObjectStoreClientBackend& client_spec);

    ObjectStoreClientPlugin::Ptr get_object_store_plugin(
        const HsmObjectStoreClientBackend& client_spec);

    HsmObjectStoreClientPlugin::Ptr get_hsm_object_store_plugin(
        const HsmObjectStoreClientBackend& client_spec);

  private:
    PluginLoader m_plugin_loader;
    std::vector<std::filesystem::path> m_search_paths;
};

class HsmObjectStoreClientFactory {
  public:
    using Ptr = std::unique_ptr<HsmObjectStoreClientFactory>;
    HsmObjectStoreClientFactory(ObjectStorePluginHandler::Ptr plugin_handler);

    bool is_client_type_available(
        const HsmObjectStoreClientBackend& client_spec) const;

    ObjectStoreClient::Ptr get_client(
        const HsmObjectStoreClientBackend& client_spec) const;

    ObjectStoreClientPlugin::Ptr get_client_from_plugin(
        const HsmObjectStoreClientBackend& client_spec) const;

    HsmObjectStoreClientPlugin::Ptr get_hsm_client_from_plugin(
        const HsmObjectStoreClientBackend& client_spec) const;

  private:
    ObjectStorePluginHandler::Ptr m_plugin_handler;
};

}  // namespace hestia