/*
    Copyright 2023 dragonflylee
*/

#include "tab/media_collection.hpp"
#include "view/video_source.hpp"
#include "api/jellyfin.hpp"
#include "view/video_card.hpp"
#include "view/media_filter.hpp"
#include "view/auto_tab_frame.hpp"
#include "view/dynamic_box.hpp"
#include "tab/suggest_show.hpp"
#include "tab/suggest_movie.hpp"
#include "api/libstremio.hpp"
#include <fmt/ranges.h>

using namespace brls::literals;  // for _i18n

std::map<std::string, std::string> MediaCollection::customPrefs;

MediaCollection::MediaCollection(const std::string& itemId, const std::string& itemType, const std::string& genresId)
    : itemId(itemId), genresId(genresId), itemType(itemType), startIndex(0) {
    brls::Logger::debug("MediaCollection: create {} type {}", itemId, itemType);
    if (itemType == jellyfin::mediaTypeMovie || itemType == jellyfin::mediaTypeSeries) {
        this->inflateFromXMLRes("xml/tabs/collection.xml");
        // add genres tab
        auto* item = new AutoSidebarItem();
        item->setTabStyle(AutoTabBarStyle::ACCENT);
        item->setFontSize(18);
        item->setLabel("main/tabs/genres"_i18n);

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

        // add suggest tab
        item = new AutoSidebarItem();
        item->setTabStyle(AutoTabBarStyle::ACCENT);
        item->setFontSize(18);
        item->setLabel("main/tabs/suggest"_i18n);
        if (itemType == jellyfin::mediaTypeSeries) {
            this->tabFrame->addTab(item, [this]() { return new SuggestShow(this->itemId); });
        } else if (itemType == jellyfin::mediaTypeMovie) {
            this->tabFrame->addTab(item, [this]() { return new SuggestMovie(this->itemId); });
        }
    } else {
        this->inflateFromXMLRes("xml/tabs/media.xml");
    }

    this->pageSize = this->recycler->spanCount * 3;

    std::transform(this->prefKey.begin(), this->prefKey.end(), this->prefKey.begin(),
        [](unsigned char c) { return std::tolower(c); });

    this->recycler->registerAction("hints/refresh"_i18n, brls::BUTTON_BACK, [this](...) {
        this->startIndex = 0;
        this->recycler->showSkeleton();
        this->doRequest();
        return true;
    });

    this->recycler->registerCell("Cell", VideoCardCell::create);
    this->recycler->onNextPage([this]() { this->doRequest(); });

    if (AppConfig::SYNC) {
        if (MediaCollection::customPrefs.empty()) {
            this->doPreferences();
        } else {
            this->loadFilter();
            this->doRequest();
        }
    } else {
        this->registerAction("main/media/sort"_i18n, brls::BUTTON_Y, [this](...) {
            MediaFilter* filter = new MediaFilter();
            filter->getEvent()->subscribe([this]() {
                this->startIndex = 0;
                this->recycler->showSkeleton();
                this->doRequest();
            });
            brls::Application::pushActivity(new brls::Activity(filter));
            return true;
        });

        this->doRequest();
    }
}

brls::View* MediaCollection::getDefaultFocus() { return this->recycler; }


struct DisplaySort {
    std::string SortBy;
    std::string SortOrder;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(DisplaySort, SortBy, SortOrder);

void MediaCollection::loadFilter() {
    this->recycler->registerAction("main/media/sort"_i18n, brls::BUTTON_Y, [this](...) {
        MediaFilter* filter = new MediaFilter();
        filter->getEvent()->subscribe([this]() {
            this->startIndex = 0;
            this->recycler->showSkeleton();
            this->doRequest();
            this->saveFilter();
        });
        brls::Application::pushActivity(new brls::Activity(filter));
        return true;
    });

    auto it = MediaCollection::customPrefs.find(this->prefKey);
    if (it == MediaCollection::customPrefs.end()) return;

    try {
        DisplaySort s = nlohmann::json::parse(it->second);
        MediaFilter::selectedOrder = s.SortOrder == "Ascending" ? 0 : 1;
        for (size_t i = 0; i < std::size(MediaFilter::sortList); i++) {
            if (MediaFilter::sortList[i] == s.SortBy) {
                MediaFilter::selectedSort = i;
            }
        }
    } catch (const std::exception& ex) {
        brls::Application::notify(ex.what());
    }
}

void MediaCollection::doRequest() {

    list<Catalog> catalogs = StremioAPI::fetchCatalogs(this->itemType);

    if (catalogs.empty()) {
        brls::Logger::error("MediaCollection: no catalogs found for item type {}", this->itemType);
        this->recycler->setError("No catalogs found");
        return;
    }

    auto dataSrc = dynamic_cast<VideoDataSource*>(this->recycler->getDataSource());
    dataSrc->appendData(r.Items);
    this->recycler->notifyDataChanged();


    // std::vector<std::string> filters;
    // if (MediaFilter::selectedPlayed) filters.push_back("IsPlayed");
    // if (MediaFilter::selectedUnplayed) filters.push_back("IsUnplayed");

    // HTTP::Form query = {
    //     {"parentId", this->itemId},
    //     {"sortBy", MediaFilter::sortList[MediaFilter::selectedSort]},
    //     {"sortOrder", MediaFilter::selectedOrder ? "Descending" : "Ascending"},
    //     {"fields", "PrimaryImageAspectRatio,Chapters,BasicSyncInfo"},
    //     {"enableImageTypes", "Primary"},
    //     {"filters", fmt::format("{}", fmt::join(filters, ","))},
    //     {"limit", std::to_string(this->pageSize)},
    //     {"startIndex", std::to_string(this->startIndex)},
    // };
    // if (this->genresId.size() > 0) {
    //     query["genreIds"] = this->genresId;
    //     query["recursive"] = "true";
    // } else if (this->itemType.size() > 0) {
    //     query["includeItemTypes"] = this->itemType;
    //     query["recursive"] = "true";
    // }

    // ASYNC_RETAIN
    // jellyfin::getJSON<jellyfin::Result<jellyfin::Episode>>(
    //     [ASYNC_TOKEN](const jellyfin::Result<jellyfin::Episode>& r) {
    //         ASYNC_RELEASE
    //         this->startIndex = r.StartIndex + this->pageSize;
    //         if (r.TotalRecordCount == 0) {
    //             this->recycler->setEmpty();
    //         } else if (r.StartIndex == 0) {
    //             this->recycler->setDataSource(new VideoDataSource(r.Items));
    //             brls::Application::giveFocus(this->recycler);
    //         } else if (r.Items.size() > 0) {
    //             auto dataSrc = dynamic_cast<VideoDataSource*>(this->recycler->getDataSource());
    //             dataSrc->appendData(r.Items);
    //             this->recycler->notifyDataChanged();
    //         }
    //     },
    //     [ASYNC_TOKEN](const std::string& ex) {
    //         ASYNC_RELEASE
    //         if (this->startIndex > 0) {
    //             brls::Application::notify(ex);
    //         } else {
    //             this->recycler->setError(ex);
    //         }
    //     },
    //     jellyfin::apiUserLibrary, AppConfig::instance().getUserId(), HTTP::encode_form(query));
}