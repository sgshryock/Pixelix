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
 * @file   SessionMgr.h
 * @brief  Web session manager for cookie-based authentication
 * @author Andreas Merkle <web@blue-andi.de>
 *
 * @addtogroup WEB
 *
 * @{
 */

#ifndef SESSION_MGR_H
#define SESSION_MGR_H

/******************************************************************************
 * Compile Switches
 *****************************************************************************/

/******************************************************************************
 * Includes
 *****************************************************************************/
#include <WString.h>
#include <vector>
#include <stdint.h>

/******************************************************************************
 * Macros
 *****************************************************************************/

/** Maximum number of concurrent sessions */
#define SESSION_MGR_MAX_SESSIONS    (8U)

/** Cookie name for session ID */
#define SESSION_MGR_COOKIE_NAME     "session"

/******************************************************************************
 * Types and Classes
 *****************************************************************************/

/**
 * Session manager for cookie-based authentication.
 * Manages session tokens in memory.
 */
class SessionMgr
{
public:
    /**
     * Get the session manager instance.
     *
     * @return Session manager instance
     */
    static SessionMgr& getInstance()
    {
        static SessionMgr instance;
        return instance;
    }

    /**
     * Create a new session and return the session token.
     *
     * @return Session token (MD5 hash) or empty string on failure
     */
    String createSession();

    /**
     * Check if a session token is valid.
     *
     * @param[in] token Session token to validate
     *
     * @return true if valid, false otherwise
     */
    bool isValidSession(const String& token);

    /**
     * Invalidate a session (logout).
     *
     * @param[in] token Session token to invalidate
     */
    void invalidateSession(const String& token);

    /**
     * Parse session token from Cookie header value.
     *
     * @param[in] cookieHeader  Cookie header value (e.g., "session=abc123; other=value")
     *
     * @return Session token or empty string if not found
     */
    String parseSessionFromCookie(const String& cookieHeader);

    /**
     * Generate Set-Cookie header value for a session token.
     *
     * @param[in] token Session token
     *
     * @return Set-Cookie header value
     */
    String generateSetCookieHeader(const String& token);

    /**
     * Generate Set-Cookie header to clear the session cookie.
     *
     * @return Set-Cookie header value that clears the cookie
     */
    String generateClearCookieHeader();

private:
    /** Session data structure */
    struct Session
    {
        String   token;     /**< Session token (MD5 hash) */
        uint32_t createdAt; /**< Timestamp when session was created (millis) */
    };

    std::vector<Session> m_sessions; /**< List of active sessions */

    /**
     * Default constructor.
     */
    SessionMgr()
    {
    }

    /**
     * Destructor.
     */
    ~SessionMgr()
    {
    }

    /* Not copyable */
    SessionMgr(const SessionMgr&) = delete;
    SessionMgr& operator=(const SessionMgr&) = delete;
};

/******************************************************************************
 * Functions
 *****************************************************************************/

#endif /* SESSION_MGR_H */

/** @} */
