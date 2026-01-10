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
 * @file   SessionAuthMiddleware.h
 * @brief  Session-based authentication middleware
 * @author Andreas Merkle <web@blue-andi.de>
 *
 * @addtogroup WEB
 *
 * @{
 */

#ifndef SESSION_AUTH_MIDDLEWARE_H
#define SESSION_AUTH_MIDDLEWARE_H

/******************************************************************************
 * Compile Switches
 *****************************************************************************/

/******************************************************************************
 * Includes
 *****************************************************************************/
#include <ESPAsyncWebServer.h>
#include <WString.h>

/******************************************************************************
 * Macros
 *****************************************************************************/

/******************************************************************************
 * Types and Classes
 *****************************************************************************/

/**
 * Authentication middleware that supports both session cookies and HTTP Basic Auth.
 *
 * Authentication flow:
 * 1. Check for valid session cookie -> allow request
 * 2. Check for valid Basic Auth credentials -> create session, set cookie, allow request
 * 3. If both fail -> return 401 Unauthorized
 */
class SessionAuthMiddleware : public AsyncMiddleware
{
public:
    /**
     * Constructor.
     */
    SessionAuthMiddleware();

    /**
     * Destructor.
     */
    ~SessionAuthMiddleware();

    /**
     * Set the username for Basic Auth.
     *
     * @param[in] username  Username
     */
    void setUsername(const char* username);

    /**
     * Set the password for Basic Auth.
     *
     * @param[in] password  Password
     */
    void setPassword(const char* password);

    /**
     * Middleware run function. Called for each request.
     *
     * @param[in] request   HTTP request
     * @param[in] next      Next middleware in chain
     */
    void run(AsyncWebServerRequest* request, ArMiddlewareNext next) override;

private:
    String m_username; /**< Username for Basic Auth */
    String m_password; /**< Password for Basic Auth */

    /**
     * Check if the request has a valid session cookie.
     *
     * @param[in] request   HTTP request
     *
     * @return true if valid session, false otherwise
     */
    bool hasValidSession(AsyncWebServerRequest* request);

    /**
     * Check if the request has valid Basic Auth credentials.
     *
     * @param[in] request   HTTP request
     *
     * @return true if valid credentials, false otherwise
     */
    bool hasValidBasicAuth(AsyncWebServerRequest* request);

    /**
     * Send 401 Unauthorized response with WWW-Authenticate header.
     *
     * @param[in] request   HTTP request
     */
    void sendUnauthorized(AsyncWebServerRequest* request);
};

/******************************************************************************
 * Functions
 *****************************************************************************/

#endif /* SESSION_AUTH_MIDDLEWARE_H */

/** @} */
