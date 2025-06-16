/*
    Copyright 2023 dragonflylee
*/

#include "tab/server_login.hpp"
#include "activity/main_activity.hpp"
#include "api/jellyfin.hpp"
#include "utils/dialog.hpp"

using namespace brls::literals;  // for _i18n


ServerLogin::ServerLogin(const std::string& name, const std::string& url, const std::string& user) : url(url) {
    // Inflate the tab from the XML file
    this->inflateFromXMLRes("xml/tabs/server_login.xml");
    brls::Logger::debug("ServerLogin: create {}", url);

    this->hdrSigin->setTitle(brls::getStr("main/setting/server/sigin_to", name));
    this->inputUser->init("main/setting/username"_i18n, user);
    this->inputPass->init("main/setting/password"_i18n, "", [](std::string text) {}, "", "", 256);

    this->btnSignin->registerClickAction([this](...) { return this->onSignin(); });

    ASYNC_RETAIN
    brls::async([ASYNC_TOKEN]() {
        try {
            std::string resp = HTTP::get(this->url + jellyfin::apiQuickEnabled, HTTP::Timeout{});
            if (resp.compare("true") == 0)
                brls::sync([ASYNC_TOKEN]() {
                    ASYNC_RELEASE
                    this->btnQuickConnect->setVisibility(brls::Visibility::VISIBLE);
                });
        } catch (const std::exception& ex) {
            ASYNC_RELEASE
            brls::Logger::warning("query quickconnect: {}", ex.what());
        }
    });

    this->Disclaimer();
}

ServerLogin::~ServerLogin() { brls::Logger::debug("ServerLogin Activity: delete"); }

void ServerLogin::Disclaimer() {
    ASYNC_RETAIN
    this->labelDisclaimer->setVisibility(brls::Visibility::INVISIBLE);
    brls::async([ASYNC_TOKEN]() {
        try {
            auto resp = HTTP::get(this->url + jellyfin::apiBranding, HTTP::Timeout{});
            jellyfin::BrandingConfig r = nlohmann::json::parse(resp);
            if (!r.LoginDisclaimer.empty()) {
                brls::sync([ASYNC_TOKEN, r]() {
                    ASYNC_RELEASE
                    this->labelDisclaimer->setText(r.LoginDisclaimer);
                    this->labelDisclaimer->setVisibility(brls::Visibility::VISIBLE);
                });
            }
        } catch (const std::exception& ex) {
            ASYNC_RELEASE
            brls::Logger::warning("get login disclaimer: {}", ex.what());
        }
    });
}

bool ServerLogin::onSignin() {
    std::string email = inputUser->getValue();
    std::string password = inputPass->getValue();
    if (email.empty()) {
        Dialog::show("Username is empty");
        return false;
    }

    brls::Application::blockInputs();
    this->btnSignin->setState(brls::ButtonState::DISABLED);


    AppUser user{email, password};

    AppConfig::instance().setUser(user);

    StremioAPI::init();

    brls::Application::unblockInputs();
    brls::Application::clear();
    brls::Application::pushActivity(new MainActivity(), brls::TransitionAnimation::NONE);

    return true;
}
