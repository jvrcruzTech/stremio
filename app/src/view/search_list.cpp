#include "view/search_list.hpp"
#include "view/video_card.hpp"

SearchList::SearchList() {
    brls::Logger::debug("View SearchList: create");
    // Inflate the tab from the XML file
    this->inflateFromXMLRes("xml/view/recycler_list.xml");

    this->registerStringXMLAttribute("title", [this](std::string value) { this->title->setTitle(value); });

    this->registerStringXMLAttribute("itemType", [this](std::string value) { this->itemType = value; });

    this->registerFloatXMLAttribute("pageSize", [this](float value) { this->pageSize = value; });
}

SearchList::~SearchList() { brls::Logger::debug("View SearchList: delete"); }

void SearchList::doRequest(const std::string& searchTerm) {
    brls::Logger::debug("SearchList: doRequest: {}", searchTerm);

    

    

    // Add the recycler to the current view
    this->addView(recycler);
}

brls::View* SearchList::create() { return new SearchList(); }