//
// Localsocket implementation
// Created by dk on 2017/5/4.
//

#ifndef HARDCODER_LOCALSOCKET_H
#define HARDCODER_LOCALSOCKET_H

#include "util.h"
#include <sys/stat.h>
#include <sys/un.h>
#include <asm/ioctls.h>

class LocalSocket {

protected:
    typedef enum {
        EVENT_ERROR, EVENT_DATA, EVENT_SESSION_FINISH, EVENT_CONNECT, EVENT_SYSTEM
    } Event;

    virtual int recvEvent(Event event, int fd, uid_t uid, const char *path, uint8_t *data, int len) = 0;

private:

    typedef struct SendPack {
        int fd;
        uint8_t *data;
        uint32_t len;
        uint32_t offset;

        SendPack() {
            fd = 0;
            data = NULL;
            len = 0;
            offset = 0;
        }

    } SendPack;

    int m_fd;
    int m_pipefd[2];
    map<int, pair<uid_t, string> > mapUid;

    rqueue<SendPack *, 5> m_sendQueue;


    static int setSocketNonBlock(int fd) {
        int flags = fcntl(fd, F_GETFL, 0);
        if (flags < 0) {
            perr("fcntl F_GETFL  failed fd:%d flag:%d", fd, flags);
            return -1;
        }
        int ret = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
        if (ret < 0) {
            perr("fcntl F_SETFL failed fd:%d ret:%d ", fd, ret);
            return -2;
        }
        return 0;
    }

    static int checkCanWrite(int fd) {
        fd_set fdset;
        FD_ZERO(&fdset);
        FD_SET(fd, &fdset);
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;
        select(fd + 1, NULL, &fdset, NULL, &timeout);
        pdbg(" FD_ISSET(fd, &fdset) :%d fd:%d", FD_ISSET(fd, &fdset), fd);
        return FD_ISSET(fd, &fdset);
    }

    bool is_uid_authorized(uint32_t uid) {
        int ret = -1;
        char value[PROP_VALUE_MAX];
        char* nexttok = nullptr;
        char* uid_str = nullptr;

        ret = __system_property_get("persist.sys.mispeed_auth_uids", value);
        if (ret < 0)
            return false;

        nexttok = value;
        while ((uid_str = strsep(&nexttok, ","))) {
            pdbg("is_uid_authorized uid: %d, auth_uid_str:%s", uid, uid_str);
            if (atoi(uid_str) == (int)uid)
                return true;
        }

        return false;
    }

    int socketAccept(int fd) {

        struct sockaddr_un un;
        socklen_t len = sizeof(un);
        int acceptFd = accept(fd, (struct sockaddr *) &un, &len);
        if (acceptFd < 0) {
            perr("accept error fd:%d ret:%d ", fd, acceptFd);
            return -1;
        }

        if (0 > setSocketNonBlock(acceptFd)) {
            perr("setSocketNonBlock error acceptFd:%d", acceptFd);
            return -2;
        }

        len -= offsetof(struct sockaddr_un, sun_path);
        un.sun_path[len] = 0;
        char *path = un.sun_path + 1; // pos 0 is \0
        struct ucred ucred;
        len = sizeof(struct ucred);
        getsockopt(acceptFd, SOL_SOCKET, SO_PEERCRED, &ucred, &len);
        pdbg("getsockopt pid:%d uid:%d gid:%d len:%d", ucred.pid, ucred.uid, ucred.gid, len);

        uid_t uid = ucred.uid;

        if (!is_uid_authorized(uid)) {
            perr("NotAccept uid:%d path:%s not permitted!!!", uid, path);
            close(acceptFd);
            return -99;
        }

        pwrn("Accept fd:%d acceptFd:%d uid:%d path:%s ", m_fd, acceptFd, uid, path);

        if (0 > recvEvent(EVENT_CONNECT, acceptFd, uid, path, NULL, 0)) {
            perr("NotAccept fd:%d uid:%d path:%s", acceptFd, uid, un.sun_path);
            close(acceptFd);
            return -4;
        }

        pair<uid_t, string> p;
        p.first = uid;
        p.second = un.sun_path;
        mapUid[acceptFd] = p;
        return acceptFd;
    }

    void notifyError(int recvFd) {
        if (recvFd == m_fd) { //client
            recvEvent(EVENT_ERROR, recvFd, 0, NULL, NULL, 0);
            return;
        }
        recvEvent(EVENT_ERROR, recvFd, mapUid[recvFd].first, mapUid[recvFd].second.c_str(), NULL, 0);
        close(recvFd); //todo clean select & mapuid
    }

