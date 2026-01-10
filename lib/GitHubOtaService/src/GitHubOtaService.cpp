/* MIT License
 *
 * Copyright (c) 2024 - 2025 Shryock Edition
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
 * @file   GitHubOtaService.cpp
 * @brief  GitHub OTA update service implementation
 * @author Shryock Edition
 */

/******************************************************************************
 * Includes
 *****************************************************************************/
#include "GitHubOtaService.h"
#include <Version.h>
#include <Logging.h>
#include <RestService.h>
#include <ArduinoJson.h>
#include <FileSystem.h>
#include <esp_partition.h>

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

/** GitHub API base URL */
static const char* GITHUB_API_BASE = "https://api.github.com";

/******************************************************************************
 * Public Methods
 *****************************************************************************/

bool GitHubOtaService::start()
{
    /* Nothing to start - service is ready immediately */
    LOG_INFO("GitHubOtaService started.");
    return true;
}

void GitHubOtaService::stop()
{
    /* Abort any ongoing operation */
    abort();
    LOG_INFO("GitHubOtaService stopped.");
}

void GitHubOtaService::process()
{
    MutexGuard<Mutex> guard(m_mutex);

    if (RestService::INVALID_REST_ID != m_restId)
    {
        bool                isValidRsp = false;
        DynamicJsonDocument jsonDoc(16384U);

        if (true == RestService::getInstance().getResponse(m_restId, isValidRsp, jsonDoc))
        {
            m_restId = RestService::INVALID_REST_ID;

            if (OtaState::CHECKING_RELEASE == m_state)
            {
                if (true == isValidRsp)
                {
                    if (true == parseReleaseJson(jsonDoc))
                    {
                        m_state = OtaState::RELEASE_INFO_READY;
                        LOG_INFO("Release found: %s", m_releaseInfo.tagName.c_str());
                    }
                    else
                    {
                        setError("Failed to parse release JSON");
                    }
                }
                else
                {
                    setError("Failed to fetch release info");
                }
            }
            /* Note: Firmware/filesystem downloads are not supported via RestService
             * as it expects JSON responses. Binary downloads would need a different approach.
             */
        }
    }
}

bool GitHubOtaService::checkForUpdates(const String& repoUrl)
{
    MutexGuard<Mutex> guard(m_mutex);

    /* GitHub OTA is currently work-in-progress.
     * The GitHub API requires HTTPS, but AsyncHttpClient's SSL support
     * has memory/stability issues on ESP32. This feature is disabled
     * until HTTPS support is improved.
     */
    (void)repoUrl;
    setError("GitHub OTA updates coming soon - feature in development");
    return false;
}

bool GitHubOtaService::startOtaUpdate()
{
    MutexGuard<Mutex> guard(m_mutex);

    if (OtaState::RELEASE_INFO_READY != m_state)
    {
        return false;
    }

    if (m_releaseInfo.downloadUrl.isEmpty())
    {
        setError("No download URL available");
        return false;
    }

    if (false == m_releaseInfo.hasMatchingBinary)
    {
        setError("No matching binary for this target");
        return false;
    }

    /* TODO: Binary downloads not yet implemented.
     * RestService only supports JSON responses.
     * Need to implement direct AsyncHttpClient usage for binary downloads.
     */
    setError("OTA download not yet implemented");
    return false;
}

void GitHubOtaService::abort()
{
    MutexGuard<Mutex> guard(m_mutex);

    if (0 != m_otaHandle)
    {
        esp_ota_abort(m_otaHandle);
        m_otaHandle = 0;
    }

    if (RestService::INVALID_REST_ID != m_restId)
    {
        RestService::getInstance().abortRequest(m_restId);
        m_restId = RestService::INVALID_REST_ID;
    }

    m_state = OtaState::IDLE;
    m_errorMessage.clear();
    m_releaseInfo.clear();
    m_bytesWritten = 0;
    m_downloadProgress = 0;
    m_fsBytesWritten = 0;
    m_fsDownloadProgress = 0;

    LOG_INFO("OTA aborted");
}

OtaState GitHubOtaService::getState() const
{
    MutexGuard<Mutex> guard(m_mutex);
    return m_state;
}

const String& GitHubOtaService::getErrorMessage() const
{
    MutexGuard<Mutex> guard(m_mutex);
    return m_errorMessage;
}

const ReleaseInfo& GitHubOtaService::getReleaseInfo() const
{
    MutexGuard<Mutex> guard(m_mutex);
    return m_releaseInfo;
}

uint8_t GitHubOtaService::getDownloadProgress() const
{
    MutexGuard<Mutex> guard(m_mutex);
    return m_downloadProgress;
}

uint8_t GitHubOtaService::getFilesystemProgress() const
{
    MutexGuard<Mutex> guard(m_mutex);
    return m_fsDownloadProgress;
}

/******************************************************************************
 * Protected Methods
 *****************************************************************************/

/******************************************************************************
 * Private Methods
 *****************************************************************************/

GitHubOtaService::GitHubOtaService() :
    m_mutex(),
    m_state(OtaState::IDLE),
    m_errorMessage(),
    m_releaseInfo(),
    m_repoUrl(),
    m_restId(RestService::INVALID_REST_ID),
    m_otaHandle(0),
    m_updatePartition(nullptr),
    m_bytesWritten(0),
    m_downloadProgress(0),
    m_spiffsPartition(nullptr),
    m_fsBytesWritten(0),
    m_fsDownloadProgress(0)
{
}

