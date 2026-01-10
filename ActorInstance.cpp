#include "StdAfx.h"
#include "ActorInstance.h"
#include "AreaTerrain.h"
#include "RaceData.h"
#include "../SpeedTreeLib/SpeedTreeForestDirectX8.h"
#include "../SpeedTreeLib/SpeedTreeWrapper.h"
#include "BlendingPosition.h"

enum
{
	MAIN_RACE_MAX_NUM = 8,
};

void CActorInstance::INSTANCEBASE_Deform()
{
	Deform();
	TraceProcess();
}

void CActorInstance::INSTANCEBASE_Transform()
{
	if (m_pkHorse)
	{
		m_pkHorse->INSTANCEBASE_Transform();

		m_x = m_pkHorse->NEW_GetCurPixelPositionRef().x;
		m_y = -m_pkHorse->NEW_GetCurPixelPositionRef().y;
		m_z = m_pkHorse->NEW_GetCurPixelPositionRef().z;
		m_bNeedUpdateCollision = TRUE;			
	}
	
	//DWORD t2=ELTimer_GetMSec();
	Update();
	//DWORD t3=ELTimer_GetMSec();
	TransformProcess();
	//DWORD t4=ELTimer_GetMSec();
	Transform();
	//DWORD t5=ELTimer_GetMSec();
	UpdatePointInstance();
	//DWORD t6=ELTimer_GetMSec();
	ShakeProcess();
	//DWORD t7=ELTimer_GetMSec();
	UpdateBoundingSphere();
	//DWORD t8=ELTimer_GetMSec();
	UpdateAttribute();
}

/*
void CActorInstance::TEMP_Update()
{
	//DWORD t1=ELTimer_GetMSec();
	OnUpdate();
	//DWORD t2=ELTimer_GetMSec();
	UpdateBoundingSphere();
	//DWORD t3=ELTimer_GetMSec();

#ifdef __PERFORMANCE_CHECKER__
	{
		static FILE* fp=fopen("perf_actor_update.txt", "w");

		if (t3-t1>3)
		{
			fprintf(fp, "AIU.Total %d (Time %f)\n", 
				t3-t1, ELTimer_GetMSec()/1000.0f);
			fprintf(fp, "AIU.UP %d\n", t2-t1);
			fprintf(fp, "AIU.UBS %d\n", t3-t2);
			fprintf(fp, "-------------------------------- \n");
			fflush(fp);
		}			
		fflush(fp);
	}
#endif
}
*/

void CActorInstance::OnUpdate()
{
#ifdef __PERFORMANCE_CHECKER__
	DWORD t1=ELTimer_GetMSec();
#endif
	if (!IsParalysis())
		CGraphicThingInstance::OnUpdate();
#ifdef __PERFORMANCE_CHECKER__
	DWORD t2=ELTimer_GetMSec();
#endif

	UpdateAttachingInstances();

#ifdef __PERFORMANCE_CHECKER__
	DWORD t3=ELTimer_GetMSec();
#endif

	__BlendAlpha_Update();

#ifdef __PERFORMANCE_CHECKER__
	DWORD t4=ELTimer_GetMSec();
	{
		static FILE* fp=fopen("perf_actor_update2.txt", "w");

		if (t4-t1>3)
		{
			fprintf(fp, "AIU2.Total %d (Time %f)\n", 
				t4-t1, ELTimer_GetMSec()/1000.0f);
			fprintf(fp, "AIU2.GU %d\n", t2-t1);
			fprintf(fp, "AIU2.UAI %d\n", t3-t2);
			fprintf(fp, "AIU2.BAU %d\n", t4-t3);
			fprintf(fp, "-------------------------------- \n");
			fflush(fp);
		}
		fflush(fp);
	}
#endif
}


// 2004.07.05.myevan. �ý�ź�� �ʿ� ���̴� �����ذ�
IBackground& CActorInstance::GetBackground()
{
	return IBackground::Instance();
}


void CActorInstance::SetMainInstance()
{
	m_isMain=true;
}

void CActorInstance::SetParalysis(bool isParalysis)
{
	m_isParalysis=isParalysis;
}

void CActorInstance::SetFaint(bool isFaint)
{
	m_isFaint=isFaint;
}

void CActorInstance::SetSleep(bool isSleep)
{
	m_isSleep=isSleep;

	Stop();
}

void CActorInstance::SetResistFallen(bool isResistFallen)
{
	m_isResistFallen=isResistFallen;
}

void CActorInstance::SetReachScale(float fScale)
{
	// Add validation for reach scale parameter
	if (fScale < 0.0f)
	{
		TraceError("CActorInstance::SetReachScale - Invalid reach scale: %f (negative), using 0.0f", fScale);
		fScale = 0.0f;
	}
	if (!isfinite(fScale))
	{
		TraceError("CActorInstance::SetReachScale - Invalid reach scale: %f (not finite), using 1.0f", fScale);
		fScale = 1.0f;
	}

	m_fReachScale=fScale;
}

float CActorInstance::__GetReachScale()
{
	return m_fReachScale;
}

float CActorInstance::__GetAttackSpeed()
{
	return m_fAtkSpd;
}

WORD CActorInstance::__GetCurrentComboType()
{
	if (IsBowMode())
		return 0;
	if (IsHandMode())
		return 0;
	if (__IsMountingHorse())
		return 0;

	return m_wcurComboType;
}

