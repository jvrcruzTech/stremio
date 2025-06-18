/*
    Copyright 2023 dragonflylee
*/

#include "activity/player_view.hpp"
#include "api/jellyfin.hpp"
#include "tab/media_streams.hpp"
#include "view/dynamic_box.hpp"
#include "view/auto_tab_frame.hpp"
#include "view/svg_image.hpp"
#include "view/text_box.hpp"
#include "view/video_card.hpp"
#include "view/video_source.hpp"
#include "view/presenter.hpp"
#include "api/libstremio.hpp"
#include <fmt/ranges.h>

using namespace brls::literals;  // for _i18n

class StreamCardCell : public BaseCardCell {
public:
    StreamCardCell() { this->inflateFromXMLRes("xml/view/stream_card.xml"); }

    static RecyclingGridItem* create() { return new StreamCardCell(); }

    BRLS_BIND(brls::Label, labelName, "stream/card/name");
    BRLS_BIND(brls::Label, labelOverview, "stream/card/overview");
    BRLS_BIND(SVGImage, badgeTopRight, "video/card/badge/top");
    BRLS_BIND(brls::Rectangle, rectProgress, "video/card/progress");
};

class StreamDataSource : public RecyclingGridDataSource {
public:
    using MediaList = std::vector<Stream>;

    explicit StreamDataSource(const MediaList& r) : list(std::move(r)) {
        brls::Logger::debug("StreamDataSource: create {}", r.size());
    }

    size_t getItemCount() override { return this->list.size(); }

    RecyclingGridItem* cellForRow(RecyclingView* recycler, size_t index) override {
        StreamCardCell* cell = dynamic_cast<StreamCardCell*>(recycler->dequeueReusableCell("Cell"));
        auto& item = this->list.at(index);
        cell->setId(item.getContentId());

        cell->labelName->setText(item.getName());
        cell->labelOverview->setText(item.getDescription());

        // if (item.UserData.Played) {
        //     cell->badgeTopRight->setImageFromSVGRes("icon/ico-checkmark.svg");
        //     cell->badgeTopRight->setVisibility(brls::Visibility::VISIBLE);
        // } else if (item.UserData.PlaybackPositionTicks) {
        //     cell->rectProgress->setWidthPercentage(item.UserData.PlayedPercentage);
        //     cell->rectProgress->getParent()->setVisibility(brls::Visibility::VISIBLE);
        //     cell->badgeTopRight->setVisibility(brls::Visibility::GONE);
        // } else {
        //     cell->badgeTopRight->setVisibility(brls::Visibility::GONE);
        //     cell->rectProgress->getParent()->setVisibility(brls::Visibility::GONE);
        // }

        // TODO: Handle playback position and played status
        cell->badgeTopRight->setVisibility(brls::Visibility::GONE);
        cell->rectProgress->getParent()->setVisibility(brls::Visibility::GONE);

        return cell;
    }

    void onItemSelected(brls::Box* recycler, size_t index) override {
        auto& item = this->list.at(index);
        string contentId = item.getContentId();
        string mediaId;
        string type;

        if (contentId.find(':') == std::string::npos) {
            type = "movie";
            mediaId = contentId;
        } else {
            type = "series";
            mediaId = contentId.substr(0, contentId.find(':'));
        }

        Meta meta = StremioAPI::fetchMeta(type,  mediaId);

        if (type == "movie") {
            brls::Logger::debug("Opening movie player for {}", meta.getName());
            PlayerView* view = new PlayerView(item);
            view->setTitie(meta.getName());
            brls::sync([view]() { brls::Application::giveFocus(view); });
        } else if (type == "series") {
            json videos = meta.getVideos();
            int seasonIndex;
            int episodeIndex;
            string episodeName;
            for (const auto& video : videos) {
                if (video["id"] == contentId) {
                    seasonIndex = video["season"].get<int>();
                    episodeIndex = video["episode"].get<int>();
                    episodeName = video["name"].get<std::string>();
                    break;
                }
            }

            PlayerView* view = new PlayerView(item);
            view->setTitie(fmt::format("S{}E{} - {}", seasonIndex, episodeIndex, episodeName));
            view->setSeries(mediaId);
            brls::sync([view]() { brls::Application::giveFocus(view); });
        }
    }

    void clearData() override { this->list.clear(); }

    void appendData(const MediaList& data) { this->list.insert(this->list.end(), data.begin(), data.end()); }

private:
    MediaList list;
};

