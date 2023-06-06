#ifndef LIBPLDM_SRC_TRANSPORT_SOCKET_H
#define LIBPLDM_SRC_TRANSPORT_SOCKET_H

struct pldm_socket_sndbuf
{
	int size;
	int max_size;
};

int pldm_socket_sndbuf_init(struct pldm_socket_sndbuf *ctx);
int pldm_socket_sndbuf_set(struct pldm_socket_sndbuf *ctx, int socket, int msg_len);
int pldm_socket_sndbuf_get(struct pldm_socket_sndbuf *ctx, int socket);

#endif // LIBPLDM_SRC_TRANSPORT_SOCKET_H
