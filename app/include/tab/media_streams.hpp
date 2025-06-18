/*
    Copyright 2023 dragonflylee
*/

#pragma once

#include <borealis.hpp>
#include <view/presenter.hpp>

class AutoTabFrame;
class HRecyclerFrame;
class TextBox;

class MediaStreams : public brls::Box, public Presenter {
public:
    MediaStreams(const Meta item);
    ~MediaStreams() override;

    void doRequest() override;

private:
    BRLS_BIND(brls::Image, imageLogo, "content/image/logo");
    BRLS_BIND(brls::Header, headerTitle, "content/header/title");
    BRLS_BIND(TextBox, labelOverview, "content/label/overview");
    BRLS_BIND(brls::Label, labelGenres, "content/label/genres");
    BRLS_BIND(HRecyclerFrame, streamsList, "content/streams");
    BRLS_BIND(AutoTabFrame, tabFrame, "content/tabFrame");

    void doStreams();
    void doList();
    void doSimilar();
    void doNextup();

    Meta item;
};