class StreamList : public AttachedView {
public:
    StreamList(const vector<Stream>& items) {
        this->inflateFromXMLRes("xml/tabs/seasons.xml");

        this->streams = items;

        this->recycler->registerCell("Cell", StreamCardCell::create);
    }

    void onCreate() override {
        if (this->streams.empty()) {
            this->recycler->setError("No streams available.");
            return;
        }
        this->recycler->setDataSource(new StreamDataSource(this->streams));

        // std::string query = HTTP::encode_form({
        //     {"userId", AppConfig::instance().getUserId()},
        //     {"seasonId", this->seasonId},
        //     {"fields", "ItemCounts,PrimaryImageAspectRatio,Chapters,Overview"},
        // });

        // ASYNC_RETAIN
        // jellyfin::getJSON<jellyfin::Result<jellyfin::Stream>>(
        //     [ASYNC_TOKEN](const jellyfin::Result<jellyfin::Stream>& r) {
        //         ASYNC_RELEASE
        //         this->recycler->setDataSource(new StreamDataSource(r.Items));
        //     },
        //     [ASYNC_TOKEN](const std::string& ex) {
        //         ASYNC_RELEASE
        //         this->recycler->setError(ex);
        //     },
        //     jellyfin::apiShowStreams, this->seriesId, query);
    }

private:
    BRLS_BIND(RecyclingGrid, recycler, "media/Streams");

    vector<Stream> streams;
};

MediaStreams::MediaStreams(const Meta item) : item(item) {
    brls::Logger::debug("Tab MediaSeries: create");
    // Inflate the tab from the XML file
    this->inflateFromXMLRes("xml/tabs/series.xml");

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

    this->doList();
    this->doStreams();
    this->doNextup();
    this->doSimilar();

    // loading Logo
    Image::load(this->imageLogo, item.getLogoUrl());
    this->imageLogo->setVisibility(brls::Visibility::VISIBLE);
}

MediaStreams::~MediaStreams() {
    brls::Logger::debug("Tab MediaStreams: delete");
    Image::cancel(this->imageLogo);
}

void MediaStreams::doRequest() {
    if (this->tabFrame->isOnTop) {
        auto view = dynamic_cast<AttachedView*>(this->tabFrame->getActiveTab());
        if (view) view->onCreate();
        this->doNextup();
    }
}

void MediaStreams::doStreams() {

    this->headerTitle->setTitle(item.getName());
    this->labelOverview->setText(item.getDescription());
    if (item.getGenres().empty()) {
        this->labelGenres->setVisibility(brls::Visibility::GONE);
    } else {
        this->labelGenres->setText(fmt::format("{}", fmt::join(item.getGenres(), ", ")));
        this->labelGenres->setVisibility(brls::Visibility::VISIBLE);
    }


    // ASYNC_RETAIN
    // jellyfin::getJSON<jellyfin::Detail>(
    //     [ASYNC_TOKEN](const jellyfin::Detail& r) {
    //         ASYNC_RELEASE
    //         this->headerTitle->setTitle(r.Name);
    //         this->labelYear->setText(std::to_string(r.ProductionYear));
    //         if (r.OfficialRating.empty()) {
    //             this->parentalRating->getParent()->setVisibility(brls::Visibility::GONE);
    //         } else {
    //             this->parentalRating->setText(r.OfficialRating);
    //             this->parentalRating->getParent()->setVisibility(brls::Visibility::VISIBLE);
    //         }
    //         if (r.CommunityRating == 0.f) {
    //             this->labelRating->getParent()->setVisibility(brls::Visibility::GONE);
    //         } else {
    //             this->labelRating->setText(fmt::format("{:.1f}", r.CommunityRating));
    //             this->labelRating->getParent()->setVisibility(brls::Visibility::VISIBLE);
    //         }
    //         this->labelOverview->setText(r.Overview);

    //         if (r.Genres.empty()) {
    //             this->labelGenres->setVisibility(brls::Visibility::GONE);
    //         } else {
    //             this->labelGenres->setText(fmt::format("{}", fmt::join(r.Genres, ", ")));
    //             this->labelGenres->setVisibility(brls::Visibility::VISIBLE);
    //         }
    //         if (r.People.size() > 0) {
    //             this->people->setDataSource(new PeopleDataSource(r.People));
    //         } else {
    //             this->people->setVisibility(brls::Visibility::GONE);
    //         }

    //         auto logo = r.ImageTags.find(jellyfin::imageTypePrimary);
    //         if (logo != r.ImageTags.end()) {
    //             Image::load(this->imageLogo, jellyfin::apiPrimaryImage, r.Id,
    //                 HTTP::encode_form({
    //                     {"tag", logo->second},
    //                     {"maxWidth", "240"},
    //                 }));
    //         }
    //     },
    //     [ASYNC_TOKEN](const std::string& ex) {
    //         ASYNC_RELEASE
    //         this->people->setVisibility(brls::Visibility::GONE);
    //     },
    //     jellyfin::apiUserItem, AppConfig::instance().getUserId(), this->seriesId);
}

