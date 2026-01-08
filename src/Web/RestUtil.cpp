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
 * @file   RestUtil.cpp
 * @brief  REST API Utilities
 * @author Andreas Merkle <web@blue-andi.de>
 */

/******************************************************************************
 * Includes
 *****************************************************************************/
#include "RestUtil.h"

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

/******************************************************************************
 * External Functions
 *****************************************************************************/

JsonVariant RestUtil::prepareRspSuccess(JsonDocument& jsonDoc)
{
    JsonObject dataObj = jsonDoc.createNestedObject("data");

    jsonDoc["status"]   = "ok";

    return dataObj;
}

void RestUtil::prepareRspError(JsonDocument& jsonDoc, const char* msg)
{
    JsonObject errorObj = jsonDoc.createNestedObject("error");

    errorObj["msg"]     = msg;
    jsonDoc["status"]   = "error";
}

void RestUtil::prepareRspErrorHttpMethodNotSupported(JsonDocument& jsonDoc)
{
    prepareRspError(jsonDoc, "HTTP method not supported.");
}

void RestUtil::sendJsonRsp(AsyncWebServerRequest* request, const JsonDocument& jsonDoc, uint32_t httpStatusCode)
{
    if (true == jsonDoc.overflowed())
    {
        LOG_ERROR("JSON document has less memory available.");
    }

    if (nullptr != request)
    {
        String                    content;
        AsyncWebServerResponse*   response;

        (void)serializeJsonPretty(jsonDoc, content);
        response = request->beginResponse(httpStatusCode, "application/json", content);

        if (nullptr != response)
        {
            /* Add security headers */
            response->addHeader("X-Content-Type-Options", "nosniff");
            response->addHeader("X-Frame-Options", "SAMEORIGIN");
            response->addHeader("X-XSS-Protection", "1; mode=block");

            /* Add CORS headers - restrict to same-origin by default */
            response->addHeader("Access-Control-Allow-Origin", "null");
            response->addHeader("Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS");
            response->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");

            request->send(response);
        }
    }
}

/******************************************************************************
 * Local Functions
 *****************************************************************************/
