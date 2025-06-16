#include "tab/remote_tab.hpp"
#include "tab/remote_view.hpp"
#include "utils/config.hpp"

using namespace brls::literals;

RemoteTab::RemoteTab() {
    this->inflateFromXMLRes("xml/tabs/remote.xml");
    brls::Logger::debug("RemoteTab: create");

    this->registerAction(
        "main/player/next"_i18n, brls::BUTTON_LB,
        [this](brls::View* view) {
            tabFrame->focus2LastTab();
            return true;
        },
        true);

    this->registerAction(
        "main/player/prev"_i18n, brls::BUTTON_RB,
        [this](brls::View* view) {
            tabFrame->focus2NextTab();
            return true;
        },
        true);
}

RemoteTab::~RemoteTab() { brls::Logger::debug("RemoteTab: deleted"); }

brls::View* RemoteTab::create() { return new RemoteTab(); }

void RemoteTab::onCreate() {
    
}