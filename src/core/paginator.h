#pragma once
#include <functional>
#include <optional>
#include <vector>
#include "core/types.h"

struct PaginationOptions {
    int count = 20;           // Items per page (Calendly default)
    bool fetch_all = false;   // --all flag
    int limit = 0;            // --limit flag (0 = no limit)
    std::optional<std::string> page_token;  // Starting page token
};

template<typename T>
class CursorPaginator {
public:
    using FetchFn = std::function<CursorPage<T>(const std::optional<std::string>& page_token)>;

    CursorPaginator(FetchFn fetch, PaginationOptions opts)
        : fetch_fn_(std::move(fetch)), opts_(opts) {}

    // Fetch a single page (uses configured page_token if none provided)
    CursorPage<T> fetch_page(const std::optional<std::string>& page_token = std::nullopt) {
        auto token = page_token.has_value() ? page_token : opts_.page_token;
        return fetch_fn_(token);
    }

    // Fetch ALL pages, concatenating collections
    std::vector<T> fetch_all() {
        std::vector<T> all_items;
        std::optional<std::string> token = opts_.page_token;

        while (true) {
            auto page = fetch_fn_(token);

            for (auto& item : page.collection) {
                all_items.push_back(std::move(item));

                // Check limit
                if (opts_.limit > 0 && static_cast<int>(all_items.size()) >= opts_.limit) {
                    all_items.resize(static_cast<size_t>(opts_.limit));
                    return all_items;
                }
            }

            // If not fetch_all mode and no limit set, stop after first page
            if (!opts_.fetch_all) {
                break;
            }

            // Check if there's a next page
            if (!page.pagination.next_page_token.has_value()) {
                break;
            }

            token = page.pagination.next_page_token;
        }

        return all_items;
    }

private:
    FetchFn fetch_fn_;
    PaginationOptions opts_;
};
