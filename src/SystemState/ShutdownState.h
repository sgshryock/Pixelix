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
 * @file   ShutdownState.h
 * @brief  System state: Shutdown (deep sleep)
 * @author Andreas Merkle <web@blue-andi.de>
 *
 * @addtogroup SYS_STATES
 *
 * @{
 */

#ifndef SHUTDOWNSTATE_H
#define SHUTDOWNSTATE_H

/******************************************************************************
 * Compile Switches
 *****************************************************************************/

/******************************************************************************
 * Includes
 *****************************************************************************/
#include <stdint.h>
#include <StateMachine.hpp>
#include <SimpleTimer.hpp>

/******************************************************************************
 * Macros
 *****************************************************************************/

/******************************************************************************
 * Types and Classes
 *****************************************************************************/

/**
 * System state: Shutdown
 * Gracefully stops all services and enters deep sleep.
 */
class ShutdownState : public AbstractState
{
public:

    /**
     * Get state instance.
     *
     * @return State instance
     */
    static ShutdownState& getInstance()
    {
        static ShutdownState instance; /* singleton idiom to force initialization in the first usage. */

        return instance;
    }

    /**
     * The entry is called once, a state is entered.
     *
     * @param[in] sm    Responsible state machine
     */
    void entry(StateMachine& sm) final;

    /**
     * The process routine is called cyclic, as long as the state is active.
     *
     * @param[in] sm    Responsible state machine
     */
    void process(StateMachine& sm) final;

    /**
     * The exit is called once, a state will be left.
     *
     * @param[in] sm    Responsible state machine
     */
    void exit(StateMachine& sm) final;

private:

    /** Wait timer in ms, after that all services will be stopped. */
    static const uint32_t WAIT_TILL_STOP_SVC = 500U;

    /** Wait timer */
    SimpleTimer m_timer;

    /**
     * Constructs the state.
     */
    ShutdownState() :
        m_timer()
    {
    }

    /**
     * Destroys the state.
     */
    ~ShutdownState()
    {
    }

    ShutdownState(const ShutdownState& state);
    ShutdownState& operator=(const ShutdownState& state);

};

/******************************************************************************
 * Functions
 *****************************************************************************/

#endif  /* SHUTDOWNSTATE_H */

/** @} */
