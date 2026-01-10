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
 * @file   SessionAuthMiddleware.cpp
 * @brief  Session-based authentication middleware
 * @author Andreas Merkle <web@blue-andi.de>
 */

/******************************************************************************
 * Includes
 *****************************************************************************/
#include "SessionAuthMiddleware.h"
#include "SessionMgr.h"
#include <WebAuthentication.h>
#include <Logging.h>

/******************************************************************************
 * Compiler Switches
 *****************************************************************************/

/******************************************************************************
 * Macros
 *****************************************************************************/

/** HTTP Basic Auth realm */
#define AUTH_REALM "Pixelix"

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

SessionAuthMiddleware::SessionAuthMiddleware() :
    AsyncMiddleware(),
    m_username(),
    m_password()
{
}

SessionAuthMiddleware::~SessionAuthMiddleware()
{
}

void SessionAuthMiddleware::setUsername(const char* username)
{
    if (nullptr != username)
    {
        m_username = username;
    }
}

void SessionAuthMiddleware::setPassword(const char* password)
{
    if (nullptr != password)
    {
        m_password = password;
    }
}

void SessionAuthMiddleware::run(AsyncWebServerRequest* request, ArMiddlewareNext next)
{
    if (nullptr == request)
    {
        return;
    }

    /* Check 1: Valid session cookie? */
    if (true == hasValidSession(request))
    {
        /* Session is valid, proceed to handler */
        next();
        return;
    }

    /* Check 2: Valid Basic Auth credentials? */
    if (true == hasValidBasicAuth(request))
    {
        /* Create a new session and set cookie */
        SessionMgr& sessionMgr = SessionMgr::getInstance();
        String      token      = sessionMgr.createSession();
        String      cookieHeader;

        if (false == token.isEmpty())
        {
            cookieHeader = sessionMgr.generateSetCookieHeader(token);
        }

        /* Credentials valid, proceed to handler */
        next();

        /* After handler runs, try to add Set-Cookie to the response.
         * The response is queued but not yet sent, so we can still modify it.
         */
        if (false == cookieHeader.isEmpty())
        {
            AsyncWebServerResponse* response = request->getResponse();

            if (nullptr != response)
            {
                response->addHeader("Set-Cookie", cookieHeader);
            }
        }

        return;
    }

    /* Both checks failed, send 401 Unauthorized */
    sendUnauthorized(request);
}

/******************************************************************************
 * Protected Methods
 *****************************************************************************/

/******************************************************************************
 * Private Methods
 *****************************************************************************/

bool SessionAuthMiddleware::hasValidSession(AsyncWebServerRequest* request)
{
    bool isValid = false;

    if (true == request->hasHeader("Cookie"))
    {
        const AsyncWebHeader* cookieHeader = request->getHeader("Cookie");

        if (nullptr != cookieHeader)
        {
            SessionMgr& sessionMgr = SessionMgr::getInstance();
            String      token      = sessionMgr.parseSessionFromCookie(cookieHeader->value());

            if (false == token.isEmpty())
            {
                isValid = sessionMgr.isValidSession(token);
            }
        }
    }

    return isValid;
}

bool SessionAuthMiddleware::hasValidBasicAuth(AsyncWebServerRequest* request)
{
    bool isValid = false;

    if (true == request->hasHeader("Authorization"))
    {
        const AsyncWebHeader* authHeader = request->getHeader("Authorization");

        if (nullptr != authHeader)
        {
            String authValue = authHeader->value();

            /* Check if it's Basic Auth */
            if (true == authValue.startsWith("Basic "))
            {
                /* Extract the base64-encoded credentials part */
                String credentials = authValue.substring(6); /* Skip "Basic " */

                isValid = checkBasicAuthentication(
                    credentials.c_str(),
                    m_username.c_str(),
                    m_password.c_str()
                );
            }
        }
    }

    return isValid;
}

void SessionAuthMiddleware::sendUnauthorized(AsyncWebServerRequest* request)
{
    AsyncWebServerResponse* response = request->beginResponse(401, "text/plain", "Unauthorized");

    if (nullptr != response)
    {
        response->addHeader("WWW-Authenticate", "Basic realm=\"" AUTH_REALM "\"");
        request->send(response);
    }
}

/******************************************************************************
 * External Functions
 *****************************************************************************/

/******************************************************************************
 * Local Functions
 *****************************************************************************/
