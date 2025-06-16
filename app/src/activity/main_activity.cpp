#include "activity/main_activity.hpp"
#include "utils/config.hpp"
#include "api/http.hpp"

MainActivity::MainActivity() {
    brls::Logger::debug("MainActivity: create");

    auto& conf = AppConfig::instance();
    conf.checkDanmuku();

}

void MainActivity::onContentAvailable() {
    StremioAPI::init();
    if (!AppConfig::instance().checkLogin()) {
        // Hide the remote tab if there are no remotes
        brls::View* tab = this->getView("tab/server_login");
        if (tab) tab->setVisibility(brls::Visibility::GONE);
    }
}