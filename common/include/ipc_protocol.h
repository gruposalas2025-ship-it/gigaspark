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
	CMD_READ     = 0x03,  /* M7 -> M4: read data from handle */
	CMD_WRITE    = 0x04,  /* M7 -> M4: write data to handle */
	CMD_RESPONSE = 0x10,  /* M4 -> M7: response with handle or error */
};

/* Response status codes */
enum ipc_status {
	STATUS_OK       = 0,
	STATUS_ERR_NOMEM     = -1,
	STATUS_ERR_BAD_HANDLE = -2,
	STATUS_ERR_COMPRESSED_FULL = -3,
	STATUS_ERR_READ_FAILED = -4,
	STATUS_ERR_SD_FAILED = -5,
	STATUS_ERR_WRITE_FAILED = -6,
};

/* IPC message structure (fixed size for OpenAMP safety) */
struct __packed ipc_msg {
	uint8_t  cmd;       /* enum ipc_cmd */
	int8_t   status;    /* enum ipc_status (response only) */
	uint16_t handle;    /* opaque memory handle */
	uint32_t size;      /* requested size (CMD_ALLOC) or actual size */
};

/* Maximum memory pool size managed by M4 */
#define MEM_POOL_SIZE       4096

/* Compressed storage pool (half of main pool) */
#define COMPRESSED_POOL_SIZE 2048

/* Maximum single allocation size */
#define MEM_MAX_ALLOC       1024

/* Maximum read/write payload that fits in a single IPC message */
#define MEM_IO_MAX          24

/* Handle value meaning "invalid" */
#define MEM_HANDLE_INVALID  0xFFFF

#endif /* GIGASPARK_IPC_PROTOCOL_H */
