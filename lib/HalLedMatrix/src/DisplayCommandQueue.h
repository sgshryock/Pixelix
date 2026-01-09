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
 * @file   DisplayCommandQueue.h
 * @brief  Thread-safe display command queue for serialized LED strip access
 * @author Andreas Merkle <web@blue-andi.de>
 *
 * @addtogroup HAL
 *
 * @{
 */

#ifndef DISPLAY_COMMAND_QUEUE_H
#define DISPLAY_COMMAND_QUEUE_H

/******************************************************************************
 * Compile Switches
 *****************************************************************************/

/******************************************************************************
 * Includes
 *****************************************************************************/
#include <stdint.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <IDisplay.hpp>

#include "Board.h"

/******************************************************************************
 * Macros
 *****************************************************************************/

/******************************************************************************
 * Types and Classes
 *****************************************************************************/

/**
 * Display command types that can be queued.
 */
enum class DisplayCommand : uint8_t
{
    CMD_SHOW = 0,       /**< Copy framebuffer to LEDs and trigger async update */
    CMD_CLEAR,          /**< Clear display to black */
    CMD_ON,             /**< Power display on */
    CMD_OFF,            /**< Power display off */
    CMD_SET_BRIGHTNESS, /**< Set brightness level */
    CMD_SHUTDOWN        /**< Stop the command processor task */
};

/**
 * Display command message structure for queue.
 */
struct DisplayCommandMsg
{
    DisplayCommand      command;        /**< Command type */
    uint8_t             brightness;     /**< Brightness value (for CMD_SET_BRIGHTNESS) */
    SemaphoreHandle_t   completionSem;  /**< Completion semaphore for sync ops (nullptr for async) */
};

/**
 * Thread-safe display command queue.
 *
 * This class provides a single point of access for all display hardware operations.
 * Commands are queued and processed by a dedicated FreeRTOS task, eliminating
 * race conditions between multiple tasks accessing the display.
 */
class DisplayCommandQueue
{
public:

    /**
     * Get display command queue instance.
     *
     * @return DisplayCommandQueue instance
     */
    static DisplayCommandQueue& getInstance()
    {
        static DisplayCommandQueue instance;

        return instance;
    }

    /**
     * Initialize the command queue and start the processor task.
     *
     * @return If successful, returns true otherwise false.
     */
    bool begin();

    /**
     * Stop the processor task and clean up resources.
     */
    void end();

    /**
     * Is the command processor task running?
     *
     * @return true if running, false otherwise.
     */
    bool isRunning() const
    {
        return (nullptr != m_taskHandle);
    }

    /**
     * Enqueue a show command (non-blocking).
     * Copies the framebuffer to the LED strip and triggers an async update.
     *
     * @return true if command was queued, false if queue is full.
     */
    bool enqueueShow();

    /**
     * Enqueue a clear command (non-blocking).
     * Clears the display to black.
     *
     * @return true if command was queued, false if queue is full.
     */
    bool enqueueClear();

    /**
     * Enqueue a display on command (non-blocking).
     *
     * @return true if command was queued, false if queue is full.
     */
    bool enqueueOn();

    /**
     * Enqueue a display off command (non-blocking).
     *
     * @return true if command was queued, false if queue is full.
     */
    bool enqueueOff();

    /**
     * Enqueue a brightness change command (non-blocking).
     *
     * @param[in] brightness    Brightness value [0; 255]
     *
     * @return true if command was queued, false if queue is full.
     */
    bool enqueueBrightness(uint8_t brightness);

    /**
     * Execute show command synchronously (blocking).
     * Waits for the command to complete before returning.
     *
     * @param[in] timeout   Maximum time to wait in ticks.
     *
     * @return true if command completed, false on timeout.
     */
    bool showSync(TickType_t timeout = portMAX_DELAY);

    /**
     * Execute clear command synchronously (blocking).
     * Waits for the command to complete before returning.
     *
     * @param[in] timeout   Maximum time to wait in ticks.
     *
     * @return true if command completed, false on timeout.
     */
    bool clearSync(TickType_t timeout = portMAX_DELAY);

    /**
     * Execute display on command synchronously (blocking).
     * Waits for the command to complete before returning.
     *
     * @param[in] timeout   Maximum time to wait in ticks.
     *
     * @return true if command completed, false on timeout.
     */
    bool onSync(TickType_t timeout = portMAX_DELAY);

    /**
     * Execute display off command synchronously (blocking).
     * Waits for the command to complete before returning.
     *
     * @param[in] timeout   Maximum time to wait in ticks.
     *
     * @return true if command completed, false on timeout.
     */
    bool offSync(TickType_t timeout = portMAX_DELAY);

    /**
     * Execute brightness change command synchronously (blocking).
     * Waits for the command to complete before returning.
     *
     * @param[in] brightness    Brightness value [0; 255]
     * @param[in] timeout       Maximum time to wait in ticks.
     *
     * @return true if command completed, false on timeout.
     */
    bool setBrightnessSync(uint8_t brightness, TickType_t timeout = portMAX_DELAY);

    /**
     * Get direct access to the display for framebuffer operations.
     * Note: Only use this for drawing to the framebuffer, not for
     * show/clear/on/off/setBrightness operations.
     *
     * @return Reference to the display.
     */
    IDisplay& getDisplay();

private:

    /** Command queue depth */
    static const uint32_t   QUEUE_DEPTH         = 8U;

    /** Command processor task stack size in bytes */
    static const uint32_t   TASK_STACK_SIZE     = 2048U;

    /** Command processor task priority (higher than updateTask's 4) */
    static const UBaseType_t TASK_PRIORITY      = 5U;

    /** Command processor task core */
    static const BaseType_t TASK_CORE           = APP_CPU_NUM;

    /** Task name */
    static const char*      TASK_NAME;

    /** Command queue handle */
    QueueHandle_t           m_commandQueue;

    /** Task handle */
    TaskHandle_t            m_taskHandle;

    /** Flag to request task exit */
    volatile bool           m_reqExit;

    /**
     * Construct display command queue.
     */
    DisplayCommandQueue();

    /**
     * Destroy display command queue.
     */
    ~DisplayCommandQueue();

    /**
     * Enqueue a command with optional blocking.
     *
     * @param[in] msg       Command message to queue.
     * @param[in] timeout   Queue timeout (0 for non-blocking).
     *
     * @return true if command was queued, false otherwise.
     */
    bool enqueueCommand(const DisplayCommandMsg& msg, TickType_t timeout);

    /**
     * Execute a synchronous command with completion notification.
     *
     * @param[in] cmd       Command type.
     * @param[in] param     Command parameter (e.g., brightness).
     * @param[in] timeout   Maximum time to wait for completion.
     *
     * @return true if command completed, false on timeout or error.
     */
    bool executeSyncCommand(DisplayCommand cmd, uint8_t param, TickType_t timeout);

    /**
     * Command processor task function.
     *
     * @param[in] param     Pointer to DisplayCommandQueue instance.
     */
    static void commandProcessorTask(void* param);

    /**
     * Execute a single command on the display.
     *
     * @param[in] msg   Command message to execute.
     */
    void executeCommand(const DisplayCommandMsg& msg);

    /* Not copyable */
    DisplayCommandQueue(const DisplayCommandQueue& queue);
    DisplayCommandQueue& operator=(const DisplayCommandQueue& queue);
};

/******************************************************************************
 * Functions
 *****************************************************************************/

#endif  /* DISPLAY_COMMAND_QUEUE_H */

/** @} */
