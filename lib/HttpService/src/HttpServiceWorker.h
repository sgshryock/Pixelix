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
 * @file   HttpServiceWorker.h
 * @brief  HTTP service worker
 * @author Andreas Merkle (web@blue-andi.de)
 *
 * @addtogroup HTTP_SERVICE
 *
 * @{
 */

#ifndef HTTP_SERVICE_WORKER_H
#define HTTP_SERVICE_WORKER_H

/******************************************************************************
 * Includes
 *****************************************************************************/
#include <Task.hpp>
#include "HttpServiceTypes.h"

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
 * The HTTP service worker handles the HTTP requests in the context of a task.
 */
class HttpServiceWorker : public Task<WorkerQueues>
{
public:

    /**
     * Stack size for HTTPS operations.
     * SSL/TLS requires significant stack space for handshake operations.
     */
    static const uint32_t HTTPS_STACK_SIZE = 16384U;

    /**
     * Constructs the HTTP service worker.
     */
    HttpServiceWorker() :
        Task<WorkerQueues>("HttpServiceWorkerTask", HTTPS_STACK_SIZE)
    {
        ;
    }

    /**
     * Destroys the HTTP service worker.
     */
    ~HttpServiceWorker()
    {
        /* Nothing to do. */
    }

private:

    /**
     * Copy constructor not allowed.
     *
     * @param[in] worker    Worker to copy.
     */
    HttpServiceWorker(const HttpServiceWorker& worker)            = delete;

    /**
     * Assignment operator not allowed.
     *
     * @param[in] worker    Worker to assign.
     *
     * @return Reference to this worker.
     */
    HttpServiceWorker& operator=(const HttpServiceWorker& worker) = delete;

    /**
     * Process the worker task.
     *
     * @param[in] queues    Worker queues used for communication.
     */
    void process(WorkerQueues* queues) final;

    /**
     * Perform the HTTP request.
     *
     * @param[in]  workerReq        HTTP request to perform.
     * @param[out] workerRsp        HTTP response.
     * @param[in]  abortJobQueue    Queue to store aborted job ids.
     * @param[in]  abortedJobQueue  Queue to signal aborted job ids.
     */
    void performHttpRequest(const WorkerRequest& workerReq, WorkerResponse& workerRsp, Queue<HttpJobId>& abortJobQueue, Queue<HttpJobId>& abortedJobQueue);

    /**
     * Handle the HTTP response.
     *
     * @param[in]  httpClient   HTTP client which holds the response.
     * @param[in]  handler      Optional response handler which will be called when the response is available.
     * @param[out] workerRsp    HTTP response.
     */
    void handleHttpResponse(HTTPClient& httpClient, IHttpResponseHandler* handler, WorkerResponse& workerRsp);

    /**
     * Check if the job id is aborted.
     *
     * @param[in] abortJobQueue Queue to store aborted job ids.
     * @param[in] jobId         Job id to check.
     *
     * @return If the job id is aborted, it will return true otherwise false.
     */
    bool isJobAborted(Queue<HttpJobId>& abortedJobQueue, HttpJobId jobId);

    /**
     * Signal that the job id is aborted.
     *
     * @param[in] abortedJobQueue   Queue to store aborted job ids.
     * @param[in] jobId             Job id to signal as aborted.
     */
    void signalJobAborted(Queue<HttpJobId>& abortedJobQueue, HttpJobId jobId);
};

/******************************************************************************
 * Variables
 *****************************************************************************/

/******************************************************************************
 * Functions
 *****************************************************************************/

#endif /* HTTP_SERVICE_WORKER_H */

/** @} */