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
 * @file   GitHubOtaService.h
 * @brief  GitHub OTA update service
 * @author Shryock Edition
 *
 * @addtogroup SERVICE
 *
 * @{
 */

#ifndef GITHUB_OTA_SERVICE_H
#define GITHUB_OTA_SERVICE_H

/******************************************************************************
 * Compile Switches
 *****************************************************************************/

/******************************************************************************
 * Includes
 *****************************************************************************/
#include <Arduino.h>
#include <esp_ota_ops.h>
#include <IService.hpp>
#include <Mutex.hpp>
#include <ArduinoJson.h>

/******************************************************************************
 * Macros
 *****************************************************************************/

/******************************************************************************
 * Types and Classes
 *****************************************************************************/

/**
 * OTA update state machine states
 */
enum class OtaState
{
    IDLE,                   /**< No operation in progress */
    CHECKING_RELEASE,       /**< Fetching latest release info from GitHub API */
    RELEASE_INFO_READY,     /**< Release info available, waiting for user action */
    DOWNLOADING_FIRMWARE,   /**< Downloading firmware binary */
    DOWNLOADING_FILESYSTEM, /**< Downloading filesystem binary */
    DOWNLOAD_COMPLETE,      /**< Download complete, ready to reboot */
    ERROR                   /**< An error occurred */
};

/**
 * Release information structure
 */
struct ReleaseInfo
{
    String tagName;          /**< Release version tag (e.g., "v10.0.1-SE") */
    String releaseNotes;     /**< Release notes/body (truncated to 1KB) */
    String downloadUrl;      /**< Direct download URL for firmware binary */
    size_t firmwareSize;     /**< Expected firmware size in bytes */
    bool   hasMatchingBinary;/**< Whether a firmware binary for this target exists */
    String filesystemUrl;    /**< Direct download URL for filesystem binary */
    size_t filesystemSize;   /**< Expected filesystem size in bytes */
    bool   hasFilesystem;    /**< Whether a filesystem binary for this target exists */

    ReleaseInfo() :
        tagName(),
        releaseNotes(),
        downloadUrl(),
        firmwareSize(0),
        hasMatchingBinary(false),
        filesystemUrl(),
        filesystemSize(0),
        hasFilesystem(false)
    {
    }

    void clear()
    {
        tagName.clear();
        releaseNotes.clear();
        downloadUrl.clear();
        firmwareSize = 0;
        hasMatchingBinary = false;
        filesystemUrl.clear();
        filesystemSize = 0;
        hasFilesystem = false;
    }
};

/**
 * GitHub OTA Service for checking and downloading firmware updates from GitHub releases.
 */
class GitHubOtaService : public IService
{
public:
    /**
     * Get the singleton instance.
     *
     * @return GitHubOtaService instance
     */
    static GitHubOtaService& getInstance()
    {
        static GitHubOtaService instance;
        return instance;
    }

    /**
     * Start the service.
     *
     * @return If successful started, it will return true otherwise false.
     */
    bool start() final;

    /**
     * Stop the service.
     */
    void stop() final;

    /**
     * Process the service (must be called periodically).
     */
    void process() final;

    /**
     * Start checking for updates. Non-blocking.
     *
     * @param[in] repoUrl GitHub repository URL (e.g., "https://github.com/user/repo")
     * @return true if check started, false if already in progress
     */
    bool checkForUpdates(const String& repoUrl);

    /**
     * Start downloading and flashing the firmware. Non-blocking.
     *
     * @return true if download started, false if not ready or already in progress
     */
    bool startOtaUpdate();

    /**
     * Abort any ongoing operation.
     */
    void abort();

    /**
     * Get current state.
     *
     * @return Current OTA state
     */
    OtaState getState() const;

    /**
     * Get last error message.
     *
     * @return Error message string
     */
    const String& getErrorMessage() const;

    /**
     * Get release information (valid when state is RELEASE_INFO_READY or later).
     *
     * @return Release info structure
     */
    const ReleaseInfo& getReleaseInfo() const;

    /**
     * Get firmware download progress (0-100).
     *
     * @return Progress percentage
     */
    uint8_t getDownloadProgress() const;

    /**
     * Get filesystem download progress (0-100).
     *
     * @return Progress percentage
     */
    uint8_t getFilesystemProgress() const;

private:
    /** Maximum release notes length */
    static const size_t MAX_RELEASE_NOTES_LEN = 1024U;

    /** HTTP response buffer size */
    static const size_t HTTP_BUFFER_SIZE = 8192U;

    mutable Mutex     m_mutex;          /**< Mutex for thread safety */
    OtaState          m_state;          /**< Current state */
    String            m_errorMessage;   /**< Last error message */
    ReleaseInfo       m_releaseInfo;    /**< Release information */
    String            m_repoUrl;        /**< GitHub repository URL */
    uint32_t          m_restId;         /**< REST service request ID for async requests */

    esp_ota_handle_t        m_otaHandle;       /**< OTA handle for flash writes */
    const esp_partition_t*  m_updatePartition; /**< Target partition for update */
    size_t                  m_bytesWritten;    /**< Bytes written to flash */
    uint8_t                 m_downloadProgress;/**< Download progress 0-100 */

    const esp_partition_t*  m_spiffsPartition; /**< SPIFFS partition for filesystem update */
    size_t                  m_fsBytesWritten;  /**< Filesystem bytes written to flash */
    uint8_t                 m_fsDownloadProgress; /**< Filesystem download progress 0-100 */

    /**
     * Construct a GitHubOtaService instance.
     */
    GitHubOtaService();

    /**
     * Destroy the GitHubOtaService instance.
     */
    ~GitHubOtaService();

    /* Non-copyable */
    GitHubOtaService(const GitHubOtaService&) = delete;
    GitHubOtaService& operator=(const GitHubOtaService&) = delete;

    /**
     * Build GitHub API URL from repository URL.
     *
     * @param[in] repoUrl Repository URL (e.g., "https://github.com/user/repo")
     * @return API URL for latest release
     */
    String buildApiUrl(const String& repoUrl) const;

    /**
     * Get expected firmware binary name for this target.
     *
     * @return Binary filename (e.g., "firmware_esp32-s3-devkitc-1.bin")
     */
    String getExpectedBinaryName() const;

    /**
     * Get expected filesystem binary name for this target.
     *
     * @return Binary filename (e.g., "littlefs_esp32-s3-devkitc-1.bin")
     */
    String getExpectedFilesystemName() const;

    /**
     * Parse GitHub release JSON response from DynamicJsonDocument.
     *
     * @param[in] jsonDoc JSON document containing release info
     * @return true if parsing succeeded
     */
    bool parseReleaseJson(const DynamicJsonDocument& jsonDoc);

    /**
     * Handle release check response.
     */
    void handleReleaseResponse();

    /**
     * Handle firmware download response chunk.
     *
     * @param[in] data    Chunk data
     * @param[in] size    Chunk size
     * @param[in] isFinal true if this is the last chunk
     */
    void handleFirmwareChunk(const uint8_t* data, size_t size, bool isFinal);

    /**
     * Handle filesystem download response chunk.
     *
     * @param[in] data    Chunk data
     * @param[in] size    Chunk size
     * @param[in] isFinal true if this is the last chunk
     */
    void handleFilesystemChunk(const uint8_t* data, size_t size, bool isFinal);

    /**
     * Start filesystem download after firmware is complete.
     *
     * @return true if filesystem download started, false on error
     */
    bool startFilesystemDownload();

    /**
     * Finalize the OTA update (set boot partition).
     */
    void finalizeUpdate();

    /**
     * Set error state with message.
     *
     * @param[in] message Error message
     */
    void setError(const String& message);
};

/******************************************************************************
 * Functions
 *****************************************************************************/

#endif /* GITHUB_OTA_SERVICE_H */

/** @} */
