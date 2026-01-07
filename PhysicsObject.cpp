#include "StdAfx.h"
#include "PhysicsObject.h"

const float c_fFrameTime = 0.02f;
const float EPSILON		 = 0.001f;

IPhysicsWorld* IPhysicsWorld::ms_pWorld = NULL;
IObjectManager* IObjectManager::ms_ObjManager = NULL;

void CPhysicsObject::Update(float fElapsedTime)
{
	if (m_xPushingPosition.isPlaying())
		m_xPushingPosition.Interpolate(fElapsedTime);
	if (m_yPushingPosition.isPlaying())
		m_yPushingPosition.Interpolate(fElapsedTime);
}

void CPhysicsObject::Accumulate(D3DXVECTOR3 * pv3Position)
{
	// Add validation for output pointer
	if (!pv3Position)
	{
		TraceError("CPhysicsObject::Accumulate - Null position pointer provided");
		return;
	}

	// If object is moving, give minor power to object.
	float fForce = 0.0f;

	if (fabs(m_v3Velocity.x) < EPSILON ||
		fabs(m_v3Velocity.y) < EPSILON ||
		fabs(m_v3Velocity.z) < EPSILON )
	{
		fForce -= (m_fMass * m_fFriction);
	}

	// Add validation for mass to prevent division by zero - but continue with safe defaults
	if (fabs(m_fMass) < EPSILON)
	{
		// Log warning but don't block - use zero acceleration as safe default
		Tracenf("CPhysicsObject::Accumulate - Mass is very small (%f), using zero acceleration", m_fMass);
		m_v3Acceleration = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
	}
	else
	{
		m_v3Acceleration = m_v3Direction * (fForce / m_fMass);
	}
	m_v3Velocity += m_v3Acceleration;
	if (m_v3Velocity.x * m_v3Direction.x < EPSILON)
	{
		m_v3Velocity.x = 0.0f;
		m_v3Direction.x = 0.0f;
	}
	if (m_v3Velocity.y * m_v3Direction.y < EPSILON)
	{
		m_v3Velocity.y = 0.0f;
		m_v3Direction.y = 0.0f;
	}
	if (m_v3Velocity.z * m_v3Direction.z < EPSILON)
	{
		m_v3Velocity.z = 0.0f;
		m_v3Direction.z = 0.0f;
	}

	pv3Position->x += m_v3Velocity.x;
	pv3Position->y += m_v3Velocity.y;
	pv3Position->z += m_v3Velocity.z;
}

void CPhysicsObject::IncreaseExternalForce(const D3DXVECTOR3 & c_rvBasePosition, float fForce)
{
	// Add validation for force value - only block on truly invalid values
	if (!isfinite(fForce))
	{
		TraceError("CPhysicsObject::IncreaseExternalForce - Invalid force value (not finite): %f, skipping", fForce);
		return;
	}

	// Add validation for mass to prevent division by zero - but continue with safe defaults
	if (fabs(m_fMass) < EPSILON)
	{
		// Log warning but don't block - use safe default mass
		Tracenf("CPhysicsObject::IncreaseExternalForce - Mass is very small (%f), using default mass 1.0", m_fMass);
		m_v3Acceleration = m_v3Direction * (fForce / 1.0f);
		m_v3Velocity = m_v3Acceleration;
	}
	else
	{
		// Accumulate Acceleration by External Force
		m_v3Acceleration = m_v3Direction * (fForce / m_fMass);
		m_v3Velocity = m_v3Acceleration;
	}
/*
	Tracenf("force %f, mass %f, accel (%f, %f, %f)", fForce, m_fMass, 
		m_v3Acceleration.x, 
		m_v3Acceleration.y,
		m_v3Acceleration.z);
*/
	// NOTE : ���� ��ġ�� ���صд�. �ٵ� 100���� ũ�ٸ�? ;
	const int LoopValue = 100;
	D3DXVECTOR3 v3Movement(0.0f, 0.0f, 0.0f);

	for(int i = 0; i < LoopValue; ++i)
	{
		Accumulate(&v3Movement);

		// VICTIM_COLLISION_TEST
		IPhysicsWorld* pWorld = IPhysicsWorld::GetPhysicsWorld();
		if (pWorld)
		{
			if (pWorld->isPhysicalCollision(c_rvBasePosition + v3Movement))
			{
				Initialize();
				return;

				//for (float fRatio = 0.0f; fRatio < 1.0f; fRatio += 0.1f)
				//{
				//	// ���� �����ϰ� üũ�Ѵ�
				//	if (pWorld->isPhysicalCollision(c_rvBasePosition + v3Movement * fRatio))
				//	{
				//		v3Movement = D3DXVECTOR3 (0.0f, 0.0f, 0.0f);
				//		break;
				//	}
				//}
				//break;
			}
		}
		// VICTIM_COLLISION_TEST_END

		if (fabs(m_v3Velocity.x) < EPSILON &&
			fabs(m_v3Velocity.y) < EPSILON &&
			fabs(m_v3Velocity.z) < EPSILON )
			break;
	}	

	// NOTE: This is the OLD implementation that caused the GetBlendingPosition bug
	// It only stores the delta movement, not the final position
	// This will be replaced by SetLastPosition(current, delta, time) below
	// SetLastPosition(v3Movement, float(LoopValue) * c_fFrameTime);

	// FIXED: Store both delta and final position
	// The caller should now use SetLastPosition(currentPos, delta, time)
	// For backward compatibility, we still set the delta position here
	TPixelPosition deltaPos;
	deltaPos.x = v3Movement.x;
	deltaPos.y = v3Movement.y;
	deltaPos.z = v3Movement.z;
	
	TPixelPosition currentPos;
	currentPos.x = c_rvBasePosition.x;
	currentPos.y = c_rvBasePosition.y;
	currentPos.z = c_rvBasePosition.z;
	
	SetLastPosition(currentPos, deltaPos, float(LoopValue) * c_fFrameTime);

	if( m_pActorInstance )
	{
		IObjectManager* pObjectManager = IObjectManager::GetObjectManager();
		pObjectManager->AdjustCollisionWithOtherObjects( m_pActorInstance );
	}
}

