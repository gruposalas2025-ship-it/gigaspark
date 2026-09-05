/*
 * Gigaspark OS - IPC Configuration
 * Common definitions for M7/M4 IPC communication
 */

#ifndef GIGASPARK_IPC_CONFIG_H
#define GIGASPARK_IPC_CONFIG_H

/* RPMsg endpoint name - must match on both cores */
#define IPC_ENDPOINT_NAME	"gigaspark_ept"

/* Protocol messages */
#define MSG_BOOT_REQUEST	"BOOT_GIGASPARK"
#define MSG_BOOT_RESPONSE	"M4_KERNEL_READY"

/* Buffer sizes */
#define IPC_BUFFER_SIZE		1024

#endif /* GIGASPARK_IPC_CONFIG_H */
