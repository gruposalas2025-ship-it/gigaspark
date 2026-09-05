/*
 * Gigaspark OS - IPC Protocol Definitions
 * Shared between M7 (host) and M4 (remote core)
 */

#ifndef GIGASPARK_IPC_PROTOCOL_H
#define GIGASPARK_IPC_PROTOCOL_H

#include <stdint.h>

/* Command types */
enum ipc_cmd {
	CMD_ALLOC         = 0x01,  /* M7 -> M4: allocate memory */
	CMD_FREE          = 0x02,  /* M7 -> M4: free memory by handle */
	CMD_READ          = 0x03,  /* M7 -> M4: read data from handle */
	CMD_WRITE         = 0x04,  /* M7 -> M4: write data to handle */
	CMD_GET_APP_LIST  = 0x10,  /* M7 -> M4: get list of apps from SD */
	CMD_LOAD_APP      = 0x11,  /* M7 -> M4: load app binary by id */
	CMD_RESPONSE      = 0x80,  /* M4 -> M7: response with data */
};

/* Response status codes */
enum ipc_status {
	STATUS_OK              = 0,
	STATUS_ERR_NOMEM       = -1,
	STATUS_ERR_BAD_HANDLE  = -2,
	STATUS_ERR_COMPRESSED_FULL = -3,
	STATUS_ERR_READ_FAILED = -4,
	STATUS_ERR_SD_FAILED   = -5,
	STATUS_ERR_WRITE_FAILED = -6,
	STATUS_ERR_NO_APPS     = -7,
	STATUS_ERR_APP_NOT_FOUND = -8,
};

/* IPC message structure (fixed size for OpenAMP safety) */
struct __packed ipc_msg {
	uint8_t  cmd;       /* enum ipc_cmd */
	int8_t   status;    /* enum ipc_status (response only) */
	uint16_t handle;    /* opaque memory handle */
	uint32_t size;      /* requested/actual size */
};

/* Maximum memory pool size managed by M4 */
#define MEM_POOL_SIZE       4096

/* Compressed storage pool */
#define COMPRESSED_POOL_SIZE 2048

/* Maximum single allocation size */
#define MEM_MAX_ALLOC       1024

/* Maximum read/write payload per IPC message */
#define MEM_IO_MAX          24

/* Handle value meaning "invalid" */
#define MEM_HANDLE_INVALID  0xFFFF

/* Application info structure (sent via IPC) */
#define APP_NAME_MAX    16
#define APP_MAX_COUNT   8

struct __packed app_info {
	uint8_t  id;                    /* unique app id */
	char     name[APP_NAME_MAX];   /* display name (null-terminated) */
};

/* Max app binary size loadable into memory */
#define APP_BINARY_MAX_SIZE  4096

#endif /* GIGASPARK_IPC_PROTOCOL_H */
