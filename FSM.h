#ifndef _fsm_fsm_h
#define _fsm_fsm_h

#include "state.h"

class CFSM
{
protected:
    CState* m_pCurrentState;         // Current State
    CState* m_pNewState;             // New State

    // Concurrent State
    CState* m_pConcurrentState;      // Active concurrent state
    CState* m_pNewConcurrentState;   // New concurrent state
    bool    bStopConcurrent;         // Flag to stop concurrent state

    CStateTemplate<CFSM> m_stateInitial; // Initial State

public:
    // Constructor
    CFSM();

    // Destructor
    virtual ~CFSM() {}

    // Update FSM
    virtual void Update();

    // State Functions
    bool IsState(CState& State) const;
    bool GotoState(CState& NewState);
    bool GotoConcurrentState(CState& NewState);   // New function for concurrent state
    void StopConcurrentState();                   // New function to stop concurrent state

    // Initial State callbacks
    virtual void BeginStateInitial() {}
    virtual void StateInitial() {}
    virtual void EndStateInitial() {}
};

#endif