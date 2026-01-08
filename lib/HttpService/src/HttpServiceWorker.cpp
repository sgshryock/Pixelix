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
 * @file   HttpServiceWorker.cpp
 * @brief  HTTP service worker
 * @author Andreas Merkle (web@blue-andi.de)
 */

/******************************************************************************
 * Includes
 *****************************************************************************/
#include "HttpServiceWorker.h"
#include <Logging.h>

/******************************************************************************
 * Compiler Switches
 *****************************************************************************/

/******************************************************************************
 * Macros
 *****************************************************************************/

/******************************************************************************
 * Types and classes
 *****************************************************************************/

/******************************************************************************
 * Prototypes
 *****************************************************************************/

/******************************************************************************
 * Local Variables
 *****************************************************************************/

/******************************************************************************
 * Public Methods
 *****************************************************************************/

/******************************************************************************
 * Protected Methods
 *****************************************************************************/

/******************************************************************************
 * Private Methods
 *****************************************************************************/

void HttpServiceWorker::process(WorkerQueues* queues)
{
    if (nullptr != queues)
    {
        const TickType_t MAX_WAIT_TIME = pdMS_TO_TICKS(100U);
        WorkerRequest    request;

        /* Wait for a new HTTP request. */
        if (true == queues->requestQueue.receive(&request, MAX_WAIT_TIME))
        {
            WorkerResponse workerRsp = {
                INVALID_HTTP_JOB_ID,    /* jobId */
                HTTP_CODE_OK,           /* statusCode */
                nullptr,                /* payload */
                0U,                     /* size */
                0U                      /* capacity */
            };

            /* Check if the job is aborted. */
            if (true == isJobAborted(queues->abortJobQueue, request.jobId))
            {
                LOG_INFO("HTTP job id %u is aborted before performing the request.", request.jobId);
                signalJobAborted(queues->abortedJobQueue, request.jobId);
            }
            else
            {
                /* Perform the HTTP request. */
                performHttpRequest(request, workerRsp, queues->abortJobQueue, queues->abortedJobQueue);

                /* Check if the job is aborted meanwhile. */
                if (true == isJobAborted(queues->abortJobQueue, request.jobId))
                {
                    LOG_INFO("HTTP job id %u is aborted after performing the request.", request.jobId);
                    signalJobAborted(queues->abortedJobQueue, request.jobId);

                    /* Clean-up */
                    if (nullptr != workerRsp.payload)
                    {
                        delete[] workerRsp.payload;

                        workerRsp.payload  = nullptr;
                        workerRsp.size     = 0U;
                        workerRsp.capacity = 0U;
                    }
                }
                else
                {
                    /* Send back the HTTP response. */
                    (void)queues->responseQueue.sendToBack(workerRsp, MAX_WAIT_TIME);
                }
            }
        }
    }
}

void HttpServiceWorker::performHttpRequest(const WorkerRequest& workerReq, WorkerResponse& workerRsp, Queue<HttpJobId>& abortJobQueue, Queue<HttpJobId>& abortedJobQueue)
{
    const char*  PREFIX_HTTPS        = "https://";
    const size_t PREFIX_HTTPS_LENGTH = strlen(PREFIX_HTTPS);

    WiFiClient*  wifiClient          = nullptr;

    /* HTTP over TLS required? */
    if (0 == strncmp(workerReq.url, PREFIX_HTTPS, PREFIX_HTTPS_LENGTH))
    {
        WiFiClientSecure* secureClient = new WiFiClientSecure();

        if (nullptr != secureClient)
        {
            wifiClient = secureClient;
            secureClient->setInsecure();
        }
    }
    /* HTTP over plain TCP. */
    else
    {
        wifiClient = new WiFiClient();
    }

    if (nullptr == wifiClient)
    {
        LOG_WARNING("HTTP request to URL %s failed, no heap memory available.", workerReq.url);
        workerRsp.statusCode = HTTP_CODE_INTERNAL_SERVER_ERROR;
    }
    else
    {
        HTTPClient httpClient;

        if (false == httpClient.begin(*wifiClient, workerReq.url))
        {
            LOG_WARNING("HTTP request to URL %s failed, unable to connect.", workerReq.url);
            workerRsp.statusCode = HTTP_CODE_SERVICE_UNAVAILABLE;
        }
        else
        {
            int httpClientRet = 0;

            switch (workerReq.method)
            {
            case HTTP_METHOD_GET:
                httpClientRet = httpClient.GET();
                break;

            case HTTP_METHOD_POST:
                httpClientRet = httpClient.POST(const_cast<uint8_t*>(workerReq.payload), workerReq.size);
                break;

            default:
                LOG_WARNING("HTTP request to URL %s failed, unsupported HTTP method %d.", workerReq.url, workerReq.method);
                workerRsp.statusCode = HTTP_CODE_NOT_IMPLEMENTED;
                break;
            }

            if (0 > httpClientRet)
            {
                LOG_WARNING("HTTP request to URL %s failed, error: %s", workerReq.url, httpClient.errorToString(httpClientRet).c_str());
                workerRsp.statusCode = HTTP_CODE_BAD_REQUEST;
            }
            else
            {
                workerRsp.statusCode = static_cast<t_http_codes>(httpClientRet);

                if (HTTP_CODE_OK == workerRsp.statusCode)
                {
                    if (true == isJobAborted(abortJobQueue, workerReq.jobId))
                    {
                        LOG_INFO("HTTP job id %u is aborted after performing the request.", workerReq.jobId);
                        signalJobAborted(abortedJobQueue, workerReq.jobId);
                    }
                    else
                    {
                        handleHttpResponse(httpClient, workerReq.handler, workerRsp);
                    }
                }
            }

            httpClient.end();
        }
    }
}

