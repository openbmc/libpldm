#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

int get_max_buf_size(void)
{
	FILE *fp;
	long max_buf_size;
	char line[128];
	char *endptr;
	fp = fopen("/proc/sys/net/core/wmem_max", "r");
	if (fp == NULL || fgets(line, 128, fp) == NULL) {
		fclose(fp);
		return -1;
	}

	errno = 0;
	max_buf_size = strtol(line, &endptr, 10);
	if (errno != 0 || endptr == line) {
		fclose(fp);
		return -1;
	}
	fclose(fp);
	return (int)max_buf_size;
}

int set_socket_send_buf(int socket, int max_buf_size, int curr_buf_size,
			int msg_len)
{
	/* If message is bigger than the max size, don't return a failure. Set
	 * the buffer to the max size and see what happens. We don't know how
	 * much of the extra space the kernel actually uses so let it tell us if
	 * there wasn't enough space */
	if (msg_len > max_buf_size) {
		msg_len = max_buf_size;
	}
	if (curr_buf_size == max_buf_size) {
		return curr_buf_size;
	}
	int rc = setsockopt(socket, SOL_SOCKET, SO_SNDBUF, &(msg_len),
			    sizeof(msg_len));
	if (rc == -1) {
		return -1;
	}
	return msg_len;
}

int get_socket_send_buf_size(int socket)
{
	/* size returned by getsockopt is the actual size of the buffer - twice
	 * the size of the value used by setsockopt. So for consistency, return
	 * half of the buffer size */
	int buf_size;
	socklen_t optlen = sizeof(buf_size);
	int rc =
		getsockopt(socket, SOL_SOCKET, SO_SNDBUF, &(buf_size), &optlen);
	if (rc == -1) {
		return -1;
	}
	return buf_size / 2;
}
