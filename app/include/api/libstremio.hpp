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



    private:
};