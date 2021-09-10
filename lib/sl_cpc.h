/***************************************************************************//**
 * @file
 * @brief Co-Processor Communication Protocol(CPC) - Library Header
 * @version 3.2.0
 *******************************************************************************
 * # License
 * <b>Copyright 2021 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/

#ifndef SL_CPC_H
#define SL_CPC_H

#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include "sl_enum.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define SL_CPC_FLAG_NON_BLOCK  (1 << 0)

SL_ENUM(cpc_endpoint_state_t){
  SL_CPC_STATE_OPEN = 0,
  SL_CPC_STATE_CLOSED,
  SL_CPC_STATE_CLOSING,
  SL_CPC_STATE_ERROR_DESTINATION_UNREACHABLE,
  SL_CPC_STATE_ERROR_SECURITY_INCIDENT,
  SL_CPC_STATE_ERROR_FAULT
};

SL_ENUM(cpc_option_t){
  CPC_OPTION_NONE = 0,
  CPC_OPTION_BLOCKING,
  CPC_OPTION_RX_TIMEOUT,
  CPC_OPTION_TX_TIMEOUT,
  CPC_OPTION_SOCKET_SIZE,
  CPC_OPTION_MAX_WRITE_SIZE
};

SL_ENUM(sl_cpc_service_endpoint_id_t){
  SL_CPC_ENDPOINT_SYSTEM = 0,                  ///< System control
  SL_CPC_ENDPOINT_SECURITY = 1,                ///< Security - related functionality
  SL_CPC_ENDPOINT_BLUETOOTH = 2,               ///< Bluetooth(BGAPI) endpoint
  SL_CPC_SLI_CPC_ENDPOINT_RAIL_DOWNSTREAM = 3, ///< RAIL downstream endpoint
  SL_CPC_SLI_CPC_ENDPOINT_RAIL_UPSTREAM = 4,   ///< RAIL upstream endpoint
  SL_CPC_ENDPOINT_ZIGBEE = 5,                  ///< ZigBee EZSP endpoint
  SL_CPC_ENDPOINT_ZWAVE = 6,                   ///< Z-Wave endpoint
  SL_CPC_ENDPOINT_CONNECT = 7,                 ///< Connect endpoint
  SL_CPC_ENDPOINT_GPIO = 8,                    ///< GPIO endpoint for controlling GPIOs on SECONDARYs
  SL_CPC_ENDPOINT_OPENTHREAD = 9,              ///< Openthread Spinel endpoint
  SL_CPC_ENDPOINT_WISUN = 10,                  ///< WiSun endpoint
  SL_CPC_ENDPOINT_WIFI = 11,                   ///< WiFi endpoint(main control)
  SL_CPC_ENDPOINT_15_4 = 12,                   ///< 802.15.4 endpoint
  SL_CPC_ENDPOINT_CLI = 13,                    ///< Ascii based CLI for stacks / applications
};

SL_ENUM(sl_cpc_user_endpoint_id_t){
  SL_CPC_ENDPOINT_USER_ID_0 = 90,
  SL_CPC_ENDPOINT_USER_ID_1 = 91,
  SL_CPC_ENDPOINT_USER_ID_2 = 92,
  SL_CPC_ENDPOINT_USER_ID_3 = 93,
  SL_CPC_ENDPOINT_USER_ID_4 = 94,
  SL_CPC_ENDPOINT_USER_ID_5 = 95,
  SL_CPC_ENDPOINT_USER_ID_6 = 96,
  SL_CPC_ENDPOINT_USER_ID_7 = 97,
  SL_CPC_ENDPOINT_USER_ID_8 = 98,
  SL_CPC_ENDPOINT_USER_ID_9 = 99,
};

typedef struct {
  void *ptr;
} cpc_handle_t;

typedef struct {
  void *ptr;
} cpc_endpoint_t;

typedef uint8_t cpc_read_flags_t;
typedef uint8_t cpc_write_flags_t;

/***************************************************************************/ /**
 * Callback to notify the lib user that the secondary has crashed/reseted itself.
 *
 * @warning This callback is called in a signal context. The user must be
 *          careful of what is done in this callback.
 ******************************************************************************/
typedef void (*cpc_reset_callback_t) (void);

/***************************************************************************/ /**
 * Initialize the CPC library.
 * Upon success the user will obtain a handle that must be passed to cpc_open calls.
 * The library will use this handle to save information that are private to libcpc.
 *
 * @param[out] handle           CPC library handle
 * @param[in]  enable_tracing   Enable tracing over stdout
 *
 * @return Status code, on error, -1 is returned, and errno is set appropriately.
 ******************************************************************************/
int cpc_init(cpc_handle_t *handle, bool enable_tracing, cpc_reset_callback_t reset_callback);

/***************************************************************************/ /**
 * Restart the CPC library.
 * The user is notified via the 'reset_callback' when the daemon restarted.
 * The user logic then has to call this function in order to [try] to re-bind
 * the lib to the daemon.
 *
 * @param[out] handle           CPC library handle
 *
 * @return Status code, on error, -1 is returned, and errno is set appropriately.
 ******************************************************************************/
int cpc_restart(cpc_handle_t *handle);

/***************************************************************************/ /**
 * Connect to the socket corresponding to the provided endpoint ID.
 * The function will also allocate the memory for the endpoint structure and assign it to the provided pointer.
 * This endpoint structure must then be used for further calls to the libcpc.
 *
 * @param[in]  handle           CPC library handle
 * @param[out] endpoint         CPC endpoint handle to open
 * @param[in]  id               CPC endpoint id to open
 * @param[in]  tx_window_size   CPC transmit window (only a window of 1 is supported at the moment)
 *
 * @return Status code, on error, -1 is returned, and errno is set appropriately.
 *         On success, the file descriptor of the socket is returned.
 ******************************************************************************/
