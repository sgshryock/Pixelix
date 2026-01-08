/* MIT License
 *
 * Copyright (c) 2019 - 2025 Andreas Merkle <web@blue-andi.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*******************************************************************************
    DESCRIPTION
*******************************************************************************/
/**
 * @file   HttpServiceTypes.h
 * @brief  HTTP service types
 * @author Andreas Merkle (web@blue-andi.de)
 *
 * @addtogroup HTTP_SERVICE
 *
 * @{
 */

#ifndef HTTP_SERVICE_TYPES_H
#define HTTP_SERVICE_TYPES_H

/******************************************************************************
 * Includes
 *****************************************************************************/
#include <stdint.h>
#include <stddef.h>
#include <HTTPClient.h>
#include "IHttpResponseHandler.h"

/******************************************************************************
 * Compiler Switches
 *****************************************************************************/

/******************************************************************************
 * Macros
 *****************************************************************************/

/******************************************************************************
 * Types and Classes
 *****************************************************************************/

/**
 * Type definition for HTTP job id.
 */
typedef uint32_t HttpJobId;

/**
 * This type defines the HTTP methods.
 */
typedef enum
{
    HTTP_METHOD_GET, /**< HTTP GET method */
    HTTP_METHOD_POST /**< HTTP POST method */

} HttpMethod;

/**
 * This type defines a HTTP request structure.
 * This structure is used to send HTTP requests to the worker task.
 *
 * Attention, constructor and destructor of the members are not called!
 */
typedef struct
{
    HttpJobId             jobId;   /**< Job id of the HTTP request/response. */
    const char*           url;     /**< URL of the HTTP request. */
    HttpMethod            method;  /**< HTTP method of the request. */
    const uint8_t*        payload; /**< Payload of the HTTP request (only for POST). */
    size_t                size;    /**< Size of the payload in byte (only for POST). */
    IHttpResponseHandler* handler; /**< Optional response handler which will be called when the response is available. */

} WorkerRequest;

/**
 * This type defines a worker response structure.
 * This structure is used to send back the HTTP response from the worker task.
 *
 * Attention, constructor and destructor of the members are not called!
 */
typedef struct
{
    HttpJobId    jobId;      /**< Job id of the HTTP request/response. */
    t_http_codes statusCode; /**< HTTP status code of the response. */
    uint8_t*     payload;    /**< Payload of the HTTP response. */
    size_t       size;       /**< Size of the payload in byte. */
    size_t       capacity;   /**< Capacity of the payload buffer in byte (for efficient reallocation). */

} WorkerResponse;

/**
 * This type defines the worker queues, that are
 * task safe to be used from the worker task.
 */
typedef struct
{
    Queue<WorkerRequest>  requestQueue;    /**< Queue to store pending HTTP requests. */
    Queue<WorkerResponse> responseQueue;   /**< Queue to store received HTTP responses. */
    Queue<HttpJobId>      abortJobQueue;   /**< Queue to store job ids to abort. */
    Queue<HttpJobId>      abortedJobQueue; /**< Queue to store aborted job ids. */

} WorkerQueues;

/******************************************************************************
 * Variables
 *****************************************************************************/

/**
 * Invalid HTTP job id.
 */
static constexpr HttpJobId INVALID_HTTP_JOB_ID = 0U;

/******************************************************************************
 * Functions
 *****************************************************************************/

#endif /* HTTP_SERVICE_TYPES_H */

/** @} */