#include "UrlHookLog.h"
#include <cutils/log.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

#include "UrlHookCommand.h"
#include "socket_utils.h"

#include <vector>

#define BUFFER_SIZE (1024 * 4)
#define MAX_BUFFER (BUFFER_SIZE * 4)
#define MAX_CONNECTS 5

#define TAG LOG_TAG

#define FD_STDIN (0)

enum { kRunAsServer, kRunAsDebugger };

static const char *socket_name = "urlhook";
static int run_mode = kRunAsServer;
static const char **debug_argv = nullptr;
static int debug_argc = 0;

static void Parse(int argc, const char *argv[]) {
  int i = 1;
  while (i < argc) {
    if (strcmp(argv[i], "--addr") == 0) {
      socket_name = argv[++i];
    } else if (strcmp(argv[i], "--debug") == 0 || strcmp(argv[i], "-d") == 0) {
      run_mode = kRunAsDebugger;
      debug_argc = argc - i - 1;
      debug_argv = &argv[i + 1];
      return;
    } else if (strcmp(argv[i], "--help") == 0) {
      printf("%s\n", argv[0]);
      printf("--addr scoket_name\n");
    }
    i++;
  }
}

enum {
  kStateInit = 0,
  kStateSpace,
  kStateChar,
  kStateString1, // start "
  kStateString2, // start '
};

static void SaveBufferArg(int i, char *prev_buffer, char *buffer, int &argc,
                          int &state, char **argv) {
  buffer[i] = '\0';
  argv[argc++] = prev_buffer;
  state = kStateInit;
  prev_buffer = nullptr;
}

static int ParseBuffer(char *buffer, size_t size, char **argv, int max_argc) {
  // split buffer to argv
  size_t i = 0;
  int state = 0;
  int argc = 0;
  char *prev_buffer = nullptr;
  while (i < size - 1) {
    if (buffer[i] == ' ' || buffer[i] == '\t' || buffer[i] == '\r' ||
        buffer[i] == '\n') {
      if (kStateInit == state || kStateSpace == state) {
        state = kStateSpace;
      } else if (kStateChar == state) {
        SaveBufferArg(i, prev_buffer, buffer, argc, state, argv);
      }
    } else if (buffer[i] == '\'') {
      if (kStateInit == state || kStateSpace == state) {
        state = kStateString1;
        prev_buffer = buffer + i + 1;
      } else if (kStateString1 == state) { // string end
        SaveBufferArg(i, prev_buffer, buffer, argc, state, argv);
      }
    } else if (buffer[i] == '\"') {
      if (kStateInit == state || kStateSpace == state) {
        state = kStateString2;
        prev_buffer = buffer + i + 1;
      } else if (kStateString2 == state) {
        SaveBufferArg(i, prev_buffer, buffer, argc, state, argv);
      }
    } else {
      if (kStateInit == state || kStateSpace == state) {
        prev_buffer = buffer + i;
        state = kStateChar;
      }
    }

    if (argc > max_argc) {
      return argc;
    }

    i++;
  }

  if (i < size) {
    buffer[i] = '\0';
  }
  if (prev_buffer) {
    SaveBufferArg(i, prev_buffer, buffer, argc, state, argv);
  }
  return argc;
}

