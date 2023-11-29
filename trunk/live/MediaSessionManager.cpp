#include "MediaSessionManager.h"

#include <utility>

#include "MediaSession.h"

MediaSessionManager* MediaSessionManager::createNew() {
    return new MediaSessionManager();
}

MediaSessionManager::MediaSessionManager() = default;

MediaSessionManager::~MediaSessionManager() = default;

bool MediaSessionManager::addSession(MediaSession* session) {
    if (mSessionMap.find(session->name()) != mSessionMap.end()) { // 已存在
        return false;
    } else {
        mSessionMap.insert(std::make_pair(session->name(), session));
        return true;
    }
}

bool MediaSessionManager::removeSession(MediaSession* session) {
    auto it = mSessionMap.find(session->name());

    if (it == mSessionMap.end()) {
        return false;
    } else {
        mSessionMap.erase(it);
        return true;
    }
}

MediaSession* MediaSessionManager::getSession(const std::string& name) {
    auto it = mSessionMap.find(name);
    if (it == mSessionMap.end()) {
        return nullptr;
    } else {
        return it->second;
    }
}