    int loop(const int isServer) {

        fd_set fdset;
        FD_ZERO(&fdset);
        FD_SET(m_fd, &fdset);
        FD_SET(m_pipefd[0], &fdset);
        int maxfd = m_fd;

        pdbg("mfd:%d  maxfd:%d isServer:%d", m_fd, maxfd, isServer);

        while (m_fd) {
            const int64_t start = getMillisecond();

            SendPack *sendPack = m_sendQueue.front();
            if (sendPack && checkCanWrite(sendPack->fd)) {

                int ret = send(sendPack->fd, sendPack->data + sendPack->offset, sendPack->len - sendPack->offset, 0); //todo process error
                if(ret <  0){
                    perr("send failed fd:%d mfd:%d", sendPack->fd, m_fd);
                    if(sendPack->fd == m_fd){ //client
                        notifyError(sendPack->fd);
                        break;
                    }
                    m_sendQueue.pop();
                    delete[]sendPack->data;
                    delete sendPack;
                    FD_CLR(sendPack->fd, &fdset);
                    notifyError(sendPack->fd);
                    continue;
                }

                if (ret > 0) sendPack->offset += ret;

                if (sendPack->offset == sendPack->len) {
                    m_sendQueue.pop();
                    delete[]sendPack->data;
                    delete sendPack;
                }

                pdbg("send: ret:%d fd:%d len:%d off:%d queue:%d time:%d ", ret, sendPack->fd, sendPack->len, sendPack->offset,
                     m_sendQueue.size(), TOINT(getMillisecond() - start) );
            }
//          set timeout = null to improve cpu idle current
//          struct timeval timeout;
//          timeout.tv_sec = 1;
//          timeout.tv_usec = 0;
            fd_set tmpSet = fdset;
            int selectRet = select(maxfd + 1, &tmpSet, NULL, NULL, NULL);
            if (selectRet == -1 && errno != EINTR) {
                pdbg("loop: select errno:%d", errno);
                continue;
            }
            else if (selectRet == 0) {
//                pdbg("loop: select timeout continue ");
                continue;
            }

            for (int i = 3; i <= maxfd; i++) {
                if ((!FD_ISSET(i, &tmpSet)) || (!FD_ISSET(i, &fdset))) continue;

                if (m_pipefd[0] == i) {
                    int l = 0;
                    int ret = read(m_pipefd[0], &l, sizeof(int));
                    UNUSED(ret);
//                    pdbg("read pipe ret:%d run:%d ", ret, getMillisecond() - start);
                }
                else if (isServer && i == m_fd) {

                    int acceptfd = socketAccept(m_fd);
                    if (acceptfd == -99) {
                        continue;
                    }
                    if (acceptfd <= 0) {
                        return -1;
                    }
                    FD_SET(acceptfd, &fdset);
                    maxfd = maxfd > acceptfd ? maxfd : acceptfd;
                } else {
                    uint8_t szBuf[4096];
                    int ret = recv(i, szBuf, sizeof(szBuf), 0);
                    if(ret <= 0){
                        perr("recv error , fd:%d ret:%d errno:%d", i, ret, errno);
                        FD_CLR(i, &fdset);
                        notifyError(i);
                        continue;
                    }
                    pdbg("recv fd:%d uid:%d ret:%d", i, mapUid[i].first ,ret);
                    if (mapUid[i].first > 1000) {
                        recvEvent(EVENT_DATA, i, mapUid[i].first, mapUid[i].second.c_str(), szBuf, ret);
                    } else {
                        // recv system event
                        recvEvent(EVENT_SYSTEM, i, mapUid[i].first, mapUid[i].second.c_str(), szBuf, ret);
                    }
                }
            }
        }
        uninit();
        return 0;
    }

protected:
    void uninit(){
        pwrn("uninit pipe:%d,%d m_fd:%d queue:%d", m_pipefd[0], m_pipefd[1], m_fd, m_sendQueue.size());
        if (m_pipefd[0]) {
            close(m_pipefd[0]);
            m_pipefd[0] = 0;
        }
        if (m_pipefd[1]) {
            close(m_pipefd[1]);
            m_pipefd[1] = 0;
        }
        if (m_fd) {
            close(m_fd);
            m_fd = 0;
        }
        while (m_sendQueue.size()) {
            SendPack *s = m_sendQueue.pop();
            delete[]s->data;
            delete s;
        }
    }

public:
    LocalSocket() {
        m_pipefd[0] = 0;
        m_pipefd[1] = 0;
        m_fd = 0;
    }


    virtual ~LocalSocket() {
        uninit();
    }

