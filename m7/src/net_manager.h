/*
 * Gigaspark OS - Network Manager Header
 * WiFi connection management for Arduino Giga R1
 */

#ifndef GIGASPARK_NET_MANAGER_H
#define GIGASPARK_NET_MANAGER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/*
 * Initialize the network subsystem.
 * Must be called before any other net_* function.
 *
 * @return 0 on success, negative on error.
 */
int net_manager_init(void);

/*
 * Connect to a WiFi network.
 *
 * @param ssid     Network SSID (null-terminated).
 * @param psk      Network password (null-terminated), or NULL for open networks.
 * @return 0 on success, negative on error.
 */
int net_connect_wifi(const char *ssid, const char *psk);

/*
 * Wait for an IP address via DHCP.
 *
 * @param timeout_ms  Maximum time to wait in milliseconds.
 *                    Use -1 for infinite wait.
 * @return 0 on success (IP obtained), negative on error or timeout.
 */
int net_wait_ip(int32_t timeout_ms);

/*
 * Check if the network is connected and has an IP.
 *
 * @return true if connected, false otherwise.
 */
bool net_is_connected(void);

/*
 * Get the current IP address as a string.
 *
 * @param buf      Buffer to store the IP string.
 * @param buf_len  Size of the buffer.
 * @return 0 on success, negative on error.
 */
int net_get_ip_str(char *buf, size_t buf_len);

/*
 * Disconnect from the current network.
 *
 * @return 0 on success, negative on error.
 */
int net_disconnect(void);

#endif /* GIGASPARK_NET_MANAGER_H */
