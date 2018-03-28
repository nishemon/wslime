#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include "wslime.h"

static void
submit(int out_fd, char cmd, int p1, int p2)
{
  static char buff[32];
  sprintf(buff + 1, " %d %d\n", p1, p2);
  buff[0] = cmd;
  size_t toWrite = strlen(buff);
  ssize_t written = 0;
  do {
    written += write(out_fd, buff, toWrite);
  } while (written < toWrite);
}

static wslime_relay_connection g_con;

wslime_relay_connection*
wslime_relay_init()
{
    int cmdpipe[2];
    int respipe[2];
    pipe(cmdpipe);
    pipe(respipe);
    pid_t cpid = fork();
    if (cpid == 0) {
      close(respipe[0]);
      close(cmdpipe[1]);
      close(STDIN_FILENO);
      close(STDOUT_FILENO);
      dup2(cmdpipe[0], STDIN_FILENO);
      dup2(respipe[1], STDOUT_FILENO);
      close(cmdpipe[0]);
      close(respipe[1]);
      char* const argv[] = {
        "\\\\.\\pipe\\wslime",
        "injector",
        NULL
      };
      execve("/mnt/c/Users/takai/source/repos/wslime/x64/Debug/bridge.exe",  argv, argv + 2);
    }
    close(respipe[1]);
    close(cmdpipe[0]);
    g_con.in_fd = respipe[0];
    g_con.out_fd = cmdpipe[1];
    return &g_con;
}

void wslime_relay_wakeup(wslime_relay_connection* con)
{
    submit(con->out_fd, 'O', 1, 0);
}

void wslime_relay_sleep(wslime_relay_connection* con)
{
    submit(con->out_fd, 'O', 0, 0);
}

void wslime_relay_setpoint(wslime_relay_connection* con, int x, int y)
{
    submit(con->out_fd, 'P', x, y);
}

static const char* parseCLine(const char* line, size_t len, int values[])
{
  const char* end = (char*)memchr(line, '\n', len);
  if (end == NULL) {
    end = line + len;
  }
  int i = 0;
  char *next;
  for (const char* rp = line; rp < end; rp = next + 1) {
    values[i++] = strtol(rp, &next, 10);
  }
  return end + 1;
}

static const char* copyLine(char* buff, const char* line, size_t len) {
  const char* end = (char*)memchr(line, '\n', len);
  if (end == NULL) {
    end = line + len;
  }
  memcpy(buff, line, end - line);
  buff[end - line] = '\0';
  return end + 1;
}

static wslime_composition wc = { 0 };
static char commit_buff[256];
static char str_buff[256];

wslime_composition*
wslime_relay_update(wslime_relay_connection* con, int key, int state)
{
  struct timeval tv;
  fd_set rfds;
  tv.tv_sec = 1;
  tv.tv_usec = 100000;
  FD_ZERO(&rfds);
  FD_SET(con->in_fd, &rfds);
  wslime_composition* ret = NULL;
  memset(&wc, 0, sizeof(wc));
  submit(con->out_fd, 'I', key, state);
  for (;;) {
    int sel = select(FD_SETSIZE, &rfds, NULL, NULL, &tv);
    if (sel <= 0) {
      return ret;
    }
    char buff[1024];
    int buffering = read(con->in_fd, buff, sizeof(buff));
    const char* rp = buff;
    ret = &wc;
    const char* end = buff + buffering;
    while (rp < end) {
      if (*rp == 'C') {
        int values[5];
        rp = parseCLine(rp + 2, end - rp - 2, values);
        if (values[0]) {
          rp = copyLine(commit_buff, rp, end - rp);
          wc.commit = commit_buff;
        }
        if (values[1]) {
          rp = copyLine(str_buff, rp, end - rp);
          wc.str = str_buff;
        }
      } else {
        const char* eol = (char*)memchr(rp, '\n', end - rp);
        if (eol == NULL) {
          rp = end;
        } else {
          rp = eol + 1;
        }
      }
    }
    memset(&tv, 0, sizeof(tv));
  }
}