    int createSocket(const char *localPath, const char *remotePath) {
        pdbg("createSocket localPath: %s remotePath: %s", localPath, remotePath);
        if (m_fd > 0) return m_fd;

        int ret = pipe(m_pipefd);
        if (ret < 0) {
            perr("create pipe ret:%d", ret);
            return -1;
        }
        const int fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (fd < 0) {
            perr("create socket fd:%d", fd);
            return -1;
        }

        struct sockaddr_un un;
        socklen_t len;
        if (remotePath == NULL || strlen(remotePath) == 0) {  //server
            unlink(localPath);

            memset(&un, 0, sizeof(un));
            un.sun_family = AF_UNIX;
            un.sun_path[0] = '\0';
            memcpy(un.sun_path + 1, localPath, strlen(localPath));
            len = offsetof(struct sockaddr_un, sun_path) + strlen(localPath) + 1;
            ret = ::bind(fd, (struct sockaddr *) &un, len);
            if (ret < 0) {
                close(fd);
                perr("bind ret:%d fd:%d localPath:%s", ret, fd, localPath);
                return -2;
            }

            ret = listen(fd, 5);
            if (ret < 0) {
                close(fd);
                perr("listen ret:%d fd:%d localPath:%s", ret, fd, localPath);
                return -3;
            }
            m_fd = fd;
            pwrn("create Server socket:%d local:%s ", m_fd, localPath);
            return fd;
        }

        memset(&un, 0, sizeof(un));
        un.sun_family = AF_UNIX;
        un.sun_path[0] = '\0';
        memcpy(un.sun_path + 1, remotePath, strlen(remotePath));
        len = offsetof(struct sockaddr_un, sun_path) + strlen(remotePath) + 1;

        // blocking connect implementation
//        ret = connect(fd, (struct sockaddr *) &un, len);

//      non-blocking connect implementation
        int opt = 1;
        //set non-blocking
        if (ioctl(fd, FIONBIO, &opt) < 0) {
            close(fd);
            perr("connect ioctl FIONBIO 1 failed ret:%d fd:%d path:%s", ret, fd, remotePath);
            return -3;
        }

        ret = connect(fd, (struct sockaddr *) &un, len);
        pwrn("connect socket ret:%d local:%s remote:%s", ret, localPath, remotePath);
        if (ret == -1) {
            if (errno == EINPROGRESS) {
                struct timeval tv_timeout = {5, 0};
                fd_set fdset;
                FD_ZERO(&fdset);
                FD_SET(fd, &fdset);
                if (select(fd + 1, NULL, &fdset, NULL, &tv_timeout) > 0) {
                    int scklen = sizeof(int);
                    int error;
                    getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, (socklen_t *)&scklen);
                    if(error != 0) {
                        close(fd);
                        perr("connect select getsockopt failed ret:%d fd:%d path:%s", ret, fd, remotePath);
                        return -3;
                    }
                } else { //timeout or select error
                    close(fd);
                    perr("connect select timeout ret:%d fd:%d path:%s", ret, fd, remotePath);
                    return -3;
                }
            } else {
                close(fd);
                perr("connect failed ret:%d fd:%d path:%s", ret, fd, remotePath);
                return -3;
            }
        }

        opt = 0;
        //set blocking
        if (ioctl(fd, FIONBIO, &opt) < 0) {
            close(fd);
            perr("connect ioctl FIONBIO 0 failed ret:%d fd:%d path:%s", ret, fd, remotePath);
            return -3;
        }


        if (ret < 0) {
            close(fd);
            perr("connect ret:%d fd:%d path:%s", ret, fd, remotePath);
            return -3;
        }
        m_fd = fd;
        pwrn("create ClientProtocal socket:%d local:%s remote:%s", m_fd, localPath, remotePath);
        return fd;
    }

    int startLoop(int isServer){
        return loop(isServer);
    }

    int64_t clientSendData(const uint8_t *data, const int len) {
        return sendData(m_fd, data, len);
    }

    int64_t sendData(const int fd, const uint8_t *data, const int len) {
        if (data == NULL || len <= 0) {
            return -1;
        }
        SendPack *sendPack = new SendPack();
        sendPack->fd = fd;
        sendPack->data = new uint8_t[len];
        memcpy(sendPack->data, data, len);
        sendPack->len = len;
        sendPack->offset = 0;
        m_sendQueue.push(sendPack);
        write(m_pipefd[1], &m_pipefd[1], sizeof(int));
        return 0;
    }

    // find client socket fd
    // TODO non-threadsafety
    int findFdByUid(uint32_t uid) {
        pdbg("findFdByUid uid:%d size:%d", uid, (int)mapUid.size());
        if (mapUid.empty()){
            return -1;
        }

        map<int, pair<uid_t, string> >::iterator iter;
        for(iter = mapUid.begin(); iter!=mapUid.end(); iter++) {
            if (iter->second.first == uid){
                pdbg("findFdByUid uid:%d found ret:%d", uid, iter->first);
                return iter->first;
            }
        }

        pdbg("findFdByUid uid:%d not found ret -1", uid);
        return -1;
    }
};

#endif //HARDCODER_LOCALSOCKET_H
