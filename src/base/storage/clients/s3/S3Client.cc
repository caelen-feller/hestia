#include "S3Client.h"

#include "ProjectConfig.h"

#ifdef HAS_LIBS3
#include "LibS3InterfaceImpl.h"
#endif

#include "S3ContainerAdapter.h"
#include "S3Object.h"
#include "S3ObjectAdapter.h"

#include "Logger.h"

namespace hestia {
S3Client::S3Client(IS3InterfaceImpl::Ptr impl) : m_impl(std::move(impl))
{
#ifdef HAS_LIBS3
    if (!m_impl) {
        m_impl = std::make_unique<LibS3InterfaceImpl>();
    }
#endif
    m_container_adapter = S3ContainerAdapter::create("s3_client");
    m_object_adapter    = S3ObjectAdapter::create("s3_client");
}

S3Client::Ptr S3Client::create(IS3InterfaceImpl::Ptr impl)
{
    return std::make_unique<S3Client>(std::move(impl));
}

S3Client::Ptr S3Client::create(
    const S3Config& config, IS3InterfaceImpl::Ptr impl)
{
    auto instance = create(std::move(impl));
    instance->do_initialize({}, config);
    return instance;
}

std::string S3Client::get_registry_identifier()
{
    return hestia::project_config::get_project_name() + "::S3Client";
}

void S3Client::initialize(
    const std::string& cache_path, const Dictionary& config_data)
{
    S3Config config;
    config.deserialize(config_data);
    do_initialize(cache_path, config);
}

void S3Client::do_initialize(const std::string&, const S3Config& config)
{
    m_impl->initialize(config);

    m_container_adapter =
        S3ContainerAdapter::create(config.m_metadataprefix.get_value());
    m_object_adapter =
        S3ObjectAdapter::create(config.m_metadataprefix.get_value());
}

void S3Client::put(
    const StorageObject& object, const Extent& extent, Stream* stream) const
{
    S3Object s3_object;
    m_object_adapter->to_s3(s3_object, object);

    LOG_INFO("Doing PUT with StorageObject: " + object.to_string());
    LOG_INFO("Doing PUT with S3Object: " + s3_object.to_string());
    m_impl->put(s3_object, extent, stream);
}

void S3Client::get(
    StorageObject& object, const Extent& extent, Stream* stream) const
{
    S3Object s3_object;
    S3Container s3_container;
    m_impl->get(s3_object, extent, stream);
    m_object_adapter->from_s3(object, s3_container, s3_object);
}

void S3Client::remove(const StorageObject& object) const
{
    S3Object s3_object;
    m_object_adapter->to_s3(s3_object, object);
    m_impl->remove(s3_object);
}

void S3Client::list(
    const KeyValuePair& query, std::vector<StorageObject>& found) const
{
    (void)query;
    (void)found;
}

bool S3Client::exists(const StorageObject& object) const
{
    (void)object;
    return false;
}
}  // namespace hestia