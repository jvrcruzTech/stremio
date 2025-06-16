/*
    Copyright 2023 dragonflylee
*/

#pragma once

#include <borealis.hpp>
#include <stream.hpp>
#include <view/dynamic_box.hpp>


class SearchList : public brls::Box {
public:
    SearchList();
    ~SearchList() override;

    void doRequest(const std::string& searchTerm);

    static brls::View* create();

private:
    BRLS_BIND(brls::Header, title, "recycler/title");
    BRLS_BIND(DynamicBox, recycler, "recycler/list");   

    std::string itemType;
    size_t pageSize = 10;
};