void CActorInstance::SetComboType(WORD wComboType)
{
	m_wcurComboType = wComboType;
}

void CActorInstance::SetAttackSpeed(float fAtkSpd)
{
	// Add validation for attack speed parameter
	if (fAtkSpd < 0.0f)
	{
		TraceError("CActorInstance::SetAttackSpeed - Invalid attack speed: %f (negative), using 0.0f", fAtkSpd);
		fAtkSpd = 0.0f;
	}
	if (!isfinite(fAtkSpd))
	{
		TraceError("CActorInstance::SetAttackSpeed - Invalid attack speed: %f (not finite), using 1.0f", fAtkSpd);
		fAtkSpd = 1.0f;
	}

	m_fAtkSpd=fAtkSpd;
}

void CActorInstance::SetMoveSpeed(float fMovSpd)
{
	// Add validation for move speed parameter
	if (fMovSpd < 0.0f)
	{
		TraceError("CActorInstance::SetMoveSpeed - Invalid move speed: %f (negative), using 0.0f", fMovSpd);
		fMovSpd = 0.0f;
	}
	if (!isfinite(fMovSpd))
	{
		TraceError("CActorInstance::SetMoveSpeed - Invalid move speed: %f (not finite), using 1.0f", fMovSpd);
		fMovSpd = 1.0f;
	}

	if (m_fMovSpd==fMovSpd)
		return;

	m_fMovSpd=fMovSpd;

	if (__IsMoveMotion())
	{
		Stop();
		Move();
	}
}

void CActorInstance::SetFishingPosition(D3DXVECTOR3 & rv3Position)
{
	m_v3FishingPosition = rv3Position;
}

// ActorInstanceMotion.cpp �� �ֵ��� ����
void  CActorInstance::Move()
{
	if (m_isWalking)
	{
		SetLoopMotion(CRaceMotionData::NAME_WALK, 0.15f, m_fMovSpd);
	}
	else
	{
		SetLoopMotion(CRaceMotionData::NAME_RUN, 0.15f, m_fMovSpd);
	}
}

void  CActorInstance::Stop(float fBlendingTime)
{
	__ClearMotion();
	SetLoopMotion(CRaceMotionData::NAME_WAIT, fBlendingTime);
}

void CActorInstance::SetOwner(DWORD dwOwnerVID)
{
	m_fOwnerBaseTime=GetLocalTime();
	m_dwOwnerVID=dwOwnerVID;
}

void CActorInstance::SetActorType(UINT eType)
{
	m_eActorType=eType;
}

UINT CActorInstance::GetActorType() const
{
	return m_eActorType;
}

bool CActorInstance::IsHandMode()
{
	if (CRaceMotionData::MODE_GENERAL==GetMotionMode())
		return true;

	if (CRaceMotionData::MODE_HORSE==GetMotionMode())
		return true;

	return false;
}

bool CActorInstance::IsTwoHandMode()
{
	if (CRaceMotionData::MODE_TWOHAND_SWORD==GetMotionMode())
		return true;

	return false;
}

bool CActorInstance::IsBowMode()
{
	if (CRaceMotionData::MODE_BOW==GetMotionMode())
		return true;

	if (CRaceMotionData::MODE_HORSE_BOW==GetMotionMode())
		return true;

	return false;
}

bool CActorInstance::IsPoly()
{
	if (TYPE_POLY==m_eActorType)
		return true;

	if (TYPE_PC==m_eActorType)
		if (m_eRace >= MAIN_RACE_MAX_NUM)
			return TRUE;

	return false;
}

bool CActorInstance::IsPC()
{
	if (TYPE_PC==m_eActorType)
		return true;

	return false;
}

bool CActorInstance::IsNPC()
{
	if (TYPE_NPC==m_eActorType)
		return true;

	return false;
}

bool CActorInstance::IsEnemy()
{
	if (TYPE_ENEMY==m_eActorType)
		return true;

	return false;
}

bool CActorInstance::IsStone()
{
	if (TYPE_STONE==m_eActorType)
		return true;

	return false;
}

bool CActorInstance::IsBoss()
{
	if (TYPE_BOSS==m_eActorType)
		return true;

	return false;
}

bool CActorInstance::IsWarp()
{
	if (TYPE_WARP==m_eActorType)
		return true;

	return false;
}

bool CActorInstance::IsGoto()
{
	if (TYPE_GOTO==m_eActorType)
		return true;

	return false;
}

bool CActorInstance::IsBuilding()
{
	if (TYPE_BUILDING==m_eActorType)
		return true;

	return false;	
}

bool CActorInstance::IsDoor()
{
	if (TYPE_DOOR==m_eActorType)
		return true;

	return false;
}

bool CActorInstance::IsObject()
{
	if (TYPE_OBJECT==m_eActorType)
		return true;

	return false;
}

void CActorInstance::DestroySystem()
{
}

void CActorInstance::DieEnd()
{
	Die();

	CGraphicThingInstance::SetMotionAtEnd();
}

void CActorInstance::Die()
{
	if (m_isRealDead)
		return;

	if (__IsMoveMotion())
		Stop();

	SetAdvancingRotation(GetRotation());

	if (IsStone())
	{
		InterceptOnceMotion(CRaceMotionData::NAME_DEAD);
	}
	else
	{
		if (!__IsDieMotion())
		{
			InterceptOnceMotion(CRaceMotionData::NAME_DEAD);
		}
	}

	m_isRealDead = TRUE;
}

