#pragma once

#include "MapUtil.h"
#include <assert.h>

class IPhysicsWorld 
{
public:
	IPhysicsWorld()
	{
		assert(ms_pWorld == NULL);
		ms_pWorld = this;
	}

	virtual ~IPhysicsWorld()
	{
		if (this == ms_pWorld) 
			ms_pWorld = NULL;
	}

	static IPhysicsWorld* GetPhysicsWorld()
	{
		return ms_pWorld;
	}

	virtual bool isPhysicalCollision(const D3DXVECTOR3 & c_rvCheckPosition) = 0;

private:
	static IPhysicsWorld* ms_pWorld;
};

class CActorInstance;

class IObjectManager 
{
public:
	IObjectManager()
	{
		assert(ms_ObjManager == NULL);
		ms_ObjManager = this;
	}

	virtual ~IObjectManager()
	{
		if (this == ms_ObjManager) 
			ms_ObjManager = NULL;
	}

	static IObjectManager* GetObjectManager()
	{
		return ms_ObjManager;
	}

	virtual void AdjustCollisionWithOtherObjects(CActorInstance* pInst) = 0;

private:
	static IObjectManager* ms_ObjManager;
};

class CPhysicsObject
{
public:
	CPhysicsObject();
	virtual ~CPhysicsObject();

	void Initialize();
	void Update(float fElapsedTime);

	bool isBlending() const;

	void SetDirection(const D3DXVECTOR3 & c_rv3Direction);
	void IncreaseExternalForce(const D3DXVECTOR3 & c_rvBasePosition, float fForce);

	// New SetLastPosition (current + delta)
	void SetLastPosition(const TPixelPosition & c_rPosition, const TPixelPosition & c_rDeltaPosition, float fBlendingTime);
	
	void GetFinalPosition(TPixelPosition* pPosition) const;
	void GetDeltaPosition(TPixelPosition* pPosition) const;
	

	float GetXMovement() const;
	float GetYMovement() const;

	float GetRemainingTime() const;

	void SetActorInstance(CActorInstance* pInst) { m_pActorInstance = pInst; }
	CActorInstance* GetActorInstance() { return m_pActorInstance; }

protected:
	void Accumulate(D3DXVECTOR3* pv3Position);

protected:
	float m_fMass;
	float m_fFriction;

	D3DXVECTOR3 m_v3Direction;
	D3DXVECTOR3 m_v3Acceleration;
	D3DXVECTOR3 m_v3Velocity;

	// Legacy last position (delta movement)
	D3DXVECTOR3 m_v3LastPosition;

	// NEW: store final position and delta separately
	D3DXVECTOR3 m_v3FinalPosition;
	D3DXVECTOR3 m_v3DeltaPosition;

	CEaseOutInterpolation m_xPushingPosition;
	CEaseOutInterpolation m_yPushingPosition;

	CActorInstance* m_pActorInstance;
};