void MediaStreams::doList() {

    auto* item = new AutoSidebarItem();
    item->setTabStyle(AutoTabBarStyle::ACCENT);
    item->setFontSize(22);
    item->setLabel("Streams");
    this->tabFrame->addTab(item, [this]() { return new StreamList(this->item.getStreams()); });

    // ASYNC_RETAIN
    // jellyfin::getJSON<jellyfin::Result<jellyfin::Season>>(
    //     [ASYNC_TOKEN](const jellyfin::Result<jellyfin::Season>& r) {
    //         ASYNC_RELEASE

    //         for (size_t i = 0; i < r.Items.size(); i++) {
    //             auto& it = r.Items.at(i);
    //             auto* item = new AutoSidebarItem();
    //             item->setTabStyle(AutoTabBarStyle::ACCENT);
    //             item->setFontSize(22);
    //             item->setLabel(it.Name);
    //             this->tabFrame->addTab(item, [it]() { return new MediaSeason(it); });
    //         }
    //     },
    //     [ASYNC_TOKEN](const std::string& ex) {
    //         ASYNC_RELEASE
    //         brls::Logger::warning("doSeason {}", ex);
    //     },
    //     jellyfin::apiShowSeanon, this->seriesId, query);
}

void MediaStreams::doNextup() {
    std::string query = HTTP::encode_form({
        {"userId", AppConfig::instance().getUserId()},
        {"fields", "MediaSourceCount"},
        {"seriesId", this->seriesId},
    });
    ASYNC_RETAIN
    jellyfin::getJSON<jellyfin::Result<jellyfin::Stream>>(
        [ASYNC_TOKEN](const jellyfin::Result<jellyfin::Stream>& r) {
            ASYNC_RELEASE
            if (r.Items.size() > 0) {
                auto items = std::move(r.Items);
                items[0].SeriesName.clear();
                this->nextUp->setDataSource(new VideoDataSource(items));
                this->nextUp->setVisibility(brls::Visibility::VISIBLE);
                this->labelNextup->setVisibility(brls::Visibility::VISIBLE);
            } else {
                this->nextUp->setVisibility(brls::Visibility::GONE);
                this->labelNextup->setVisibility(brls::Visibility::GONE);
            }
        },
        [ASYNC_TOKEN](const std::string& ex) {
            ASYNC_RELEASE
            this->nextUp->setVisibility(brls::Visibility::GONE);
            this->labelNextup->setSubtitle(ex);
            brls::Application::notify(ex);
        },
        jellyfin::apiShowNextUp, query);
}

void MediaStreams::doSimilar() {
    std::string query = HTTP::encode_form({
        {"userId", AppConfig::instance().getUserId()},
        {"limit", "12"},
        {"fields", "ItemCounts"},
    });

    ASYNC_RETAIN
    jellyfin::getJSON<jellyfin::Result<jellyfin::Stream>>(
        [ASYNC_TOKEN](const jellyfin::Result<jellyfin::Stream>& r) {
            ASYNC_RELEASE
            if (r.Items.size() > 0) {
                this->similar->setDataSource(new VideoDataSource(r.Items));
                this->similar->setVisibility(brls::Visibility::VISIBLE);
                this->labelSimilar->setVisibility(brls::Visibility::VISIBLE);
            } else {
                this->similar->setVisibility(brls::Visibility::GONE);
                this->labelSimilar->setVisibility(brls::Visibility::GONE);
                this->similar->clearData();
            }
        },
        [ASYNC_TOKEN](const std::string& ex) {
            ASYNC_RELEASE
            this->similar->setVisibility(brls::Visibility::GONE);
            this->labelSimilar->setSubtitle(ex);
            brls::Application::notify(ex);
        },
        jellyfin::apiSimilar, this->seriesId, query);
}