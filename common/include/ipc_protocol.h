/*
 * Gigaspark OS - IPC Protocol Definitions
 * Shared between M7 (host) and M4 (remote memory manager)
 */

#ifndef GIGASPARK_IPC_PROTOCOL_H
#define GIGASPARK_IPC_PROTOCOL_H

#include <stdint.h>

/* Command types */
enum ipc_cmd {
	CMD_ALLOC    = 0x01,  /* M7 -> M4: allocate memory */
	CMD_FREE     = 0x02,  /* M7 -> M4: free memory by handle */
	CMD_RESPONSE = 0x10,  /* M4 -> M7: response with handle or error */
};

/* Response status codes */
enum ipc_status {
	STATUS_OK       = 0,
	STATUS_ERR_NOMEM = -1,
	STATUS_ERR_BAD_HANDLE = -2,
};

/* IPC message structure (fixed size for OpenAMP safety) */
struct __packed ipc_msg {
	uint8_t  cmd;       /* enum ipc_cmd */
	int8_t   status;    /* enum ipc_status (response only) */
	uint16_t handle;    /* opaque memory handle */
	uint32_t size;      /* requested size (CMD_ALLOC) or actual size */
};

/* Maximum memory pool size managed by M4 */
#define MEM_POOL_SIZE   4096

/* Maximum single allocation size */
#define MEM_MAX_ALLOC   1024

/* Handle value meaning "invalid" */
#define MEM_HANDLE_INVALID  0xFFFF

#endif /* GIGASPARK_IPC_PROTOCOL_H */
