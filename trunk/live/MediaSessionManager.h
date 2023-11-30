#ifndef RTSPSERVER_MEDIASESSIONMANAGER
#define RTSPSERVER_MEDIASESSIONMANAGER

#include <map>
#include <string>

class MediaSession;
class MediaSessionManager {
public:
    static MediaSessionManager* createNew();

    MediaSessionManager();
    ~MediaSessionManager();

public:
    bool addSession(MediaSession* session);
    bool removeSession(MediaSession* session);

    MediaSession* getSession(const std::string& name);

private:
    std::map<std::string, MediaSession*> mSessionMap;
};

#endif
