#pragma once

#include <view/recycling_grid.hpp>
#include <api/jellyfin/media.hpp>
#include <meta.hpp>

class VideoDataSource : public RecyclingGridDataSource {
public:
    using MediaList = std::vector<jellyfin::Episode>;

    explicit VideoDataSource(const MediaList& r, bool resume = false);

    size_t getItemCount() override;

    RecyclingGridItem* cellForRow(RecyclingView* recycler, size_t index) override;

    void onItemSelected(brls::Box* recycler, size_t index) override;

    void clearData() override;

    void appendData(list<Meta> data);

protected:
    vector<Meta> list;
    bool resume;
};