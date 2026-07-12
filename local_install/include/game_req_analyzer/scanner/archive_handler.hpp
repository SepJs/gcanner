#pragma once

#include <game_req_analyzer/core/pch.hpp>
#include <game_req_analyzer/scanner/file_type_detector.hpp>
#include <game_req_analyzer/core/error.hpp>
#include <memory>
#include <vector>

namespace game_req {

struct FileInfo;

class ArchiveHandler {
public:
    virtual ~ArchiveHandler() = default;

    virtual bool can_handle(FileType type) const = 0;
    virtual Result<std::vector<FileInfo>> extract(const Path& archive_path, const Path& dest_dir) = 0;
    virtual Result<std::vector<FileInfo>> list_contents(const Path& archive_path) = 0;
    virtual String name() const = 0;
    virtual std::vector<FileType> supported_types() const = 0;
};

class ArchiveManager {
public:
    static ArchiveManager& instance() {
        static ArchiveManager instance;
        return instance;
    }

    void register_handler(std::unique_ptr<ArchiveHandler> handler) {
        handlers_.push_back(std::move(handler));
    }

    Result<std::vector<FileInfo>> extract(const Path& archive_path, const Path& dest_dir, FileType type) {
        for (const auto& handler : handlers_) {
            if (handler->can_handle(type)) {
                return handler->extract(archive_path, dest_dir);
            }
        }
        return make_unexpected(MAKE_ERROR(ErrorCode::UnsupportedFormat, "No handler for archive type"));
    }

    Result<std::vector<FileInfo>> list_contents(const Path& archive_path, FileType type) {
        for (const auto& handler : handlers_) {
            if (handler->can_handle(type)) {
                return handler->list_contents(archive_path);
            }
        }
        return make_unexpected(MAKE_ERROR(ErrorCode::UnsupportedFormat, "No handler for archive type"));
    }

    bool is_archive(FileType type) const {
        for (const auto& handler : handlers_) {
            for (const auto& supported : handler->supported_types()) {
                if (supported == type) return true;
            }
        }
        return false;
    }

    std::vector<String> available_handlers() const {
        std::vector<String> names;
        for (const auto& handler : handlers_) {
            names.push_back(handler->name());
        }
        return names;
    }

private:
    ArchiveManager();
    std::vector<std::unique_ptr<ArchiveHandler>> handlers_;
};

} // namespace game_req