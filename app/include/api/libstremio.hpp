#pragma once

#include <borealis.hpp>
#include <stremio.hpp>
#include <addons.hpp>
#include "utils/config.hpp"
#include "http.hpp"

class StremioAPI {
    public:
    static void init();
    static void login(string email = "", string password = "");
    static void fetchAddons();
    static list<Catalog> fetchCatalogs(const std::string& type);
    static Meta fetchMeta(const std::string& type, const std::string& id) {
        list<Addon> addons = AddonCollection::getAddonsFromResource("meta");
        for (const auto& addon : addons) {
            Meta meta = addon.getMeta(type, id);
            if (meta.getId() == id && meta.getType() == type) {
                return meta;
            }
        }
        return;
    }



    private:
};