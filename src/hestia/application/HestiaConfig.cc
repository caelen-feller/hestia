#include "HestiaConfig.h"

#include "Logger.h"
#include "YamlUtils.h"

#include <filesystem>
#include <iostream>

namespace hestia {

std::string HestiaConfig::load(const std::string& path)
{
    auto config_path = path;
    if (!std::filesystem::is_regular_file(config_path)) {
        config_path = std::filesystem::current_path() / "hestia.yaml";
    }

    if (!std::filesystem::is_regular_file(config_path)) {
        return {};
    }

    Dictionary dict;
    YamlUtils::load(config_path, dict);

    const auto root = dict.get_map_item("root");
    load(*root);

    if (m_backends.empty()) {
        load_object_store_defaults();
    }
    return config_path;
}

void HestiaConfig::load_server_config(const Dictionary& config)
{
    m_server_config.load(config);
}

void HestiaConfig::load_defaults()
{
    load_object_store_defaults();
    load_kv_store_defaults();
    load_event_feed_defaults();
}

void HestiaConfig::load_object_store_defaults()
{
    std::string default_root = m_cache_path + "/hsm_object_store";

    HsmObjectStoreClientBackend client_config(
        HsmObjectStoreClientBackend::Type::HSM,
        HsmObjectStoreClientBackend::Source::BUILT_IN,
        "hestia::FileHsmObjectStoreClient");
    m_backends.emplace(client_config.m_identifier, client_config);

    for (uint8_t idx = 0; idx < 5; idx++) {
        StorageTier tier(idx);
        tier.m_backend = client_config.m_identifier;
        m_tiers.emplace(idx, tier);
    }
}

void HestiaConfig::load_kv_store_defaults()
{
    std::string default_root = m_cache_path + "/hsm_key_value_store";

    Metadata config;
    config.set_item("root", default_root);
    m_key_value_store_spec =
        KeyValueStoreClientSpec(KeyValueStoreClientSpec::Type::FILE, config);
}

void HestiaConfig::load_kv_store(
    const Dictionary& kv_store_config, const Dictionary& kv_store_clients)
{
    std::string client_identifier;
    if (auto client_id = kv_store_config.get_map_item("client_identifier");
        client_id != nullptr) {
        client_identifier = client_id->get_scalar();
    }

    for (const auto& config : kv_store_clients.get_sequence()) {
        std::string identifier;
        if (const auto identifier_dict = config->get_map_item("identifier");
            identifier_dict != nullptr) {
            identifier = identifier_dict->get_scalar();
        }

        if (identifier == client_identifier) {
            Metadata client_config;
            config->get_map_items(client_config);
            if (client_identifier == "hestia::FileKeyValueStoreClient") {
                m_key_value_store_spec = KeyValueStoreClientSpec(
                    KeyValueStoreClientSpec::Type::FILE, client_config);
            }
            break;
        }
    }
}

void HestiaConfig::load_event_feed(const Dictionary& event_feed_config)
{
    if (const auto event_feed_path_dict =
            event_feed_config.get_map_item("event_feed_path");
        event_feed_path_dict != nullptr) {
        m_event_feed_config.m_event_feed_file_path =
            event_feed_path_dict->get_scalar();
    }
    if (const auto event_feed_active_dict =
            event_feed_config.get_map_item("event_feed_active");
        event_feed_active_dict != nullptr) {
        auto event_feed_active = event_feed_active_dict->get_scalar();
        if (event_feed_active == "n") {
            m_event_feed_config.m_active = false;
        }
        else {
            m_event_feed_config.m_active = true;
        }
    }
    if (const auto event_feed_sorted_dict =
            event_feed_config.get_map_item("event_feed_sorted");
        event_feed_sorted_dict != nullptr) {
        auto event_feed_sorted = event_feed_sorted_dict->get_scalar();
        if (event_feed_sorted == "y") {
            m_event_feed_config.m_sorted_keys = true;
        }
        else {
            m_event_feed_config.m_sorted_keys = false;
        }
    }
}

void HestiaConfig::load_event_feed_defaults()
{
    m_event_feed_config.m_event_feed_file_path =
        std::filesystem::current_path() / "event-feed.yaml";
    m_event_feed_config.m_active      = true;
    m_event_feed_config.m_sorted_keys = false;
}

void HestiaConfig::load(const Dictionary& dict)
{
    if (auto cache_loc = dict.get_map_item("cache_location"); cache_loc) {
        m_cache_path = cache_loc->get_scalar();
    }

    auto backends = dict.get_map_item("object_store_clients");
    auto tiers    = dict.get_map_item("tier_registry");

    if ((backends != nullptr) && (tiers != nullptr)) {
        load_object_store_clients(*backends, *tiers);
    }

    auto kv_store_config   = dict.get_map_item("key_value_store");
    auto kv_store_registry = dict.get_map_item("key_value_store_clients");

    if ((kv_store_config != nullptr) && (kv_store_registry != nullptr)) {
        load_kv_store(*kv_store_config, *kv_store_registry);
    }
    else {
        load_kv_store_defaults();
    }

    auto event_feed_config = dict.get_map_item("event_feed");
    if (event_feed_config != nullptr) {
        load_event_feed(*event_feed_config);
    }
    else {
        load_event_feed_defaults();
    }

    load_server_config(dict);
}

void HestiaConfig::load_object_store_clients(
    const Dictionary& backends, const Dictionary& tiers)
{
    for (const auto& store : backends.get_sequence()) {
        std::string identifier;
        if (const auto identifier_dict = store->get_map_item("identifier");
            identifier_dict != nullptr) {
            identifier = identifier_dict->get_scalar();
        }
        m_backends.emplace(identifier, HsmObjectStoreClientBackend(*store));
    }

    for (const auto& tier : tiers.get_sequence()) {
        uint8_t identifier{0};
        if (const auto identifier_dict = tier->get_map_item("identifier");
            identifier_dict != nullptr) {
            identifier =
                static_cast<uint8_t>(std::stoi(identifier_dict->get_scalar()));
        }

        std::string client_identifier;
        if (const auto identifier_dict =
                tier->get_map_item("client_identifier");
            identifier_dict != nullptr) {
            client_identifier = identifier_dict->get_scalar();
        }

        if (!client_identifier.empty()) {
            StorageTier tier_obj(identifier);
            tier_obj.m_backend = client_identifier;
            m_tiers.emplace(identifier, tier_obj);
        }
    }
}
}  // namespace hestia