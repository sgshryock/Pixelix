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
 * @file   DisplayCommandQueue.cpp
 * @brief  Thread-safe display command queue for serialized LED strip access
 * @author Andreas Merkle <web@blue-andi.de>
 */

/******************************************************************************
 * Includes
 *****************************************************************************/
#include "DisplayCommandQueue.h"
#include "Display.h"

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

const char* DisplayCommandQueue::TASK_NAME = "dispCmdQueue";

/******************************************************************************
 * Public Methods
 *****************************************************************************/

bool DisplayCommandQueue::begin()
{
    bool success = false;

    /* Only start if not already running */
    if (nullptr == m_taskHandle)
    {
        /* Create the command queue */
        m_commandQueue = xQueueCreate(QUEUE_DEPTH, sizeof(DisplayCommandMsg));

        if (nullptr != m_commandQueue)
        {
            m_reqExit = false;

            /* Create the command processor task */
            BaseType_t result = xTaskCreatePinnedToCore(
                commandProcessorTask,
                TASK_NAME,
                TASK_STACK_SIZE,
                this,
                TASK_PRIORITY,
                &m_taskHandle,
                TASK_CORE
            );

            if (pdPASS == result)
            {
                success = true;
            }
            else
            {
                /* Task creation failed, clean up queue */
                vQueueDelete(m_commandQueue);
                m_commandQueue = nullptr;
            }
        }
    }

    return success;
}

void DisplayCommandQueue::end()
{
    if (nullptr != m_taskHandle)
    {
        /* Request the task to exit */
        m_reqExit = true;

        /* Send shutdown command to unblock the task if waiting on queue */
        DisplayCommandMsg shutdownMsg = {
            DisplayCommand::CMD_SHUTDOWN,
            0U,
            nullptr
        };
        (void)xQueueSend(m_commandQueue, &shutdownMsg, pdMS_TO_TICKS(100));

        /* Wait for task to exit (with timeout) */
        uint32_t timeout = 2000U;
        while ((nullptr != m_taskHandle) && (timeout > 0U))
        {
            vTaskDelay(pdMS_TO_TICKS(10));
            timeout -= 10U;
        }

        /* Force delete if still running */
        if (nullptr != m_taskHandle)
        {
            vTaskDelete(m_taskHandle);
            m_taskHandle = nullptr;
        }

        /* Clean up the queue */
        if (nullptr != m_commandQueue)
        {
            vQueueDelete(m_commandQueue);
            m_commandQueue = nullptr;
        }
    }
}

bool DisplayCommandQueue::enqueueShow()
{
    DisplayCommandMsg msg = {
        DisplayCommand::CMD_SHOW,
        0U,
        nullptr
    };

    return enqueueCommand(msg, 0);
}

bool DisplayCommandQueue::enqueueClear()
{
    DisplayCommandMsg msg = {
        DisplayCommand::CMD_CLEAR,
        0U,
        nullptr
    };

    return enqueueCommand(msg, 0);
}

bool DisplayCommandQueue::enqueueOn()
{
    DisplayCommandMsg msg = {
        DisplayCommand::CMD_ON,
        0U,
        nullptr
    };

    return enqueueCommand(msg, 0);
}

bool DisplayCommandQueue::enqueueOff()
{
    DisplayCommandMsg msg = {
        DisplayCommand::CMD_OFF,
        0U,
        nullptr
    };

    return enqueueCommand(msg, 0);
}

bool DisplayCommandQueue::enqueueBrightness(uint8_t brightness)
{
    DisplayCommandMsg msg = {
        DisplayCommand::CMD_SET_BRIGHTNESS,
        brightness,
        nullptr
    };

    return enqueueCommand(msg, 0);
}

bool DisplayCommandQueue::showSync(TickType_t timeout)
{
    return executeSyncCommand(DisplayCommand::CMD_SHOW, 0U, timeout);
}

bool DisplayCommandQueue::clearSync(TickType_t timeout)
{
    return executeSyncCommand(DisplayCommand::CMD_CLEAR, 0U, timeout);
}

