#pragma once

#include <borealis/core/singleton.hpp>
#include <borealis/core/logger.hpp>
#include <nlohmann/json.hpp>
#include <atomic>

class AppVersion {
public:
    static std::string getVersion();
    static std::string getPlatform();
    static std::string getDeviceName();
    static std::string getPackageName();
    static std::string getCommit();
    static bool needUpdate(std::string latestVersion);
    static void checkUpdate(int delay = 2000, bool showUpToDateDialog = false);

    inline static std::shared_ptr<std::atomic_bool> updating = std::make_shared<std::atomic_bool>(true);
    inline static std::string git_repo = "jvrcruzTech/stremio";
};

struct AppUser {
    std::string email;
    std::string passwd;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AppUser, email, passwd);


class AppConfig : public brls::Singleton<AppConfig> {
    

public:
    enum Item {
        FULLSCREEN,
        OVERCLOCK,
        UMS,
        APP_THEME,
        APP_LANG,
        APP_UPDATE,
        KEYMAP,
        WINDOW_STATE,
        TRANSCODEC,
        FORCE_DIRECTPLAY,
        OSD_ON_TOGGLE,
        TOUCH_GESTURE,
        CLIP_POINT,
        SYNC_SETTING,
        MPV_VO,
        PLAYER_BOTTOM_BAR,
        PLAYER_LOW_QUALITY,
        PLAYER_INMEMORY_CACHE,
        PLAYER_HWDEC,
        PLAYER_HWDEC_CUSTOM,
        PLAYER_ASPECT,
        PLAYER_SUBS_FALLBACK,
        DANMAKU,
        DANMAKU_ON,
        DANMAKU_STYLE_AREA,
        DANMAKU_STYLE_ALPHA,
        DANMAKU_STYLE_FONTSIZE,
        DANMAKU_STYLE_LINE_HEIGHT,
        DANMAKU_STYLE_SPEED,
        DANMAKU_STYLE_FONT,
        DANMAKU_RENDER_QUALITY,
        ALWAYS_ON_TOP,
        SINGLE,
        SHOW_FPS,
        SWAP_INTERVAL,
        APP_SWAP_ABXY,  // A-B 交换 和 X-Y 交换
        TEXTURE_CACHE_NUM,
        REQUEST_THREADS,
        REQUEST_TIMEOUT,
        HTTP_PROXY_STATUS,
        HTTP_PROXY,
    };

    AppConfig() = default;

    bool init();
    void save();
    /// @brief 检查是否安装Danmuku插件
    bool checkDanmuku();

    std::string configDir();
    std::string ipcSocket();
    void checkRestart(char* argv[]);

    template <typename T>
    T getItem(const Item item, T defaultValue) {
        auto& o = settingMap[item];
        try {
            if (!setting.contains(o.key)) return defaultValue;
            return this->setting.at(o.key).get<T>();
        } catch (const std::exception& e) {
            brls::Logger::error("Damaged config found: {}/{}", o.key, e.what());
            return defaultValue;
        }
    }

    template <typename T>
    void setItem(const Item item, T data) {
        auto& o = settingMap[item];
        this->setting[o.key] = data;
        this->save();
    }

    struct Option {
        std::string key;
        std::vector<std::string> options;
        std::vector<long> values;
    };

    int getOptionIndex(const Item item, int default_index = 0) const;
    int getValueIndex(const Item item, int default_index = 0) const;
    inline const Option& getOptions(const Item item) const { return settingMap[item]; }
;
    const std::string& getDeviceId() { return this->device; }
    std::string getDevice(const std::string& token = "");
    AppUser getUser() const;
    void setUser(const AppUser& u);
    bool removeUser(const std::string& email);
    bool checkLogin();

    inline static bool SYNC = true;

private:
    static std::unordered_map<Item, Option> settingMap;

    AppUser user;
    std::string device;
    std::vector<AppUser> users;
    nlohmann::json setting = {};
};