int cpc_open_endpoint(cpc_handle_t handle, cpc_endpoint_t *endpoint, uint8_t id, uint8_t tx_window_size);

/***************************************************************************/ /**
 * Close the socket connection to the endpoint.
 * This function will also free the memory used to allocate the endpoint structure.
 *
 * @param[in] endpoint         CPC endpoint handle to close
 *
 * @return Status code, on error, -1 is returned, and errno is set appropriately.
 ******************************************************************************/
int cpc_close_endpoint(cpc_endpoint_t *endpoint);

/***************************************************************************/ /**
 * Attempts to read up to count bytes from  from a previously opened endpoint socket.
 * Once data is received it will be copied to the user-provided buffer.
 * The lifecycle of this buffer is handled by the user.
 *
 * By default the cpc_read function will block indefinitely.
 * A timeout can be configured with cpc_set_option.
 *
 * @param[in] endpoint         CPC endpoint handle to read from
 * @param[out] buffer          The buffer to which the data will be copied to
 * @param[in] count            The number of bytes to copy to that buffer
 * @param[in] flags            Optional flags:
 *                             - SL_CPC_FLAG_NON_BLOCK: Set this transaction as non-blocking
 *
 * @return Status code, on error, -1 is returned, and errno is set appropriately.
 *                      on success, the function returns the amount of bytes that have been read.
 ******************************************************************************/
ssize_t cpc_read_endpoint(cpc_endpoint_t endpoint, void *buffer, size_t count, cpc_read_flags_t flags);

/***************************************************************************/ /**
 * Write data to a previously opened endpoint socket.
 * The user provides the data and the associated data length.
 *
 * @param[in] endpoint         CPC endpoint handle to read from
 * @param[in] data             The data to write on the CPC endpoint
 * @param[in] data_length      The length of the data to write on the CPC endpoint
 * @param[in] flags            Optional flags:
 *                             - SL_CPC_FLAG_NON_BLOCK: Set this transaction as non-blocking
 *
 * @return Status code, on error, -1 is returned, and errno is set appropriately.
 *                      on success, the function returns the amount of bytes that have been written.
 ******************************************************************************/
ssize_t cpc_write_endpoint(cpc_endpoint_t endpoint, const void *data, size_t data_length, cpc_write_flags_t flags);

/***************************************************************************/ /**
 * Obtain the state of an endpoint via the daemon control socket.
 *
 * @param[in]  handle          CPC library handle
 * @param[in]  id              The id from which to obtain the endpoint state
 * @param[out] state           The state of the provided CPC endpoint
 *
 * @return Status code, on error, -1 is returned, and errno is set appropriately.
 ******************************************************************************/
int cpc_get_endpoint_state(cpc_handle_t handle, uint8_t id, cpc_endpoint_state_t *state);

/***************************************************************************/ /**
 * Configure an endpoint with a specified option
 *
 * @param[in] endpoint         CPC endpoint handle to read from
 * @param[in] option           The option to configure
 * @param[in] optval           The value of the option
 * @param[in] optlen           The length of the value of the option
 *
 * @return Status code, on error, -1 is returned, and errno is set appropriately.
 *
 * @note Options are as follow:
 * CPC_OPTION_RX_TIMEOUT: Set a timeout for the read transaction, optval
 *                        must be a struct timeval (as specified in <sys/time.h>)
 *
 * CPC_OPTION_BLOCKING: Set every transactions (read or write) as blocking or not, optval
 *                      is a boolean.
 *
 * CPC_OPTION_SOCKET_SIZE: Set the buffer size for the socket used to write on an endpoint
 *                         optval is an integer. The kernel doubles this value (to allow space for
                           bookkeeping overhead).
 ******************************************************************************/
int cpc_set_endpoint_option(cpc_endpoint_t endpoint, cpc_option_t option, const void *optval, size_t optlen);

/***************************************************************************/ /**
 * Obtain the option configured for a specified endpoint
 *
 * @param[in] endpoint         CPC endpoint handle to read from
 * @param[in] option           The option to obtain
 * @param[out] optval          The value of the option
 * @param[out] optlen          The length of the value of the option optlen is a value-result argument,
 *                             initially containing the size of the buffer pointed to by optval,
 *                             and modified on return to indicate the actual size of the value returned.
 *
 * @return Status code, on error, -1 is returned, and errno is set appropriately.
 *
 * @note Options are as follow:
 * CPC_OPTION_RX_TIMEOUT: Set a timeout for the read transaction, optval
 *                        must be a struct timeval (as specified in <sys/time.h>)
 *
 * CPC_OPTION_BLOCKING: Set every transactions as blocking or not, optval
 *                      is a boolean.
 *
 * CPC_OPTION_SOCKET_SIZE: Get the buffer size for the socket used to write on an endpoint
 *                         optval is an integer.
 *
 * CPC_OPTION_MAX_WRITE_SIZE: Get the maximum size of the payload that will can be written
 *                            on an endpoint.
 ******************************************************************************/
int cpc_get_endpoint_option(cpc_endpoint_t endpoint, cpc_option_t option, void *optval, size_t *optlen);

#ifdef __cplusplus
}
#endif

#endif // SL_CPC_H