#pragma once

#include "CrudRequest.h"
#include "HestiaTypes.h"
#include "HsmAction.h"

#include <filesystem>

namespace hestia {
class HestiaClientCommand {

  public:
    std::vector<std::string> get_crud_methods() const;

    std::vector<std::string> get_action_subjects() const;

    std::vector<HsmAction::Action> get_subject_actions(
        const std::string& subject) const;

    std::vector<std::string> get_hsm_subjects() const;

    std::vector<std::string> get_system_subjects() const;

    bool is_crud_method() const;

    bool is_create_method() const;

    bool is_read_method() const;

    bool is_update_method() const;

    bool is_remove_method() const;

    bool is_identify_method() const;

    bool is_data_management_action() const;

    bool is_data_io_action() const;

    bool is_data_put_action() const;

    void set_crud_method(const std::string& method);

    void set_hsm_action(HsmAction::Action action);

    void set_hsm_subject(const std::string& subject);

    void set_system_subject(const std::string& subject);

    HsmAction m_action;
    CrudMethod m_crud_method{CrudMethod::READ};

    uint8_t m_source_tier{0};
    uint8_t m_target_tier{0};

    HestiaType m_subject;

    std::string m_id;
    std::string m_id_format;
    std::string m_attribute_format;
    std::string m_output_format;
    std::string m_query_format;
    std::string m_body;

    std::string m_offset;
    std::string m_count;

    std::filesystem::path m_path;
};

}  // namespace hestia