// ============================================================================
// FIXED IMPLEMENTATION - Stores both final position and delta
// ============================================================================
// This fix resolves the Rolling Dagger bug and "Fly" synchronization issues
// 
// OLD BUG: SetLastPosition only stored delta, causing GetBlendingPosition 
//          to return current + delta instead of final position
//
// NEW: Stores both delta and final = current + delta
// ============================================================================

void CPhysicsObject::SetLastPosition(const TPixelPosition& c_rPosition, const TPixelPosition& c_rDeltaPosition, float fBlendingTime)
{
    m_v3FinalPosition.x = float(c_rPosition.x + c_rDeltaPosition.x);
    m_v3FinalPosition.y = float(c_rPosition.y + c_rDeltaPosition.y);
    m_v3FinalPosition.z = float(c_rPosition.z + c_rDeltaPosition.z);
    m_v3DeltaPosition.x = float(c_rDeltaPosition.x);
    m_v3DeltaPosition.y = float(c_rDeltaPosition.y);
    m_v3DeltaPosition.z = float(c_rDeltaPosition.z);
    m_xPushingPosition.Setup(0.0f, c_rDeltaPosition.x, fBlendingTime);
    m_yPushingPosition.Setup(0.0f, c_rDeltaPosition.y, fBlendingTime);
}
void CPhysicsObject::GetFinalPosition(TPixelPosition* pPosition) const
{
	// Add validation for output pointer
	if (!pPosition)
	{
		TraceError("CPhysicsObject::GetFinalPosition - Null position pointer provided");
		return;
	}

	pPosition->x = (m_v3FinalPosition.x);
	pPosition->y = (m_v3FinalPosition.y);
	pPosition->z = (m_v3FinalPosition.z);
}

void CPhysicsObject::GetDeltaPosition(TPixelPosition* pPosition) const
{
	// Add validation for output pointer
	if (!pPosition)
	{
		TraceError("CPhysicsObject::GetDeltaPosition - Null position pointer provided");
		return;
	}

	pPosition->x = (m_v3DeltaPosition.x);
	pPosition->y = (m_v3DeltaPosition.y);
	pPosition->z = (m_v3DeltaPosition.z);
}
// Returns remaining time in the blending animation
float CPhysicsObject::GetRemainingTime() const
{
	// Get the remaining time from either X or Y interpolation
	// (they should be the same duration)
	if (m_xPushingPosition.isPlaying())
		return m_xPushingPosition.GetRemainingTime();
	if (m_yPushingPosition.isPlaying())
		return m_yPushingPosition.GetRemainingTime();
	return 0.0f;
}

void CPhysicsObject::SetDirection(const D3DXVECTOR3 & c_rv3Direction)
{
	m_v3Direction.x = c_rv3Direction.x;
	m_v3Direction.y = c_rv3Direction.y;
	m_v3Direction.z = c_rv3Direction.z;
}

float CPhysicsObject::GetXMovement() const 
{
	return m_xPushingPosition.GetChangingValue();
}

float CPhysicsObject::GetYMovement() const
{
	return m_yPushingPosition.GetChangingValue();
}

bool CPhysicsObject::isBlending() const
{
	// NOTE : IncreaseExternalForce() �� ���� �и��� ó�����ΰ�?
	if (0.0f != D3DXVec3Length(&m_v3Velocity))
		return true;

	// NOTE : SetLastPosition() �� ���� �и��� ó�����ΰ�?
	if (m_xPushingPosition.isPlaying() ||
		m_yPushingPosition.isPlaying())
		return true;

	return false;
}

void CPhysicsObject::Initialize()
{
	m_fMass = 1.0f;
	m_fFriction = 0.3f;
	m_v3Direction = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
	m_v3Acceleration = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
	m_v3Velocity = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
	m_v3LastPosition = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
	m_v3FinalPosition = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
	m_v3DeltaPosition = D3DXVECTOR3(0.0f, 0.0f, 0.0f);

	m_xPushingPosition.Initialize();
	m_yPushingPosition.Initialize();
}

CPhysicsObject::CPhysicsObject()
{
	m_pActorInstance = NULL;
	Initialize(); // Make sure new members are initialized
}

CPhysicsObject::~CPhysicsObject()
{
}