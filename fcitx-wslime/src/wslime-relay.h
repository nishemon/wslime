
#define WSLIME_IMPL(x) wslime_relay_ ## x

typedef struct {
  int in_fd;
  int out_fd;
} wslime_relay_connection;

