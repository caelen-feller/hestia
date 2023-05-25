#include <catch2/catch_all.hpp>

#include "FileKeyValueStoreClient.h"
#include "HsmObjectAdapter.h"
#include "ObjectService.h"
#include "TestUtils.h"

#include <filesystem>

class ObjectServiceTestFixture {
  public:
    void init(const std::string& test_name)
    {
        m_test_name           = test_name;
        const auto store_path = get_store_path();
        std::filesystem::remove_all(store_path);

        hestia::Metadata config;
        config.set_item("root", store_path);
        m_kv_store_client = std::make_unique<hestia::FileKeyValueStoreClient>();
        m_kv_store_client->initialize(config);
        m_object_service =
            std::make_unique<hestia::ObjectService>(m_kv_store_client.get());
    }

    ~ObjectServiceTestFixture()
    {
        std::filesystem::remove_all(get_store_path());
    }

    void put(const hestia::StorageObject& obj)
    {
        hestia::CrudRequest<hestia::HsmObject> request(
            obj, hestia::CrudMethod::PUT);
        auto response = m_object_service->make_request(request);
        REQUIRE(response->ok());
    }

    void get(hestia::StorageObject& obj)
    {
        hestia::CrudRequest<hestia::HsmObject> request(
            obj, hestia::CrudMethod::GET);
        auto response = m_object_service->make_request(request);
        REQUIRE(response->ok());
        obj = response->item().object();
    }

    bool exists(const hestia::StorageObject& obj)
    {
        hestia::CrudRequest<hestia::HsmObject> request(
            obj, hestia::CrudMethod::EXISTS);
        auto response = m_object_service->make_request(request);
        REQUIRE(response->ok());
        return response->found();
    }

    void list(std::vector<hestia::HsmObject>& objects)
    {
        hestia::CrudRequest<hestia::HsmObject> request(
            hestia::StorageObject(), hestia::CrudMethod::LIST);
        auto response = m_object_service->make_request(request);
        REQUIRE(response->ok());
        objects = response->items();
    }

    void remove(const hestia::StorageObject& obj)
    {
        hestia::CrudRequest<hestia::HsmObject> request(
            obj, hestia::CrudMethod::REMOVE);
        auto response = m_object_service->make_request(request);
        REQUIRE(response->ok());
    }

    std::string get_store_path() const
    {
        return TestUtils::get_test_output_dir(__FILE__) / m_test_name;
    }

    std::string m_test_name;
    std::unique_ptr<hestia::KeyValueStoreClient> m_kv_store_client;
    std::unique_ptr<hestia::ObjectService> m_object_service;
};

TEST_CASE_METHOD(
    ObjectServiceTestFixture,
    "Test Object Service - Get and Set",
    "[object_service]")
{
    init("TestObjectService");

    hestia::StorageObject object0("01234");
    object0.m_metadata.set_item("key0", "value0");
    object0.m_metadata.set_item("key1", "value1");
    object0.m_metadata.set_item("key2", "value2");

    hestia::StorageObject object1("56789");
    hestia::StorageObject object2("abcde");

    put(object0);
    put(object1);

    REQUIRE(exists(object0));
    REQUIRE(exists(object1));
    REQUIRE_FALSE(exists(object2));

    hestia::StorageObject retrieved_object0("01234");
    get(retrieved_object0);
    REQUIRE(retrieved_object0.m_metadata.get_item("key0") == "value0");
    REQUIRE(retrieved_object0.m_metadata.get_item("key1") == "value1");
    REQUIRE(retrieved_object0.m_metadata.get_item("key2") == "value2");

    std::vector<hestia::HsmObject> objects;
    list(objects);
    REQUIRE(objects.size() == 2);
    REQUIRE(objects[0].object().id() == object0.id());
    REQUIRE(objects[1].object().id() == object1.id());

    remove(object0);

    REQUIRE_FALSE(exists(object0));
    std::vector<hestia::HsmObject> updated_objects;
    list(updated_objects);
    REQUIRE(updated_objects.size() == 1);
}
