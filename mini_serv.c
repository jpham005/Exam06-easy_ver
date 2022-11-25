#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>

static const int localhost = 2130706433;
static const int initial_cap = 1;

#define GET_CLIENT _data_holder._client
#define GET_MSG _data_holder._msg

int nfds;

fd_set readfds;
fd_set writefds;

char sprintf_buf[256];

typedef struct vector vector;

typedef struct client {
  int _fd;
  int _id;
  vector* _msg_buffer;
} client;

typedef union vector_data_type {
  void* _data;
  client* _client;
  char* _msg;
} vector_data_type;

struct vector {
  vector_data_type _data_holder;
  size_t _size;
  size_t _cap;
};

vector client_vec;

static void ft_putstr_fd(const char* str, int fd) {
  const size_t target_len = strlen(str);
  size_t total_written = 0;
  ssize_t curr;

  while (total_written < target_len) {
    curr = write(fd, str, target_len - total_written);
    if (curr < 0) {
      exit(EXIT_FAILURE);
    }

    total_written += (size_t) curr;
  }
}

static void syscall_err() {
  ft_putstr_fd("Fatal Error\n", STDERR_FILENO);
  exit(EXIT_FAILURE);
}

void init_vector(vector* vec, size_t el_size) {
  vec->_data_holder._data = malloc(el_size * initial_cap);
  if (!vec->_data_holder._data) {
    syscall_err();
  }

  vec->_size = 0;
  vec->_cap = initial_cap;
}

void init_client(client* cli, int fd, int id) {
  cli->_fd = fd;
  cli->_id = id;
  cli->_msg_buffer = malloc(sizeof(vector));
  if (!cli->_msg_buffer) {
    syscall_err();
  }
  init_vector(cli->_msg_buffer, sizeof(char));
}

void append_client(vector* clients, int fd, int id) {
  if (clients->_size >= clients->_cap) {
    void* temp = realloc(clients->_data_holder._data, sizeof(client) * clients->_cap << 1);
    if (!temp) {
      syscall_err();
    }

    clients->_cap <<= 1;
  }

  init_client(&(clients->GET_CLIENT[clients->_size++]), fd, id);
}

void delete_client(vector* vec, int fd) {
  size_t i = 0;
  while (i < vec->GET_CLIENT[i]._fd == fd)
    i++;

  if (nfds == vec->GET_CLIENT[i]._id) {
    nfds--;
  }

  free(vec->GET_CLIENT[i]._msg_buffer);
  memcpy(&vec->GET_CLIENT[i], &vec->GET_CLIENT[i + 1], (vec->_size - i - 1) * sizeof(client));
}

static bool is_valid_argc(int argc) {
  return argc == 2;
}

static void handle_arg_err() {
  ft_putstr_fd("Wrong number of Arguments\n", STDERR_FILENO);
  exit(1);
}

static int get_port(const char* str) {
  return atoi(str);
}

static int init_server(int port) {
  int server = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (server == -1) {
    syscall_err();
  }

  struct sockaddr_in addr;
  bzero(&addr, sizeof(addr));

  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = htonl(localhost);

  if (bind(server, (struct sockaddr*) &addr, sizeof(addr)) == -1) {
    syscall_err();
  }

  if (listen(server, SOMAXCONN) == -1) {
    syscall_err();
  }

  return server;
}

static void init_fd_set(int server) {
  nfds = server + 1;

  FD_ZERO(&readfds);
  FD_ZERO(&writefds);

  FD_SET(server, &readfds);
}

void append_string(vector* string_buffer, const char* str) {
  const size_t append_len = strlen(str);

  while (string_buffer->_size + append_len >= string_buffer->_cap) {
    void* temp = realloc(string_buffer->GET_MSG, string_buffer->_cap << 1);
    if (!temp) {
      syscall_err();
    }

    string_buffer->_cap <<= 1;
  }

  for (size_t i = 0; i < append_len; i++) {
    string_buffer->GET_MSG[string_buffer->_size++] = str[i];
  }
}

void add_msg(vector* clients, const char* msg) {
  for (size_t i = 0; i < clients->_size; i++) {
    append_string(clients->GET_CLIENT->_msg_buffer, msg);
  }
}

static void accept_client(int server) {
  static int id;

  int client = accept(server, NULL, NULL);
  if (client == -1) {
    syscall_err();
  }

  sprintf(sprintf_buf, "server: client %d has arrived\n", id);
  add_msg(&client_vec, sprintf_buf);

  append_client(&client_vec, client, id++);
}

static void run_server(int server) {
  fd_set cp_read;
  fd_set cp_write;

  init_vector(&client_vec, sizeof(client));
  while (true) {
    FD_COPY(&readfds, &cp_read);
    FD_COPY(&writefds, &cp_write);

    if (select(nfds, &cp_read, &cp_write, NULL, NULL) == -1) {
      syscall_err();
    }

    for (int i = 0; i < nfds; i++) {
      if (FD_ISSET(i, &cp_read)) {
        if (i == server) {
          accept_client(server);
        } else {
//          recv_client(i);
        }
      }

      if (FD_ISSET(i, &cp_write)) {
//        send_client(i);
      }
    }

    for (size_t i = 0; i < client_vec._size; i++) {
      printf("%s", client_vec.GET_CLIENT[i]._msg_buffer->GET_MSG);
    }
  }
}

int main(int argc, char** argv) {
  if (!is_valid_argc(argc)) {
    handle_arg_err();
  }

  int server = init_server(get_port(argv[1]));
  init_fd_set(server);
  run_server(server);
}
