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
#include <HttpService.h>
#include <ArduinoJson.h>

/******************************************************************************
 * Compiler Switches
 *****************************************************************************/

/******************************************************************************
 * Macros
 *****************************************************************************/

/** Invalid HTTP job ID */
#define INVALID_HTTP_JOB_ID 0U

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

    if (INVALID_HTTP_JOB_ID != m_httpJobId)
    {
        HttpRsp response;

        if (true == HttpService::getInstance().getResponse(m_httpJobId, response))
        {
            m_httpJobId = INVALID_HTTP_JOB_ID;

            if (OtaState::CHECKING_RELEASE == m_state)
            {
                if ((200 == response.statusCode) && (nullptr != response.payload) && (0 < response.size))
                {
                    if (true == parseReleaseJson(response.payload, response.size))
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
                    setError("HTTP error: " + String(response.statusCode));
                }
            }
            else if (OtaState::DOWNLOADING == m_state)
            {
                if ((200 == response.statusCode) && (nullptr != response.payload) && (0 < response.size))
                {
                    handleFirmwareChunk(response.payload, response.size, true);
                }
                else
                {
                    if (0 != m_otaHandle)
                    {
                        esp_ota_abort(m_otaHandle);
                        m_otaHandle = 0;
                    }
                    setError("Download failed: " + String(response.statusCode));
                }
            }
        }
    }
}

bool GitHubOtaService::checkForUpdates(const String& repoUrl)
{
    MutexGuard<Mutex> guard(m_mutex);

    if ((OtaState::IDLE != m_state) &&
        (OtaState::ERROR != m_state) &&
        (OtaState::RELEASE_INFO_READY != m_state))
    {
        return false;
    }

    m_repoUrl = repoUrl;
    m_errorMessage.clear();
    m_releaseInfo.clear();

    String apiUrl = buildApiUrl(repoUrl);
    if (apiUrl.isEmpty())
    {
        setError("Invalid repository URL");
        return false;
    }

    LOG_INFO("Checking for updates: %s", apiUrl.c_str());

    m_httpJobId = HttpService::getInstance().get(apiUrl.c_str());

    if (INVALID_HTTP_JOB_ID == m_httpJobId)
    {
        setError("Failed to start HTTP request");
        return false;
    }

    m_state = OtaState::CHECKING_RELEASE;
    return true;
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

    /* Find the OTA update partition */
    m_updatePartition = esp_ota_get_next_update_partition(nullptr);
    if (nullptr == m_updatePartition)
    {
        setError("No OTA partition found");
        return false;
    }

    /* Begin OTA update */
    esp_err_t err = esp_ota_begin(m_updatePartition, OTA_SIZE_UNKNOWN, &m_otaHandle);
    if (ESP_OK != err)
    {
        setError("Failed to begin OTA: " + String(esp_err_to_name(err)));
        return false;
    }

    m_bytesWritten = 0;
    m_downloadProgress = 0;

    LOG_INFO("Starting firmware download: %s", m_releaseInfo.downloadUrl.c_str());

    m_httpJobId = HttpService::getInstance().get(m_releaseInfo.downloadUrl.c_str());

    if (INVALID_HTTP_JOB_ID == m_httpJobId)
    {
        esp_ota_abort(m_otaHandle);
        m_otaHandle = 0;
        setError("Failed to start firmware download");
        return false;
    }

    m_state = OtaState::DOWNLOADING;
    return true;
}

void GitHubOtaService::abort()
{
    MutexGuard<Mutex> guard(m_mutex);

    if (0 != m_otaHandle)
    {
        esp_ota_abort(m_otaHandle);
        m_otaHandle = 0;
    }

    m_httpJobId = INVALID_HTTP_JOB_ID;
    m_state = OtaState::IDLE;
    m_errorMessage.clear();
    m_releaseInfo.clear();
    m_bytesWritten = 0;
    m_downloadProgress = 0;

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
    m_httpJobId(INVALID_HTTP_JOB_ID),
    m_otaHandle(0),
    m_updatePartition(nullptr),
    m_bytesWritten(0),
    m_downloadProgress(0)
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

bool GitHubOtaService::parseReleaseJson(const uint8_t* data, size_t size)
{
    const size_t JSON_DOC_SIZE = 16384U;
    DynamicJsonDocument jsonDoc(JSON_DOC_SIZE);

    DeserializationError error = deserializeJson(jsonDoc, data, size);
    if (DeserializationError::Ok != error.code())
    {
        LOG_ERROR("JSON parse error: %s", error.c_str());
        return false;
    }

    /* Extract tag_name (version) */
    JsonVariant tagName = jsonDoc["tag_name"];
    if (tagName.isNull())
    {
        LOG_ERROR("No tag_name in release");
        return false;
    }
    m_releaseInfo.tagName = tagName.as<const char*>();

    /* Extract body (release notes) - truncate if too long */
    JsonVariant body = jsonDoc["body"];
    if (!body.isNull())
    {
        String notes = body.as<const char*>();
        if (notes.length() > MAX_RELEASE_NOTES_LEN)
        {
            notes = notes.substring(0, MAX_RELEASE_NOTES_LEN - 3) + "...";
        }
        m_releaseInfo.releaseNotes = notes;
    }

    /* Find matching binary in assets */
    String expectedBinary = getExpectedBinaryName();
    m_releaseInfo.hasMatchingBinary = false;

    LOG_INFO("Looking for binary: %s", expectedBinary.c_str());

    JsonArray assets = jsonDoc["assets"];
    for (JsonVariant asset : assets)
    {
        String assetName = asset["name"].as<const char*>();
        LOG_INFO("Found asset: %s", assetName.c_str());

        if (assetName == expectedBinary)
        {
            m_releaseInfo.downloadUrl = asset["browser_download_url"].as<const char*>();
            m_releaseInfo.firmwareSize = asset["size"].as<size_t>();
            m_releaseInfo.hasMatchingBinary = true;
            LOG_INFO("Matched binary! Size: %u", m_releaseInfo.firmwareSize);
            break;
        }
    }

    return true;
}

void GitHubOtaService::handleFirmwareChunk(const uint8_t* data, size_t size, bool isFinal)
{
    esp_err_t err = esp_ota_write(m_otaHandle, data, size);
    if (ESP_OK != err)
    {
        esp_ota_abort(m_otaHandle);
        m_otaHandle = 0;
        setError("OTA write failed: " + String(esp_err_to_name(err)));
        return;
    }

    m_bytesWritten += size;

    /* Update progress */
    if (m_releaseInfo.firmwareSize > 0)
    {
        m_downloadProgress = static_cast<uint8_t>((m_bytesWritten * 100U) / m_releaseInfo.firmwareSize);
    }

    if (isFinal)
    {
        err = esp_ota_end(m_otaHandle);
        m_otaHandle = 0;

        if (ESP_OK != err)
        {
            setError("OTA end failed: " + String(esp_err_to_name(err)));
            return;
        }

        err = esp_ota_set_boot_partition(m_updatePartition);
        if (ESP_OK != err)
        {
            setError("Failed to set boot partition: " + String(esp_err_to_name(err)));
            return;
        }

        m_downloadProgress = 100;
        m_state = OtaState::DOWNLOAD_COMPLETE;
        LOG_INFO("OTA update complete. Ready for reboot.");
    }
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
