#include "UserService.h"

#include "HttpClient.h"
#include "HttpCrudClient.h"
#include "KeyValueCrudClient.h"
#include "KeyValueStoreClient.h"

#include "HashUtils.h"
#include "IdGenerator.h"
#include "StringAdapter.h"
#include "TimeProvider.h"
#include "TypedCrudRequest.h"
#include "UserTokenGenerator.h"

#include <cassert>
#include <stdexcept>

namespace hestia {

UserService::UserService(
    const ServiceConfig& config,
    std::unique_ptr<CrudClient> client,
    CrudService::Ptr token_service,
    std::unique_ptr<UserTokenGenerator> token_generator) :
    CrudService(config, std::move(client)),
    m_token_service(std::move(token_service)),
    m_token_generator(std::move(token_generator))
{
    if (m_token_generator == nullptr) {
        m_token_generator = std::make_unique<UserTokenGenerator>();
    }
}

UserService::~UserService() {}

UserService::Ptr UserService::create(
    const ServiceConfig& config,
    CrudServiceBackend* backend,
    IdGenerator* id_generator,
    TimeProvider* time_provider,
    std::unique_ptr<UserTokenGenerator> token_generator)
{
    auto user_model_factory     = std::make_unique<TypedModelFactory<User>>();
    auto user_model_factory_raw = user_model_factory.get();

    auto user_adapters =
        std::make_unique<AdapterCollection>(std::move(user_model_factory));
    user_adapters->add_adapter(
        CrudAttributes::to_string(CrudAttributes::Format::JSON),
        std::make_unique<JsonAdapter>(user_model_factory_raw));
    user_adapters->add_adapter(
        CrudAttributes::to_string(CrudAttributes::Format::KV_PAIR),
        std::make_unique<KeyValueAdapter>(user_model_factory_raw));

    auto token_model_factory = std::make_unique<TypedModelFactory<UserToken>>();
    auto token_model_factory_raw = token_model_factory.get();

    auto token_adapters =
        std::make_unique<AdapterCollection>(std::move(token_model_factory));
    token_adapters->add_adapter(
        CrudAttributes::to_string(CrudAttributes::Format::JSON),
        std::make_unique<JsonAdapter>(token_model_factory_raw));
    token_adapters->add_adapter(
        CrudAttributes::to_string(CrudAttributes::Format::KV_PAIR),
        std::make_unique<KeyValueAdapter>(token_model_factory_raw));

    CrudClientConfig client_config;
    client_config.m_prefix = config.m_global_prefix;

    std::unique_ptr<CrudClient> crud_client;
    std::unique_ptr<CrudClient> token_crud_client;

    if (backend->get_type() == CrudServiceBackend::Type::KEY_VALUE_STORE) {
        auto kv_backend =
            dynamic_cast<KeyValueStoreCrudServiceBackend*>(backend);
        if (kv_backend == nullptr) {
            throw std::runtime_error("Failed to convert to kv service backend");
        }

        crud_client = std::make_unique<KeyValueCrudClient>(
            client_config, std::move(user_adapters), kv_backend->m_client,
            id_generator, time_provider);

        token_crud_client = std::make_unique<KeyValueCrudClient>(
            client_config, std::move(token_adapters), kv_backend->m_client,
            id_generator, time_provider);
    }
    else {
        auto http_backend = dynamic_cast<HttpRestCrudServiceBackend*>(backend);
        if (http_backend == nullptr) {
            throw std::runtime_error(
                "Failed to convert to http service backend");
        }
        crud_client = std::make_unique<HttpCrudClient>(
            client_config, std::move(user_adapters), http_backend->m_client);

        token_crud_client = std::make_unique<HttpCrudClient>(
            client_config, std::move(token_adapters), http_backend->m_client);
    }
    auto token_service =
        std::make_unique<CrudService>(config, std::move(token_crud_client));
    return std::make_unique<UserService>(
        config, std::move(crud_client), std::move(token_service),
        std::move(token_generator));
}

CrudResponse::Ptr UserService::authenticate_user(
    const std::string& username, const std::string& password) const
{
    CrudRequest get_request(CrudQuery{
        CrudIdentifier{username, CrudIdentifier::Type::NAME},
        CrudQuery::OutputFormat::ITEM});
    auto get_response = make_request(get_request);
    if (!get_response->ok()) {
        LOG_ERROR("Error getting user");
        return get_response;
    }

    if (!get_response->found()) {
        LOG_ERROR("Couldn't find requested user");
        return get_response;
    }

    const auto user = get_response->get_item_as<User>();
    if (user == nullptr) {
        auto response = std::make_unique<CrudResponse>(get_request);
        response->on_error(
            {CrudErrorCode::ERROR, "Bad cast of response item to user"});
        return response;
    }

    const auto hashed_password = get_hashed_password(username, password);
    if (hashed_password == user->password()) {
        LOG_INFO("Supplied password is ok");
        return get_response;
    }
    else {
        LOG_ERROR(
            "Supplied password doesn't match: " << hashed_password << " | "
                                                << user->password());
        auto response = std::make_unique<CrudResponse>(get_request);
        response->on_error(
            {CrudErrorCode::NOT_AUTHENTICATED, "Failed to authenticate user"});
        return response;
    }
}

CrudResponse::Ptr UserService::authenticate_with_token(
    const std::string& token) const
{
    CrudRequest req(
        CrudQuery{KeyValuePair{"value", token}, CrudQuery::OutputFormat::ITEM});

    auto token_get_response = m_token_service->make_request(req);
    if (!token_get_response->ok()) {
        LOG_ERROR("Failed to authenticate with token - internal error");
        auto error_response = std::make_unique<CrudResponse>(req);
        error_response->on_error(
            {CrudErrorCode::ERROR, "Failed to check for token."});
        return error_response;
    }

    if (!token_get_response->found()) {
        LOG_ERROR("No matching token found.");
        auto error_response = std::make_unique<CrudResponse>(req);
        error_response->on_error(
            {CrudErrorCode::ITEM_NOT_FOUND, "No matching token found."});
        return error_response;
    }

    const auto user_token = token_get_response->get_item_as<UserToken>();
    assert(user_token != nullptr);
    if (user_token == nullptr) {
        auto response = std::make_unique<CrudResponse>(req);
        response->on_error(
            {CrudErrorCode::ERROR, "Bad cast of response item to user token"});
        return response;
    }

    CrudRequest user_req(CrudQuery{
        CrudIdentifier{
            user_token->get_user_id(), CrudIdentifier::Type::PRIMARY_KEY},
        CrudQuery::OutputFormat::ITEM});

    auto get_response = make_request(user_req);
    if (!get_response->ok()) {
        LOG_ERROR("Failed to get user given id - internal error");
        auto error_response = std::make_unique<CrudResponse>(req);
        error_response->on_error(
            {CrudErrorCode::ITEM_NOT_FOUND,
             "Failed to get user from token id."});
        return error_response;
    }

    if (!get_response->found()) {
        LOG_ERROR("No matching user found.");
        auto error_response = std::make_unique<CrudResponse>(req);
        error_response->on_error(
            {CrudErrorCode::ITEM_NOT_FOUND, "No matching user found."});
        return error_response;
    }
    return get_response;
}

CrudResponse::Ptr UserService::register_user(
    const std::string& username, const std::string& password) const
{
    LOG_INFO("Register user: " << username);

    CrudRequest request(CrudQuery{
        CrudIdentifier{username, CrudIdentifier::Type::NAME},
        CrudQuery::OutputFormat::ITEM});
    auto find_response = make_request(request);
    if (!find_response->ok()) {
        LOG_ERROR("Unexpected error creating new user");
        return find_response;
    }

    if (find_response->found()) {
        auto response = std::make_unique<CrudResponse>(request);
        LOG_INFO("Attempted to regiser already existing user");
        response->on_error(
            {CrudErrorCode::CANT_OVERRIDE_EXISTING, "User already exists"});
        return response;
    }

    LOG_INFO("User not found - creating new one");
    User user;
    user.set_name(username);

    auto create_response = make_request(TypedCrudRequest<User>{
        CrudMethod::CREATE, user, CrudQuery::OutputFormat::ITEM});
    if (!create_response->ok()) {
        LOG_ERROR("Unexpected error creating new user");
        return create_response;
    }

    const auto created_user = create_response->get_item_as<User>();
    if (created_user == nullptr) {
        auto response = std::make_unique<CrudResponse>(request);
        response->on_error(
            {CrudErrorCode::ERROR, "Bad cast of response item to user"});
        return response;
    }

    LOG_INFO("Adding user token");
    UserToken token(created_user->id());
    token.set_value(m_token_generator->generate());

    TypedCrudRequest<UserToken> req(
        CrudMethod::CREATE, token, CrudQuery::OutputFormat::ITEM);
    auto token_response = m_token_service->make_request(req);
    if (!token_response->ok()) {
        auto error_response = std::make_unique<CrudResponse>(req);
        error_response->on_error(
            {CrudErrorCode::ITEM_NOT_FOUND,
             "Failed to create access token for user."});
        return error_response;
    }

    const auto user_token = token_response->get_item_as<UserToken>();
    if (user_token == nullptr) {
        auto response = std::make_unique<CrudResponse>(req);
        response->on_error(
            {CrudErrorCode::ERROR, "Bad cast of response item to user token"});
        return response;
    }

    User updated_user(*created_user);
    updated_user.set_password(get_hashed_password(username, password));
    updated_user.set_token(*user_token);

    LOG_INFO("Updating user password");
    auto update_response = make_request(TypedCrudRequest<User>{
        CrudMethod::UPDATE, updated_user, CrudQuery::OutputFormat::ITEM});
    return update_response;
}

std::string UserService::get_hashed_password(
    const std::string& username, const std::string& password) const
{
    std::string salt = "hestia";
    return HashUtils::base64_encode(
        HashUtils::do_sha256(username + salt + password));
}

}  // namespace hestia