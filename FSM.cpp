#include <cassert>
#include <cstdlib>
#include "FSM.h"

// Constructor
CFSM::CFSM()
{
    // Initialize States
    m_stateInitial.Set(this, &CFSM::BeginStateInitial, &CFSM::StateInitial, &CFSM::EndStateInitial);

    // Initialize State Machine
    m_pCurrentState = static_cast<CState*>(&m_stateInitial);
    m_pNewState = nullptr;

    // Initialize concurrent state
    m_pConcurrentState = nullptr;
    m_pNewConcurrentState = nullptr;
    bStopConcurrent = false;
}

//======================================================================================================
// Global Functions

void CFSM::Update()
{
    // Check New State
    if (m_pNewState)
    {
        if (m_pCurrentState)
            m_pCurrentState->ExecuteEndState();

        m_pCurrentState = m_pNewState;
        m_pNewState = nullptr;

        m_pCurrentState->ExecuteBeginState();
    }

    // Check New Concurrent State
    if (m_pNewConcurrentState)
    {
        if (m_pConcurrentState)
            m_pConcurrentState->ExecuteEndState();

        m_pConcurrentState = m_pNewConcurrentState;
        m_pNewConcurrentState = nullptr;

        m_pConcurrentState->ExecuteBeginState();
    }

    // Stop Concurrent State
    if (bStopConcurrent && m_pConcurrentState)
    {
        m_pConcurrentState->ExecuteEndState();
        m_pConcurrentState = nullptr;
        bStopConcurrent = false;
    }

    // Execute States
    if (m_pCurrentState)
        m_pCurrentState->ExecuteState();

    if (m_pConcurrentState)
        m_pConcurrentState->ExecuteState();
}

//======================================================================================================
// State Functions

bool CFSM::IsState(CState& State) const
{
    return (m_pCurrentState == &State);
}

bool CFSM::GotoState(CState& NewState)
{
    if (IsState(NewState) && m_pNewState == &NewState)
        return true;

    m_pNewState = &NewState;
    return true;
}

// Goto concurrent state
bool CFSM::GotoConcurrentState(CState& NewState)
{
    if (m_pConcurrentState == &NewState && m_pNewConcurrentState == &NewState)
        return true;

    m_pNewConcurrentState = &NewState;
    return true;
}

// Stop concurrent state
void CFSM::StopConcurrentState()
{
    bStopConcurrent = true;
}