static void set_nonbolock(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static void RunUrlHookCommand(UrlHookCommand &command, SocketClient *client,
                              std::vector<char> &buffer) {
  int n;
  int start = 0;
  int fd = client->fd();
  errno = 0;
  DEBUG("buffer capacity: %d, start: %d", (int)buffer.capacity(), start);
  while ((n = read(fd, buffer.data() + start, buffer.capacity() - start)) > 0) {
    DEBUG("read data len=%d", n);
    start += n;
    if (start >= static_cast<int>(buffer.capacity())) {
      buffer.resize(buffer.capacity() + BUFFER_SIZE);
    }
  }

  if (n == 0) {
    DEBUG("RunUrlHookCommand read 0 end of file %s", strerror(errno));
    client->setClosing(true);
    return;
  } else if (n == -1) {
    DEBUG("RunUrlHookCommand read -1 %s", strerror(errno));
  } else {
    DEBUG("RunUrlHookCommand read result:%d\n", n);
  }

  //{
  //  // del me test
  //  std::string str(buffer.data(), buffer.size());
  //  ALOGE(TAG "recv data %d:%s", fd, str.c_str());
  //}

  char *cmd_args[64];
  int cmd_argc = ParseBuffer(buffer.data(), buffer.size(), cmd_args,
                             sizeof(cmd_args) / sizeof(cmd_args[0]));
  //ALOGE("cmd_argc:%d", cmd_argc);

  //{
  //  // del me test
  //  for (int i = 0; i < cmd_argc; i++) {
  //    ALOGE(TAG "args[%d]=\"%s\"", i, cmd_args[i]);
  //  }
  //}

  if (cmd_argc <= 0) {
    ALOGE(TAG "get the null command");
    return;
  }

  client->setCmdNum(strtol(cmd_args[0], nullptr, 0));

  if ((cmd_args[1] != nullptr) && (strncmp(cmd_args[1], "urlhook", strlen("urlhook")) == 0)) {
    command.runCommand(client, cmd_argc - 1, cmd_args + 1);
  } else {
    ALOGE(TAG "unkown command: %s", cmd_args[1]);
  }

  if (buffer.capacity() > MAX_BUFFER) {
    buffer.resize(MAX_BUFFER);
  }
}

static int RunServer() {
  int serv_fd;
  socklen_t addr_len;
  struct sockaddr_un addr;

  DEBUG("create server:%s", socket_name);
  InitUnixDomain(socket_name, &addr, &addr_len);

  serv_fd = socket(AF_UNIX, SOCK_STREAM, 0);

  set_nonbolock(serv_fd);

  if (bind(serv_fd, (struct sockaddr *)&addr, addr_len) != 0) {
    ALOGE("Cannot create server:%s", socket_name);
    close(serv_fd);
    return -1;
  }

  DEBUG("server fd:%d", serv_fd);

  listen(serv_fd, MAX_CONNECTS);

  std::vector<SocketClient *> clients;

  std::vector<char> buffer(BUFFER_SIZE);

  UrlHookCommand command;

  while (true) {
    fd_set readfds;
    fd_set exceptfds;
    FD_ZERO(&readfds);
    FD_ZERO(&exceptfds);

    int max_fd = serv_fd;
    FD_SET(serv_fd, &readfds);

    for (auto client : clients) {
      if (client != nullptr) {
        FD_SET(client->fd(), &readfds);
        FD_SET(client->fd(), &exceptfds);
        if (max_fd < client->fd()) {
          max_fd = client->fd();
        }
      }
    }

    int r = select(max_fd + 1, &readfds, nullptr, &exceptfds, nullptr);
    DEBUG(" select result: %d", r);
    if (r <= 0) {
      ALOGE(TAG " select error r = %d", r);
      break;
    }

    if (FD_ISSET(serv_fd, &readfds)) {
      // accept the fd
      struct sockaddr_un caddr;
      socklen_t clen = sizeof(caddr);
      int fd =
          accept(serv_fd, reinterpret_cast<struct sockaddr *>(&caddr), &clen);
      DEBUG("client connected: %d", fd);
      set_nonbolock(fd);
      SocketClient *client = new SocketClient(fd);
      clients.push_back(client);
      DEBUG("client creanted:%p", client);
    } else {
      for (auto client : clients) {
        if (client && FD_ISSET(client->fd(), &readfds)) {
          RunUrlHookCommand(command, client, buffer);
        } else if (client && FD_ISSET(client->fd(), &exceptfds)) {
          ALOGE(TAG "client fd:%d in exception", client->fd());
        }
      }
    }

    size_t i = 0;
    while (i < clients.size()) {
      SocketClient *client = clients[i];
      if (client && (FD_ISSET(client->fd(), &exceptfds) || client->getClosing())) {
        DEBUG(" delete client:%d", client->fd());
        delete client;
        clients.erase(clients.begin() + i);
      } else {
        i++;
      }
    }
  }

  for (auto client : clients) {
    if (client != nullptr) {
      delete client;
    }
  }
  return 0;
}

static void append_command(std::vector<char> &buffer, const char *cmd,
                           bool qutor_wrapp) {
  if (qutor_wrapp) {
    buffer.push_back('\"');
  }

  for (int i = 0; cmd[i]; i++) {
    if (cmd[i] == '\"' || cmd[i] == '\'') {
      buffer.push_back('\\');
    }
    buffer.push_back(cmd[i]);
  }

  if (qutor_wrapp) {
    buffer.push_back('\"');
  }
}

static int RunDebugger() {
  struct sockaddr_un addr;
  socklen_t addr_len;

  InitUnixDomain(socket_name, &addr, &addr_len);

  int fd = socket(AF_UNIX, SOCK_STREAM, 0);

  int ret =
      connect(fd, reinterpret_cast<const struct sockaddr *>(&addr), addr_len);

  if (ret != 0) {
    ALOGE(TAG "debugger connect %s failed: %s", socket_name, strerror(errno));
    close(fd);
    return -1;
  }

  ALOGE(TAG "debugger connected %s", socket_name);

  std::vector<char> buffer(BUFFER_SIZE);

  char *argv[64];
  char str_number[] = "0";
  char str_urlhook[] = "urlhook";
  argv[0] = str_number;
  argv[1] = str_urlhook;

  append_command(buffer, "0", false);
  buffer.push_back(' ');
  append_command(buffer, "urlhook", false);

  for (int i = 0; i < debug_argc; i++) {
    buffer.push_back(' ');
    append_command(buffer, debug_argv[i], true);
  }

  // send command
  ssize_t n = write(fd, buffer.data(), buffer.size());

  if (n != static_cast<ssize_t>(buffer.size())) {
    buffer.push_back('\0');
    ALOGE(TAG "Write command: \"%s\" faield: %s", buffer.data(),
          strerror(errno));
    close(fd);
    return -1;
  }

  read(fd, buffer.data(), buffer.capacity() - 1);
  buffer.push_back('\0');

  ALOGE(TAG "%s", buffer.data());
  printf("%s\n", buffer.data());

  close(fd);
  return 0;
}

int main(int argc, const char *argv[]) {
  Parse(argc, argv);

  if (run_mode == kRunAsDebugger) {
    return RunDebugger();
  } else if (run_mode == kRunAsServer) {
    return RunServer();
  } else {
    ALOGE(TAG "unkwon run mode:%d", run_mode);
    return -1;
  }
}