void HttpServiceWorker::handleHttpResponse(HTTPClient& httpClient, IHttpResponseHandler* handler, WorkerResponse& workerRsp)
{
    int32_t     contentLength = httpClient.getSize(); /* Get size of the payload. If no Content-Length header present, size will be -1. */
    uint8_t     buffer[1024U];
    WiFiClient& stream = httpClient.getStream();
    uint32_t    index  = 0U;

    while ((true == httpClient.connected()) && ((0 < contentLength) || (-1 == contentLength)))
    {
        int32_t toRead = stream.available();

        if (0 < toRead)
        {
            char*   cBuffer = static_cast<char*>(static_cast<void*>(buffer));
            int32_t read    = stream.readBytes(cBuffer, (toRead < sizeof(buffer)) ? toRead : sizeof(buffer));

            if (0 < read)
            {
                if (0 < contentLength)
                {
                    if (read > contentLength)
                    {
                        contentLength = 0;
                    }
                    else
                    {
                        contentLength -= read;
                    }
                }

                ++index;

                /* If a response handler is provided, call it to process the received payload chunk. */
                if (nullptr != handler)
                {
                    bool isFinal = (0 == contentLength);

                    handler->onResponse(index, isFinal, buffer, static_cast<size_t>(read));
                }
                /* Store payload internally with exponential buffer growth to reduce heap fragmentation. */
                else
                {
                    const size_t INITIAL_BUFFER_SIZE = 1024U;
                    size_t       requiredSize        = workerRsp.size + static_cast<size_t>(read);

                    /* Need to allocate or grow buffer? */
                    if (requiredSize > workerRsp.capacity)
                    {
                        /* Calculate new capacity using exponential growth (double each time). */
                        size_t newCapacity = (0U == workerRsp.capacity) ? INITIAL_BUFFER_SIZE : workerRsp.capacity * 2U;

                        /* Ensure capacity is sufficient for required size. */
                        while (newCapacity < requiredSize)
                        {
                            newCapacity *= 2U;
                        }

                        uint8_t* newPayload = new (std::nothrow) uint8_t[newCapacity];

                        if (nullptr != newPayload)
                        {
                            /* Copy existing data if any. */
                            if ((nullptr != workerRsp.payload) && (0U < workerRsp.size))
                            {
                                memcpy(newPayload, workerRsp.payload, workerRsp.size);
                            }

                            delete[] workerRsp.payload;
                            workerRsp.payload  = newPayload;
                            workerRsp.capacity = newCapacity;
                        }
                    }

                    /* Append new data if buffer is available. */
                    if ((nullptr != workerRsp.payload) && (requiredSize <= workerRsp.capacity))
                    {
                        memcpy(workerRsp.payload + workerRsp.size, buffer, read);
                        workerRsp.size = requiredSize;
                    }
                }
            }
        }

        /* Give other tasks a chance to run. */
        delay(1U);
    }
}

bool HttpServiceWorker::isJobAborted(Queue<HttpJobId>& abortedJobQueue, HttpJobId jobId)
{
    bool      isAborted    = false;
    HttpJobId jobIdToAbort = INVALID_HTTP_JOB_ID;

    if (true == abortedJobQueue.receive(&jobIdToAbort, 0U))
    {
        if (jobId == jobIdToAbort)
        {
            isAborted = true;
        }
    }

    return isAborted;
}

void HttpServiceWorker::signalJobAborted(Queue<HttpJobId>& abortedJobQueue, HttpJobId jobId)
{
    (void)abortedJobQueue.sendToBack(jobId, portMAX_DELAY);
}

/******************************************************************************
 * External Functions
 *****************************************************************************/

/******************************************************************************
 * Local Functions
 *****************************************************************************/