BOOL CActorInstance::IsSleep()
{
	return m_isSleep;
}

BOOL CActorInstance::IsParalysis()
{
	return m_isParalysis;
}

BOOL CActorInstance::IsFaint()
{
	return m_isFaint;
}

BOOL CActorInstance::IsResistFallen()
{
	return m_isResistFallen;
}

BOOL CActorInstance::IsMoving()
{
	return __IsMoveMotion();
}

BOOL CActorInstance::IsWaiting()
{
	return __IsWaitMotion();
}

BOOL CActorInstance::IsDead()
{
	return m_isRealDead;
}

BOOL CActorInstance::IsKnockDown()
{
	return __IsKnockDownMotion();
}

BOOL CActorInstance::IsDamage()
{
	return __IsDamageMotion();
}

BOOL CActorInstance::IsAttacked()
{
	if (IsPushing())
		return TRUE;

	if (__IsDamageMotion())
		return TRUE;

	if (__IsKnockDownMotion())
		return TRUE;

	if (__IsDieMotion())
		return TRUE;

	return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Process
void CActorInstance::PhysicsProcess()
{
	m_PhysicsObject.Update(m_fSecondElapsed);
	AddMovement(m_PhysicsObject.GetXMovement(), m_PhysicsObject.GetYMovement(), 0.0f);
}

void CActorInstance::__AccumulationMovement(float fRot)
{
	// NOTE - �ϴ��� WAIT�� �̲����� ����
	//        ���Ŀ��� RaceMotionData�� �̵��Ǵ� ��������� ���� Flag�� ���� �ְԲ� �Ѵ�. - [levites]
	if (CRaceMotionData::NAME_WAIT == __GetCurrentMotionIndex())
		return;

	D3DXMATRIX s_matRotationZ;
	D3DXMatrixRotationZ(&s_matRotationZ, D3DXToRadian(fRot));
	UpdateTransform(&s_matRotationZ, GetAverageSecondElapsed());

	AddMovement(s_matRotationZ._41, s_matRotationZ._42, s_matRotationZ._43);
}

void CActorInstance::AccumulationMovement()
{
	if (m_pkTree)
		return;

	if (m_pkHorse)
	{
		m_pkHorse->__AccumulationMovement(m_fcurRotation);
		return;
	}

	__AccumulationMovement(m_fAdvancingRotation);
}

void CActorInstance::TransformProcess()
{
	if (!IsParalysis())
	{
		m_x += m_v3Movement.x;
		m_y += m_v3Movement.y;
		m_z += m_v3Movement.z;
	}

	__InitializeMovement();

	SetPosition(m_x, m_y, m_z);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// Process

void CActorInstance::OnUpdateCollisionData(const CStaticCollisionDataVector * pscdVector)
{
	assert(pscdVector);
	CStaticCollisionDataVector::const_iterator it;
	for(it = pscdVector->begin();it!=pscdVector->end();++it)
	{
		const CStaticCollisionData & c_rColliData = *it;
		const D3DXMATRIX & c_rMatrix = GetTransform();
		AddCollision(&c_rColliData, &c_rMatrix);
	}
}

void CActorInstance::OnUpdateHeighInstance(CAttributeInstance * pAttributeInstance)
{
	assert(pAttributeInstance);
	SetHeightInstance(pAttributeInstance);
}

bool CActorInstance::OnGetObjectHeight(float fX, float fY, float * pfHeight)
{
	if (!m_pHeightAttributeInstance)
		return false;

	if (TYPE_BUILDING != GetType())
		return false;

	return m_pHeightAttributeInstance->GetHeight(fX, fY, pfHeight) == 1 ? true : false;
}

//////////////////////////////////////////////////////////////////
// Battle
void CActorInstance::Revive()
{
	m_isSleep = FALSE;
	m_isParalysis = FALSE;
	m_isFaint = FALSE;
	m_isRealDead = FALSE;
	m_isStun = FALSE;
	m_isWalking = FALSE;
	m_isMain = FALSE;
	m_isResistFallen = FALSE;

	__InitializeCollisionData();
}

BOOL CActorInstance::IsStun()
{
	return m_isStun;
}

void CActorInstance::Stun()
{
	m_isStun = TRUE;
}

void CActorInstance::SetWalkMode()
{
	m_isWalking = TRUE;
	if (CRaceMotionData::NAME_RUN == GET_MOTION_INDEX(m_kCurMotNode.dwMotionKey))
		SetLoopMotion(CRaceMotionData::NAME_WALK, 0.15f, m_fMovSpd);
}

void CActorInstance::SetRunMode()
{
	m_isWalking = FALSE;
	if (CRaceMotionData::NAME_WALK == GET_MOTION_INDEX(m_kCurMotNode.dwMotionKey))
		SetLoopMotion(CRaceMotionData::NAME_RUN, 0.15f, m_fMovSpd);
}

MOTION_KEY CActorInstance::GetNormalAttackIndex()
{
	WORD wMotionIndex;
	m_pkCurRaceData->GetNormalAttackIndex(GetMotionMode(), &wMotionIndex);

	return MAKE_MOTION_KEY(GetMotionMode(), wMotionIndex);
}

//////////////////////////////////////////////////////////////////
// Movement
void CActorInstance::__InitializeMovement()
{
	m_v3Movement.x = 0.0f;
	m_v3Movement.y = 0.0f;
	m_v3Movement.z = 0.0f;
}

void CActorInstance::AddMovement(float fx, float fy, float fz)
{
	m_v3Movement.x += fx;
	m_v3Movement.y += fy;
	m_v3Movement.z += fz;
}

const float gc_fActorSlideMoveSpeed = 5.0f;

void CActorInstance::AdjustDynamicCollisionMovement(const CActorInstance * c_pActorInstance)
{
	// Add null pointer check for actor instance
	if (!c_pActorInstance)
	{
		TraceError("CActorInstance::AdjustDynamicCollisionMovement - Null actor instance provided");
		return;
	}

	// Log self-collision but allow it (might be intentional in some cases)
	if (c_pActorInstance == this)
	{
		Tracenf("CActorInstance::AdjustDynamicCollisionMovement - Self-collision check for VID: %d", GetVirtualID());
		// Don't return - allow the collision check to proceed
	}

	if (m_pkHorse)
	{
		m_pkHorse->AdjustDynamicCollisionMovement(c_pActorInstance);
		return;
	}

	// NOTE : ������ Sphere Overlap������� ó���� ���⸦ �ϸ��� Penetration�� ������ ���Ƽ� ( �����ε� ���԰� --)
	// Sphere�� Collision�� ������ ��� ������ġ�� RollBack�ϴ� ������� �ٲ��.
	// �� BGObject�� ���ؼ���.

	if (isAttacking() )
		return;
	
	UINT uActorType = c_pActorInstance->GetActorType();
	if( (uActorType == TYPE_BUILDING) || (uActorType == TYPE_OBJECT) || (uActorType == TYPE_DOOR) || (uActorType == TYPE_STONE))
	{
		BlockMovement();

		//Movement�ʱ�ȭ
	/*	m_v3Movement = D3DXVECTOR3(0.f,0.f,0.f);

		TCollisionPointInstanceListIterator itMain = m_BodyPointInstanceList.begin();
		for (; itMain != m_BodyPointInstanceList.end(); ++itMain)
		{
			CDynamicSphereInstanceVector & c_rMainSphereVector = (*itMain).SphereInstanceVector;
			for (DWORD i = 0; i < c_rMainSphereVector.size(); ++i)
			{
				CDynamicSphereInstance & c_rMainSphere = c_rMainSphereVector[i];
				c_rMainSphere.v3Position =c_rMainSphere.v3LastPosition;
			}
		}*/
	}
	else
	{

		float move_length = D3DXVec3Length(&m_v3Movement);
		if (move_length>gc_fActorSlideMoveSpeed)
			m_v3Movement*=gc_fActorSlideMoveSpeed/move_length;

		TCollisionPointInstanceListIterator itMain = m_BodyPointInstanceList.begin();
		for (; itMain != m_BodyPointInstanceList.end(); ++itMain)
		{
			CDynamicSphereInstanceVector & c_rMainSphereVector = (*itMain).SphereInstanceVector;
			for (DWORD i = 0; i < c_rMainSphereVector.size(); ++i)
			{
				CDynamicSphereInstance & c_rMainSphere = c_rMainSphereVector[i];

				TCollisionPointInstanceList::const_iterator itOpp = c_pActorInstance->m_BodyPointInstanceList.begin();
				for(;itOpp != c_pActorInstance->m_BodyPointInstanceList.end();++itOpp)
				{
					CSphereCollisionInstance s;
					s.GetAttribute().fRadius=itOpp->SphereInstanceVector[0].fRadius;
					s.GetAttribute().v3Position=itOpp->SphereInstanceVector[0].v3Position;
					D3DXVECTOR3 v3Delta = s.GetCollisionMovementAdjust(c_rMainSphere);
					m_v3Movement+=v3Delta;
					c_rMainSphere.v3Position+=v3Delta;

					if (v3Delta.x !=0.0f || v3Delta.y !=0.0f || v3Delta.z !=0.0f )
					{
						move_length = D3DXVec3Length(&m_v3Movement);
						if (move_length>gc_fActorSlideMoveSpeed)
						{
							m_v3Movement*=gc_fActorSlideMoveSpeed/move_length;
							c_rMainSphere.v3Position = c_rMainSphere.v3LastPosition;
							c_rMainSphere.v3Position+=m_v3Movement;
						}
					}
				}
			}
		}
	}
}


void CActorInstance::__AdjustCollisionMovement(const CGraphicObjectInstance * c_pGraphicObjectInstance)
{
	// Add null pointer check for graphic object instance
	if (!c_pGraphicObjectInstance)
	{
		TraceError("CActorInstance::__AdjustCollisionMovement - Null graphic object instance provided");
		return;
	}

	if (m_pkHorse)
	{
		m_pkHorse->__AdjustCollisionMovement(c_pGraphicObjectInstance);
		return;
	}

	// Body�� �ϳ����� �����մϴ�.

	if (m_v3Movement.x == 0.0f && m_v3Movement.y == 0.0f && m_v3Movement.z == 0.0f) 
		return;

	float move_length = D3DXVec3Length(&m_v3Movement);
	if (move_length>gc_fActorSlideMoveSpeed)
		m_v3Movement*=gc_fActorSlideMoveSpeed/move_length;

	TCollisionPointInstanceListIterator itMain = m_BodyPointInstanceList.begin();
	for (; itMain != m_BodyPointInstanceList.end(); ++itMain)
	{
		CDynamicSphereInstanceVector & c_rMainSphereVector = (*itMain).SphereInstanceVector;
		for (DWORD i = 0; i < c_rMainSphereVector.size(); ++i)
		{
			CDynamicSphereInstance & c_rMainSphere = c_rMainSphereVector[i];

			D3DXVECTOR3 v3Delta = c_pGraphicObjectInstance->GetCollisionMovementAdjust(c_rMainSphere);
			m_v3Movement+=v3Delta;
			c_rMainSphere.v3Position+=v3Delta;

			if (v3Delta.x !=0.0f || v3Delta.y !=0.0f || v3Delta.z !=0.0f )
			{
				move_length = D3DXVec3Length(&m_v3Movement);
				if (move_length>gc_fActorSlideMoveSpeed)
				{
					m_v3Movement*=gc_fActorSlideMoveSpeed/move_length;
					c_rMainSphere.v3Position = c_rMainSphere.v3LastPosition;
					c_rMainSphere.v3Position+=m_v3Movement;
				}
			}

			/*if (c_pObjectInstance->CollisionDynamicSphere(c_rMainSphere))
			{
				const D3DXVECTOR3 & c_rv3Position = c_pObjectInstance->GetPosition();
				//if (GetVector3Distance(c_rMainSphere.v3Position, c_rv3Position) <
				//	GetVector3Distance(c_rMainSphere.v3LastPosition, c_rv3Position))
				{
					return TRUE;
				}

				return FALSE;
			}*/
		}
	}
}

BOOL CActorInstance::IsMovement()
{
	if (m_pkHorse)
		if (m_pkHorse->IsMovement())
			return TRUE;
		
	if (0.0f != m_v3Movement.x)
		return TRUE;
	if (0.0f != m_v3Movement.y)
		return TRUE;
	if (0.0f != m_v3Movement.z)
		return TRUE;

	return FALSE;
}

float CActorInstance::GetHeight()
{
	return CGraphicThingInstance::GetHeight();
}

bool CActorInstance::IntersectDefendingSphere()
{
	for (TCollisionPointInstanceList::iterator it = m_DefendingPointInstanceList.begin(); it != m_DefendingPointInstanceList.end(); ++it)
	{
		CDynamicSphereInstanceVector & rSphereInstanceVector = (*it).SphereInstanceVector;

		CDynamicSphereInstanceVector::iterator it2 = rSphereInstanceVector.begin();
		for (; it2 != rSphereInstanceVector.end(); ++it2)
		{
			CDynamicSphereInstance & rInstance = *it2;
			D3DXVECTOR3 v3SpherePosition = rInstance.v3Position;
			float fRadius = rInstance.fRadius;

			D3DXVECTOR3 v3Orig;
			D3DXVECTOR3 v3Dir;
			float fRange;
			ms_Ray.GetStartPoint(&v3Orig);
			ms_Ray.GetDirection(&v3Dir, &fRange);

			D3DXVECTOR3 v3Distance = v3Orig - v3SpherePosition;
			float b = D3DXVec3Dot(&v3Dir, &v3Distance);
			float c = D3DXVec3Dot(&v3Distance, &v3Distance) - fRadius*fRadius;

			if (b*b - c >= 0)
				return true;
		}
	}
	return false;
}

bool CActorInstance::__IsMountingHorse()
{
	return NULL != m_pkHorse;
}

void CActorInstance::MountHorse(CActorInstance * pkHorse)
{
	// Add validation for horse instance
	if (pkHorse && pkHorse == this)
	{
		TraceError("CActorInstance::MountHorse - Cannot mount self as horse (VID: %d)", GetVirtualID());
		return;
	}

	m_pkHorse = pkHorse;

	if (m_pkHorse)
	{
		m_pkHorse->SetCurPixelPosition(NEW_GetCurPixelPositionRef());
		m_pkHorse->SetRotation(GetRotation());
		m_pkHorse->SetAdvancingRotation(GetRotation());
	}
}

void CActorInstance::__CreateTree(const char * c_szFileName)
{
	__DestroyTree();

	CSpeedTreeForestDirectX8& rkForest=CSpeedTreeForestDirectX8::Instance();
	m_pkTree=rkForest.CreateInstance(m_x, m_y, m_z, GetCaseCRC32(c_szFileName, strlen(c_szFileName)), c_szFileName);
	m_pkTree->SetPosition(m_x, m_y, m_z);
	m_pkTree->UpdateBoundingSphere();
	m_pkTree->UpdateCollisionData();
}

void CActorInstance::__DestroyTree()
{
	if (!m_pkTree)
		return;

	CSpeedTreeForestDirectX8::Instance().DeleteInstance(m_pkTree);
}

void CActorInstance::__SetTreePosition(float fx, float fy, float fz)
{
	if (!m_pkTree)
		return;
	if (m_x == fx && m_y == fy && m_z == fz)
		return;

	m_pkTree->SetPosition(fx, fy, fz);
	m_pkTree->UpdateBoundingSphere();
	m_pkTree->UpdateCollisionData();
}

void CActorInstance::ClearAttachingEffect()
{
	__ClearAttachingEffect();
}

void CActorInstance::Destroy()
{
	ClearFlyTargeter();

	m_HitDataMap.clear();
	m_MotionDeque.clear();

	if (m_pAttributeInstance)
	{
		m_pAttributeInstance->Clear();
		CAttributeInstance::Delete(m_pAttributeInstance);
		m_pAttributeInstance = NULL;
	}

	__ClearAttachingEffect();

	CGraphicThingInstance::Clear();

	__DestroyWeaponTrace();
	__DestroyTree();
	//m_PhysicsObject.SetActorInstance(NULL);


	__Initialize();
}

void CActorInstance::__InitializeRotationData()
{
	m_fAtkDirRot = 0.0f;
	m_fcurRotation = 0.0f;
	m_rotBegin = 0.0f;
	m_rotEnd = 0.0f;
	m_rotEndTime = 0.0f;
	m_rotBeginTime = 0.0f;
	m_rotBlendTime = 0.0f;
	m_fAdvancingRotation = 0.0f;
	m_rotX = 0.0f;
	m_rotY = 0.0f;
}


void CActorInstance::__InitializeStateData()
{
	m_bEffectInitialized = false;

	m_isPreInput = FALSE;
	m_isNextPreInput = FALSE;

	m_isSleep = FALSE;
	m_isParalysis = FALSE;
	m_isFaint = FALSE;
	m_isRealDead = FALSE;
	m_isWalking = FALSE;
	m_isMain = FALSE;
	m_isStun = FALSE;
	m_isHiding = FALSE;
	m_isResistFallen = FALSE;

	m_iRenderMode = RENDER_MODE_NORMAL;
	m_fAlphaValue = 0.0f;
	m_AddColor = D3DXCOLOR(0.0f, 0.0f, 0.0f, 1.0f);
	
	m_dwMtrlColor=0xffffffff;
	m_dwMtrlAlpha=0xff000000;

	m_dwBattleHitEffectID = 0;
	m_dwBattleAttachEffectID = 0;
}

void CActorInstance::__InitializeMotionData()
{	
	m_wcurMotionMode = CRaceMotionData::MODE_GENERAL;
	m_wcurComboType = 0;

	m_fReachScale=1.0f;
	m_fMovSpd=1.0f;
	m_fAtkSpd=1.0f;

	m_fInvisibleTime = 0.0f;

	m_kSplashArea.isEnableHitProcess=TRUE;
	m_kSplashArea.uSkill=0;
	m_kSplashArea.MotionKey=0;
	m_kSplashArea.fDisappearingTime = 0.0f;
	m_kSplashArea.SphereInstanceVector.clear();
	m_kSplashArea.HittedInstanceMap.clear();

	memset(&m_kCurMotNode, 0, sizeof(m_kCurMotNode));

	__ClearCombo();
}

void CActorInstance::__Initialize()
{
	m_pkCurRaceMotionData=NULL;
	m_pkCurRaceData=NULL;
	m_pkHorse=NULL;
	m_pkTree=NULL;

	m_fOwnerBaseTime=0.0f;

	m_eActorType = TYPE_PC;
	m_eRace = 0;

	m_eShape = 0;
	m_eHair = 0;

	m_dwSelfVID = 0;
	m_dwOwnerVID = 0;

	m_pkEventHandler =  NULL;

	m_PhysicsObject.Initialize();

	m_pAttributeInstance = NULL;

	m_pFlyEventHandler = 0;

	m_v3FishingPosition = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
	m_iFishingEffectID = -1;

	m_pkHorse = NULL;

	__InitializePositionData();
	__InitializeRotationData();
	__InitializeMotionData();
	__InitializeStateData();
	__InitializeCollisionData();

	__BlendAlpha_Initialize();

	ClearFlyTargeter();	
}

CActorInstance::CActorInstance()
{
	m_bPositionBlending = false;
	__Initialize();
	m_PhysicsObject.SetActorInstance(this);
}

CActorInstance::~CActorInstance()
{
	Destroy();
}

void CActorInstance::GetBlendingPosition(TPixelPosition * pPosition)
{
    TPixelPosition kCurrentPos;
    GetPixelPosition(&kCurrentPos);

    if (m_PhysicsObject.isBlending())
    {
        m_PhysicsObject.GetFinalPosition(pPosition);
        TraceError("[ACTOR_GET_BLEND] VID:%d Current:(%.2f,%.2f,%.2f) Final:(%.2f,%.2f,%.2f) [BLENDING]",
            GetVirtualID(), kCurrentPos.x, kCurrentPos.y, kCurrentPos.z,
            pPosition->x, pPosition->y, pPosition->z);
    }
    else
    {
        GetPixelPosition(pPosition);
        TraceError("[ACTOR_GET_BLEND] VID:%d Position:(%.2f,%.2f,%.2f) [NOT_BLENDING]",
            GetVirtualID(), pPosition->x, pPosition->y, pPosition->z);
    }
}

bool CActorInstance::IsPositionBlending() const
{
    return m_PhysicsObject.isBlending();
}

void CActorInstance::__Push(const TPixelPosition& c_rkPPosDst, unsigned int unDuration)
{
    DWORD dwVID = GetVirtualID();

    if (unDuration == 0)
        unDuration = 1000;

    const D3DXVECTOR3& c_rv3Src = GetPosition();
    const D3DXVECTOR3 c_v3Delta = c_rkPPosDst - c_rv3Src;

    TraceError("[ACTOR_PUSH] VID:%d Source:(%.2f,%.2f,%.2f) Dest:(%.2f,%.2f,%.2f) Delta:(%.2f,%.2f,%.2f) Duration:%dms",
        dwVID,
        c_rv3Src.x, c_rv3Src.y, c_rv3Src.z,
        c_rkPPosDst.x, c_rkPPosDst.y, c_rkPPosDst.z,
        c_v3Delta.x, c_v3Delta.y, c_v3Delta.z,
        unDuration);

    SetBlendingPosition(c_rkPPosDst, float(unDuration) / 1000);

    if (!IsUsingSkill() && !IsResistFallen())
    {
        int len = sqrt(c_v3Delta.x * c_v3Delta.x + c_v3Delta.y * c_v3Delta.y);
        if (len > 150.0f)
        {
            TraceError("[ACTOR_PUSH] VID:%d Applying DAMAGE_FLYING animation (push distance:%.2f)", dwVID, (float)len);
            InterceptOnceMotion(CRaceMotionData::NAME_DAMAGE_FLYING);
            PushOnceMotion(CRaceMotionData::NAME_STAND_UP);
        }
    }
}

void CActorInstance::ClientAttack(DWORD dwVID)
{
    if (m_mapAttackSync.find(dwVID) == m_mapAttackSync.end()) {
        m_mapAttackSync.insert(std::make_pair(dwVID, -1));
    }
    else
    {
        if (m_mapAttackSync[dwVID] == 1)
        {
            m_mapAttackSync.erase(dwVID);
            return;
        }
        m_mapAttackSync[dwVID]--;
    }
}

void CActorInstance::ServerAttack(DWORD dwVID)
{
    if (m_mapAttackSync.find(dwVID) == m_mapAttackSync.end()) {
        m_mapAttackSync.insert(std::make_pair(dwVID, 1));
    }
    else
    {
        if (m_mapAttackSync[dwVID] == -1)
        {
            m_mapAttackSync.erase(dwVID);
            return;
        }
        m_mapAttackSync[dwVID]++;
    }
}

bool CActorInstance::ProcessingClientAttack(DWORD dwVID)
{
    return m_mapAttackSync.find(dwVID) != m_mapAttackSync.end() && m_mapAttackSync[dwVID] < 0;
}

//
bool CActorInstance::ServerAttackCameFirst(DWORD dwVID)
{
    return m_mapAttackSync.find(dwVID) != m_mapAttackSync.end() && m_mapAttackSync[dwVID] > 0;
}

void CActorInstance::StartPositionBlend(
    const BlendingPosition& sBlending,
    DWORD dwDuration,
    DWORD dwServerTime)
{
    m_bPositionBlending = true;

    m_fBlendStartX = sBlending.source.x;
    m_fBlendStartY = sBlending.source.y;

    m_fBlendDestX = sBlending.dest.x;
    m_fBlendDestY = sBlending.dest.y;

    m_dwBlendStartTime = dwServerTime;
    m_dwBlendDuration = max(1u, dwDuration);
}

void CActorInstance::UpdatePositionBlend(DWORD dwCurrentTime)
{
    if (!m_bPositionBlending)
        return;

    DWORD elapsed = dwCurrentTime - m_dwBlendStartTime;

    float t = min(1.0f, (float)elapsed / (float)m_dwBlendDuration);

    float x = m_fBlendStartX + (m_fBlendDestX - m_fBlendStartX) * t;
    float y = m_fBlendStartY + (m_fBlendDestY - m_fBlendStartY) * t;

	SetPosition(x, y, m_z);

    if (t >= 1.0f)
        m_bPositionBlending = false;
}

float CActorInstance::__GetInvisibleTimeAdjust(UINT uiSkill, const NRaceData::TAttackData& c_rAttackData)
	{
		return 0.0f;  // Žádná úprava = plný invisible time z c_rAttackData
	}


void CActorInstance::__ProcessDataAttackSuccess(const NRaceData::TAttackData & c_rAttackData, CActorInstance & rVictim, const D3DXVECTOR3 & c_rv3Position, UINT uiSkill, BOOL isSendPacket)
{
    if (NRaceData::HIT_TYPE_NONE == c_rAttackData.iHittingType)
        return;

    InsertDelay(c_rAttackData.fStiffenTime);

    BlendingPosition sBlending;
    memset(&sBlending, 0, sizeof(sBlending));
    sBlending.source = rVictim.NEW_GetCurPixelPositionRef();

    // LOG: Blending source initialized
    TraceError("[HIT_BLEND_START] Attacker:%d Victim:%d Source:(%.2f,%.2f,%.2f)",
        GetVirtualID(), rVictim.GetVirtualID(),
        sBlending.source.x, sBlending.source.y, sBlending.source.z);

    if (__CanPushDestActor(rVictim) && c_rAttackData.fExternalForce > 0.0f)
    {
        const bool bServerAttackAlreadyCame = rVictim.ServerAttackCameFirst(GetVirtualID());
        rVictim.ClientAttack(GetVirtualID());
        TraceError("[HIT_PUSH_CHECK] Attacker:%d Victim:%d ServerFirst:%d CanPush:YES Force:%.2f",
            GetVirtualID(), rVictim.GetVirtualID(), bServerAttackAlreadyCame ? 1 : 0,
            c_rAttackData.fExternalForce);

        if (!bServerAttackAlreadyCame)
        {
            __PushCircle(rVictim);

            // VICTIM_COLLISION_TEST
            const D3DXVECTOR3& kVictimPos = rVictim.GetPosition();
            TraceError("[HIT_PUSH_APPLY] Victim:%d VictimPos:(%.2f,%.2f,%.2f) Force:%.2f",
                rVictim.GetVirtualID(), kVictimPos.x, kVictimPos.y, kVictimPos.z,
                c_rAttackData.fExternalForce);

            rVictim.m_PhysicsObject.IncreaseExternalForce(kVictimPos, c_rAttackData.fExternalForce);
            rVictim.GetBlendingPosition(&(sBlending.dest));
            sBlending.duration = rVictim.m_PhysicsObject.GetRemainingTime();

            // LOG: Blending dest calculated after push
            TraceError("[HIT_BLEND_DEST] Attacker:%d Victim:%d Dest:(%.2f,%.2f,%.2f) Duration:%.3fs",
                GetVirtualID(), rVictim.GetVirtualID(),
                sBlending.dest.x, sBlending.dest.y, sBlending.dest.z, sBlending.duration);
            // VICTIM_COLLISION_TEST_END
        }
        else
        {
            TraceError("[HIT_PUSH_SKIP] Victim:%d - Server attack came first, no push applied",
                rVictim.GetVirtualID());
        }
    }
    else
    {
        TraceError("[HIT_PUSH_CHECK] Attacker:%d Victim:%d CanPush:NO (CanPushDestActor:%d Force:%.2f)",
            GetVirtualID(), rVictim.GetVirtualID(),
            __CanPushDestActor(rVictim) ? 1 : 0, c_rAttackData.fExternalForce);
    }


    // Invisible Time
    rVictim.m_fInvisibleTime = CTimer::Instance().GetCurrentSecond() + (c_rAttackData.fInvisibleTime - __GetInvisibleTimeAdjust(uiSkill, c_rAttackData));

    // Stiffen Time
    rVictim.InsertDelay(c_rAttackData.fStiffenTime);

    // Hit Effect
    D3DXVECTOR3 vec3Effect(rVictim.m_x, rVictim.m_y, rVictim.m_z);
   
    // #0000780: [M2KR] ¼ö·æ Å¸°Ý±¸ ¹®Á¦
    extern bool IS_HUGE_RACE(unsigned int vnum);
    if (IS_HUGE_RACE(rVictim.GetRace()))
    {
        vec3Effect = c_rv3Position;
    }
   
    const D3DXVECTOR3 & v3Pos = GetPosition();

    float fHeight = D3DXToDegree(atan2(-vec3Effect.x + v3Pos.x,+vec3Effect.y - v3Pos.y));

    // 2004.08.03.myevan.ºôµùÀÌ³ª ¹®ÀÇ °æ¿ì Å¸°Ý È¿°ú°¡ º¸ÀÌÁö ¾Ê´Â´Ù
    if (rVictim.IsBuilding()||rVictim.IsDoor())
    {
        D3DXVECTOR3 vec3Delta=vec3Effect-v3Pos;
        D3DXVec3Normalize(&vec3Delta, &vec3Delta);
        vec3Delta*=30.0f;

        CEffectManager& rkEftMgr=CEffectManager::Instance();
        if (m_dwBattleHitEffectID)
            rkEftMgr.CreateEffect(m_dwBattleHitEffectID, v3Pos+vec3Delta, D3DXVECTOR3(0.0f, 0.0f, 0.0f));
    }
    else
    {
        // Create hit effects - skip for friendly fire on players
        if(c_rAttackData.isEnemy == 0)
        {
            // Skip effects for friendly fire, but DON'T return - we still need to call __OnHit!
        }
        else
        {
            CEffectManager& rkEftMgr=CEffectManager::Instance();
            if (m_dwBattleHitEffectID)
                rkEftMgr.CreateEffect(m_dwBattleHitEffectID, vec3Effect, D3DXVECTOR3(0.0f, 0.0f, fHeight));
            if (m_dwBattleAttachEffectID)
                rVictim.AttachEffectByID(0, NULL, m_dwBattleAttachEffectID);
        }
    }

    if (rVictim.IsBuilding())
    {
        // 2004.08.03.ºôµùÀÇ °æ¿ì Èçµé¸®¸é ÀÌ»óÇÏ´Ù
    }
    else if (rVictim.IsStone() || rVictim.IsDoor())
    {
        __HitStone(rVictim);
    }
    else
    {
        ///////////
        // Motion
        bool ForceHitGOOD = rVictim.IsPC() && (rVictim.IsKnockDown() || rVictim.__IsStandUpMotion());
        if (NRaceData::HIT_TYPE_GOOD == c_rAttackData.iHittingType || (TRUE == rVictim.IsResistFallen()))
        {
            __HitGood(rVictim);
        }
        else if (NRaceData::HIT_TYPE_GREAT == c_rAttackData.iHittingType)
        {
            if(c_rAttackData.isEnemy == 0)
            {  
                if(rVictim.IsEnemy() || rVictim.IsPC() || rVictim.IsBoss() || rVictim.IsStone())
                {
                    // Skip GREAT hit animation for PvP, but DON'T return - we need to call __OnHit!
                }
                else
                {
                    int isEnemy = rVictim.IsEnemy() ? 1 : 0;
					__HitGreate(rVictim, uiSkill, isEnemy);
                }
            }      
            else
            {
                int isEnemy = rVictim.IsEnemy() ? 1 : 0;
				__HitGreate(rVictim, uiSkill, isEnemy);
            }
        }
        else
        {
            TraceError("ProcessSucceedingAttacking: Unknown AttackingData.iHittingType %d", c_rAttackData.iHittingType);
        }
    }

    __OnHit(uiSkill, rVictim, isSendPacket, &sBlending);
}