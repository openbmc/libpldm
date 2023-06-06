#ifndef SOCKET_H
#define SOCKET_H

#ifdef __cplusplus
extern "C" {
#endif

int get_max_buf_size(void);
int set_socket_send_buf(int socket, int max_buf_size, int curr_buf_size,
			int msg_len);
int get_socket_send_buf_size(int socket);

#ifdef __cplusplus
}
#endif

#endif // SOCKET_H
