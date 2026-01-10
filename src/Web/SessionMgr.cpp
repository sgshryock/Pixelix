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
 * @file   SessionMgr.cpp
 * @brief  Web session manager for cookie-based authentication
 * @author Andreas Merkle <web@blue-andi.de>
 */

/******************************************************************************
 * Includes
 *****************************************************************************/
#include "SessionMgr.h"
#include <WebAuthentication.h>
#include <Arduino.h>

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

String SessionMgr::createSession()
{
    String token;

    /* Remove oldest session if we've reached the maximum */
    if (m_sessions.size() >= SESSION_MGR_MAX_SESSIONS)
    {
        m_sessions.erase(m_sessions.begin());
    }

    /* Generate a random MD5 token */
    token = genRandomMD5();

    if (false == token.isEmpty())
    {
        Session newSession;
        newSession.token     = token;
        newSession.createdAt = millis();

        m_sessions.push_back(newSession);
    }

    return token;
}

bool SessionMgr::isValidSession(const String& token)
{
    bool isValid = false;

    if (false == token.isEmpty())
    {
        for (const Session& session : m_sessions)
        {
            if (session.token == token)
            {
                isValid = true;
                break;
            }
        }
    }

    return isValid;
}

void SessionMgr::invalidateSession(const String& token)
{
    for (auto it = m_sessions.begin(); it != m_sessions.end(); ++it)
    {
        if (it->token == token)
        {
            m_sessions.erase(it);
            break;
        }
    }
}

String SessionMgr::parseSessionFromCookie(const String& cookieHeader)
{
    String token;
    String cookieName = String(SESSION_MGR_COOKIE_NAME) + "=";
    int    startIdx   = cookieHeader.indexOf(cookieName);

    if (startIdx >= 0)
    {
        startIdx += cookieName.length();
        int endIdx = cookieHeader.indexOf(';', startIdx);

        if (endIdx < 0)
        {
            /* No semicolon found, token goes to end of string */
            token = cookieHeader.substring(startIdx);
        }
        else
        {
            token = cookieHeader.substring(startIdx, endIdx);
        }

        token.trim();
    }

    return token;
}

String SessionMgr::generateSetCookieHeader(const String& token)
{
    /* Session cookie (no Max-Age) - expires when browser closes */
    /* HttpOnly prevents XSS, SameSite=Strict prevents CSRF */
    return String(SESSION_MGR_COOKIE_NAME) + "=" + token + "; Path=/; HttpOnly; SameSite=Strict";
}

String SessionMgr::generateClearCookieHeader()
{
    /* Set Max-Age=0 to delete the cookie */
    return String(SESSION_MGR_COOKIE_NAME) + "=; Path=/; HttpOnly; SameSite=Strict; Max-Age=0";
}

/******************************************************************************
 * Protected Methods
 *****************************************************************************/

/******************************************************************************
 * Private Methods
 *****************************************************************************/

/******************************************************************************
 * External Functions
 *****************************************************************************/

/******************************************************************************
 * Local Functions
 *****************************************************************************/