bool DisplayCommandQueue::onSync(TickType_t timeout)
{
    return executeSyncCommand(DisplayCommand::CMD_ON, 0U, timeout);
}

bool DisplayCommandQueue::offSync(TickType_t timeout)
{
    return executeSyncCommand(DisplayCommand::CMD_OFF, 0U, timeout);
}

bool DisplayCommandQueue::setBrightnessSync(uint8_t brightness, TickType_t timeout)
{
    return executeSyncCommand(DisplayCommand::CMD_SET_BRIGHTNESS, brightness, timeout);
}

IDisplay& DisplayCommandQueue::getDisplay()
{
    return Display::getInstance();
}

/******************************************************************************
 * Protected Methods
 *****************************************************************************/

/******************************************************************************
 * Private Methods
 *****************************************************************************/

DisplayCommandQueue::DisplayCommandQueue() :
    m_commandQueue(nullptr),
    m_taskHandle(nullptr),
    m_reqExit(false)
{
}

DisplayCommandQueue::~DisplayCommandQueue()
{
    end();
}

bool DisplayCommandQueue::enqueueCommand(const DisplayCommandMsg& msg, TickType_t timeout)
{
    bool success = false;

    if (nullptr != m_commandQueue)
    {
        if (pdTRUE == xQueueSend(m_commandQueue, &msg, timeout))
        {
            success = true;
        }
    }

    return success;
}

bool DisplayCommandQueue::executeSyncCommand(DisplayCommand cmd, uint8_t param, TickType_t timeout)
{
    bool success = false;

    /* Create a completion semaphore for this command */
    SemaphoreHandle_t completionSem = xSemaphoreCreateBinary();

    if (nullptr != completionSem)
    {
        DisplayCommandMsg msg = {
            cmd,
            param,
            completionSem
        };

        /* Queue the command */
        if (pdTRUE == xQueueSend(m_commandQueue, &msg, timeout))
        {
            /* Wait for command completion */
            if (pdTRUE == xSemaphoreTake(completionSem, timeout))
            {
                success = true;
            }
        }

        /* Clean up the semaphore */
        vSemaphoreDelete(completionSem);
    }

    return success;
}

void DisplayCommandQueue::commandProcessorTask(void* param)
{
    DisplayCommandQueue* self = static_cast<DisplayCommandQueue*>(param);

    if (nullptr != self)
    {
        DisplayCommandMsg   msg;
        Display&            display = Display::getInstance();

        while (false == self->m_reqExit)
        {
            /* Wait for a command with timeout to allow exit check */
            if (pdTRUE == xQueueReceive(self->m_commandQueue, &msg, pdMS_TO_TICKS(100)))
            {
                /* Check for shutdown command */
                if (DisplayCommand::CMD_SHUTDOWN == msg.command)
                {
                    break;
                }

                /* Wait for display to be ready before executing command */
                while (false == display.isReady())
                {
                    vTaskDelay(1);
                }

                /* Execute the command */
                self->executeCommand(msg);

                /* Signal completion if this was a sync operation */
                if (nullptr != msg.completionSem)
                {
                    xSemaphoreGive(msg.completionSem);
                }
            }
        }

        /* Clear task handle before exiting */
        self->m_taskHandle = nullptr;
    }

    /* Delete ourselves */
    vTaskDelete(nullptr);
}

void DisplayCommandQueue::executeCommand(const DisplayCommandMsg& msg)
{
    Display& display = Display::getInstance();

    switch (msg.command)
    {
    case DisplayCommand::CMD_SHOW:
        display.show();
        break;

    case DisplayCommand::CMD_CLEAR:
        display.clear();
        break;

    case DisplayCommand::CMD_ON:
        display.on();
        break;

    case DisplayCommand::CMD_OFF:
        display.off();
        break;

    case DisplayCommand::CMD_SET_BRIGHTNESS:
        display.setBrightness(msg.brightness);
        break;

    case DisplayCommand::CMD_SHUTDOWN:
        /* Handled in task loop */
        break;

    default:
        /* Unknown command - ignore */
        break;
    }
}

/******************************************************************************
 * External Functions
 *****************************************************************************/

/******************************************************************************
 * Local Functions
 *****************************************************************************/
