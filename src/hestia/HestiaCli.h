#pragma once

#include "HestiaApplication.h"
#include "HestiaCommands.h"

namespace CLI {  // NOLINT
class App;
}

namespace hestia {

class IHestiaClient;

class HestiaCli {
  public:
    enum class AppCommand {
        UNKNOWN,
        CLIENT,
        DAEMON_START,
        DAEMON_STOP,
        SERVER
    };

    bool is_client() const;

    bool is_server() const;

    void parse_args(int argc, char* argv[]);

    OpStatus run(IHestiaApplication* app);

  protected:
    virtual void console_write(const std::string& output) const;

  private:
    void add_crud_commands(
        std::unordered_map<std::string, CLI::App*>& commands,
        CLI::App& app,
        const std::string& subject);
    void on_crud_option(const std::string& subject, const std::string& method);

    void add_hsm_actions(
        std::unordered_map<std::string, CLI::App*>& commands,
        CLI::App* command,
        const std::string& subject);
    void add_get_data_options(CLI::App* command);
    void add_put_data_options(CLI::App* command);
    void add_copy_data_options(CLI::App* command);
    void add_move_data_options(CLI::App* command);
    void add_release_data_options(CLI::App* command);

    OpStatus run_client(IHestiaApplication* app);
    OpStatus on_crud_method(IHestiaClient* client);

    OpStatus run_server(IHestiaApplication* app);
    OpStatus start_daemon(IHestiaApplication* app);
    OpStatus stop_daemon();

    std::string m_user_token;
    std::string m_config_path;
    std::string m_server_host;
    std::string m_server_port;
    AppCommand m_app_command{AppCommand::UNKNOWN};
    HestiaClientCommand m_client_command;
};
}  // namespace hestia