GitHubOtaService::~GitHubOtaService()
{
    if (0 != m_otaHandle)
    {
        esp_ota_abort(m_otaHandle);
    }
}

String GitHubOtaService::buildApiUrl(const String& repoUrl) const
{
    /* Parse GitHub URL: https://github.com/owner/repo */
    String url = repoUrl;

    /* Remove trailing slash if present */
    if (url.endsWith("/"))
    {
        url = url.substring(0, url.length() - 1);
    }

    /* Extract owner and repo from URL */
    int githubIdx = url.indexOf("github.com/");
    if (-1 == githubIdx)
    {
        return String();
    }

    String path = url.substring(githubIdx + 11); /* Skip "github.com/" */

    /* Should be "owner/repo" */
    int slashIdx = path.indexOf('/');
    if (-1 == slashIdx)
    {
        return String();
    }

    String owner = path.substring(0, slashIdx);
    String repo = path.substring(slashIdx + 1);

    /* Remove .git suffix if present */
    if (repo.endsWith(".git"))
    {
        repo = repo.substring(0, repo.length() - 4);
    }

    /* Build API URL */
    String apiUrl = GITHUB_API_BASE;
    apiUrl += "/repos/";
    apiUrl += owner;
    apiUrl += "/";
    apiUrl += repo;
    apiUrl += "/releases/latest";

    return apiUrl;
}

String GitHubOtaService::getExpectedBinaryName() const
{
    String binaryName = "firmware_";
    binaryName += Version::getTargetName();
    binaryName += ".bin";
    return binaryName;
}

String GitHubOtaService::getExpectedFilesystemName() const
{
    String fsName = "littlefs_";
    fsName += Version::getTargetName();
    fsName += ".bin";
    return fsName;
}

bool GitHubOtaService::parseReleaseJson(const DynamicJsonDocument& jsonDoc)
{
    /* Extract tag_name (version) */
    JsonVariantConst tagName = jsonDoc["tag_name"];
    if (tagName.isNull())
    {
        LOG_ERROR("No tag_name in release");
        return false;
    }
    m_releaseInfo.tagName = tagName.as<const char*>();

    /* Extract body (release notes) - truncate if too long */
    JsonVariantConst body = jsonDoc["body"];
    if (!body.isNull())
    {
        String notes = body.as<const char*>();
        if (notes.length() > MAX_RELEASE_NOTES_LEN)
        {
            notes = notes.substring(0, MAX_RELEASE_NOTES_LEN - 3) + "...";
        }
        m_releaseInfo.releaseNotes = notes;
    }

    /* Find matching binaries in assets */
    String expectedFirmware = getExpectedBinaryName();
    String expectedFilesystem = getExpectedFilesystemName();
    m_releaseInfo.hasMatchingBinary = false;
    m_releaseInfo.hasFilesystem = false;

    LOG_INFO("Looking for firmware: %s", expectedFirmware.c_str());
    LOG_INFO("Looking for filesystem: %s", expectedFilesystem.c_str());

    JsonArrayConst assets = jsonDoc["assets"];
    for (JsonVariantConst asset : assets)
    {
        String assetName = asset["name"].as<const char*>();
        LOG_INFO("Found asset: %s", assetName.c_str());

        if (assetName == expectedFirmware)
        {
            m_releaseInfo.downloadUrl = asset["browser_download_url"].as<const char*>();
            m_releaseInfo.firmwareSize = asset["size"].as<size_t>();
            m_releaseInfo.hasMatchingBinary = true;
            LOG_INFO("Matched firmware! Size: %u", m_releaseInfo.firmwareSize);
        }
        else if (assetName == expectedFilesystem)
        {
            m_releaseInfo.filesystemUrl = asset["browser_download_url"].as<const char*>();
            m_releaseInfo.filesystemSize = asset["size"].as<size_t>();
            m_releaseInfo.hasFilesystem = true;
            LOG_INFO("Matched filesystem! Size: %u", m_releaseInfo.filesystemSize);
        }
    }

    return true;
}

void GitHubOtaService::handleFirmwareChunk(const uint8_t* data, size_t size, bool isFinal)
{
    /* TODO: Not implemented - binary downloads need AsyncHttpClient */
    (void)data;
    (void)size;
    (void)isFinal;
}

bool GitHubOtaService::startFilesystemDownload()
{
    /* TODO: Not implemented - binary downloads need AsyncHttpClient */
    setError("Filesystem download not yet implemented");
    return false;
}

void GitHubOtaService::handleFilesystemChunk(const uint8_t* data, size_t size, bool isFinal)
{
    /* TODO: Not implemented - binary downloads need AsyncHttpClient */
    (void)data;
    (void)size;
    (void)isFinal;
}

void GitHubOtaService::finalizeUpdate()
{
    /* TODO: Not implemented - binary downloads need AsyncHttpClient */
}

void GitHubOtaService::setError(const String& message)
{
    m_state = OtaState::ERROR;
    m_errorMessage = message;
    LOG_ERROR("OTA error: %s", message.c_str());
}

/******************************************************************************
 * External Functions
 *****************************************************************************/

/******************************************************************************
 * Local Functions
 *****************************************************************************/
