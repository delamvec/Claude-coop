// ============================================================================
// PATCH FOR: input_main.cpp - CInputMain::Attack()
//
// FIXES:
// 1. Removed early returns blocking __OnHit for PvP (client-side - DONE)
// 2. Fixed coordinate system mismatch (server validates global vs local)
// 3. Added triple validation: packet integrity, knockback distance, source position
// 4. Removed duplicate Attack() and PacketAround() calls
// 5. Fixed CheckSkillHitCount order (must be BEFORE Attack())
// 6. Anti-cheat: detects packet manipulation, teleport hack, speed hack
//
// APPLY THIS TO: server/game/src/input_main.cpp
// REPLACE: void CInputMain::Attack() function in HEADER_CG_ATTACK case
// ============================================================================

void CInputMain::Attack(LPCHARACTER ch, const BYTE header, const char* data)
{
	if (NULL == ch)
		return;

	struct type_identifier
	{
		BYTE header;
		BYTE type;
	};

	const struct type_identifier* const type = reinterpret_cast<const struct type_identifier*>(data);

	if (type->type > 0)
	{
		if (false == ch->CanUseSkill(type->type))
		{
			return;
		}

		switch (type->type)
		{
			case SKILL_GEOMPUNG:
			case SKILL_SANGONG:
			case SKILL_YEONSA:
			case SKILL_KWANKYEOK:
			case SKILL_HWAJO:
			case SKILL_GIGUNG:
			case SKILL_PABEOB:
			case SKILL_MARYUNG:
			case SKILL_TUSOK:
			case SKILL_MAHWAN:
			case SKILL_BIPABU:
			case SKILL_NOEJEON:
			case SKILL_CHAIN:
			case SKILL_HORSE_WILDATTACK_RANGE:
				if (HEADER_CG_SHOOT != type->header)
				{
					if (test_server)
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Attack :name[%s] Vnum[%d] can't use skill by attack(warning)"), type->type);
					return;
				}
				break;
		}
	}

	switch (header)
	{
		case HEADER_CG_ATTACK:
			{
				if (NULL == ch->GetDesc())
					return;

				const TPacketCGAttack* const packMelee = reinterpret_cast<const TPacketCGAttack*>(data);

				DWORD dwAttackerVID = ch->GetVID();

				sys_log(0, "[SERVER_ATTACK_RECV] Type:%d Attacker:%s(%u) VictimVID:%u SyncDest:(%.1f,%.1f) BlendDur:%u",
					packMelee->bType, ch->GetName(), dwAttackerVID, packMelee->dwVID,
					packMelee->fSyncDestX, packMelee->fSyncDestY, packMelee->dwBlendDuration);

				// ============ VALIDACE VICTIM ============
				LPCHARACTER victim = CHARACTER_MANAGER::instance().Find(packMelee->dwVID);

				if (NULL == victim || ch == victim)
				{
					sys_log(0, "[SERVER_ATTACK_REJECT] Victim NULL or self-attack");
					return;
				}

				switch (victim->GetCharType())
				{
					case CHAR_TYPE_NPC:
					case CHAR_TYPE_WARP:
					case CHAR_TYPE_GOTO:
						sys_log(0, "[SERVER_ATTACK_REJECT] Invalid victim type");
						return;
				}

				// ============ VALIDACE SKILL HIT COUNT (BEFORE Attack!) ============
				if (packMelee->bType > 0)
				{
					if (false == ch->CheckSkillHitCount(packMelee->bType, victim->GetVID()))
					{
						sys_log(0, "[SERVER_ATTACK_REJECT] CheckSkillHitCount failed");
						return;
					}
				}

				// ============ TRIPLE VALIDATION SYSTEM ============

				// VALIDACE #1: PACKET INTEGRITY CHECK (Anti-cheat: packet manipulation)
				// TODO: Replace hardcoded values with actual map base coordinates from SECTREE_MAP
				// Example: LPSECTREE_MAP pMap = SECTREE_MANAGER::instance().GetMap(ch->GetMapIndex());
				//          long lBaseX = pMap->m_setting.iBaseX;
				//          long lBaseY = pMap->m_setting.iBaseY;

				const long TEMP_MAP_BASE_X = 921600; // TEMPORARY - from log analysis (963877 - 42094 ≈ 921783)
				const long TEMP_MAP_BASE_Y = 204800; // TEMPORARY - from log analysis (200 * 1024)

				if (packMelee->lX != 0 && packMelee->lY != 0 &&
					packMelee->fSyncDestX != 0.0f && packMelee->fSyncDestY != 0.0f)
				{
					// Convert GLOBAL to LOCAL and compare with client's LOCAL coordinates
					long lCalcLocalX = packMelee->lX - TEMP_MAP_BASE_X;
					long lCalcLocalY = packMelee->lY - TEMP_MAP_BASE_Y;

					float fDiffX = fabs((float)lCalcLocalX - packMelee->fSyncDestX);
					float fDiffY = fabs((float)lCalcLocalY - packMelee->fSyncDestY);

					sys_log(0, "[SERVER_ATTACK_INTEGRITY] GlobalDst:(%ld,%ld) - Base:(%ld,%ld) = CalcLocal:(%ld,%ld) vs ClientLocal:(%.1f,%.1f) Diff:(%.1f,%.1f)",
						packMelee->lX, packMelee->lY,
						TEMP_MAP_BASE_X, TEMP_MAP_BASE_Y,
						lCalcLocalX, lCalcLocalY,
						packMelee->fSyncDestX, packMelee->fSyncDestY,
						fDiffX, fDiffY);

					// If coordinates don't match (allowing 10 pixel tolerance for rounding) = PACKET MANIPULATION!
					if (fDiffX > 10.0f || fDiffY > 10.0f)
					{
						sys_log(0, "[SERVER_ATTACK_REJECT] PACKET MANIPULATION DETECTED! Coordinates mismatch!");
						// TODO: Increase hack counter, ban if repeated
						return;
					}
				}

				// VALIDACE #2: KNOCKBACK DISTANCE CHECK (Anti-cheat: teleport hack)
				if (packMelee->dwBlendDuration > 0 && packMelee->lX != 0 && packMelee->lY != 0)
				{
					float fKnockbackDist = DISTANCE_SQRT(
						packMelee->lSX - packMelee->lX,
						packMelee->lSY - packMelee->lY
					);

					sys_log(0, "[SERVER_ATTACK_KNOCKBACK] Src:(%ld,%ld) Dst:(%ld,%ld) Distance:%.1f MaxAllowed:800",
						packMelee->lSX, packMelee->lSY,
						packMelee->lX, packMelee->lY,
						fKnockbackDist);

					// Max realistic knockback: Force 20 * ~30-40 multiplier ≈ 600-800 pixels
					if (fKnockbackDist > 800)
					{
						sys_log(0, "[SERVER_ATTACK_REJECT] Knockback distance too large (teleport hack?)!");
						return;
					}
				}

				// VALIDACE #3: SOURCE POSITION CHECK (Anti-desync & anti-speed-hack)
				float fSourceDist = DISTANCE_SQRT(
					ch->GetX() - packMelee->lSX,
					ch->GetY() - packMelee->lSY
				);

				sys_log(0, "[SERVER_ATTACK_SOURCE] ServerPos:(%ld,%ld) PacketSrc:(%ld,%ld) Dist:%.1f MaxAllowed:300",
					ch->GetX(), ch->GetY(),
					packMelee->lSX, packMelee->lSY,
					fSourceDist);

				// If player is too far from claimed position = DESYNC or SPEED-HACK
				if (fSourceDist > 300) // 300 pixels tolerance for lag/movement
				{
					sys_log(0, "[SERVER_ATTACK_REJECT] Source position mismatch! Desync or speed-hack detected.");
					// TODO: Send resync packet to client
					return;
				}

				// ============ BLEND SYNC ============
				if (packMelee->dwBlendDuration > 0 && packMelee->lX != 0 && packMelee->lY != 0)
				{
					sys_log(0, "[SERVER_ATTACK_BLEND_SYNC] Syncing attacker to:(%ld,%ld) duration:%ums",
						packMelee->lX, packMelee->lY, packMelee->dwBlendDuration);

					ch->BlendSync(packMelee->lX, packMelee->lY, packMelee->dwBlendDuration);
				}

				// ============ EXECUTE ATTACK (ONCE!) ============
				sys_log(0, "[SERVER_ATTACK_EXECUTE] Calling Attack()");
				ch->Attack(victim, packMelee->bType);

				// ============ BROADCAST TO NEARBY PLAYERS (ONCE!) ============
				TPacketGCAttack pack;
				pack.bHeader = HEADER_GC_ATTACK;
				pack.bType = packMelee->bType;
				pack.dwAttackerVID = dwAttackerVID;  // FIX: Use correct field name
				pack.dwVID = packMelee->dwVID;
				pack.bPacket = packMelee->bPacket;
				pack.lSX = packMelee->lSX;
				pack.lSY = packMelee->lSY;
				pack.lX = packMelee->lX;
				pack.lY = packMelee->lY;
				pack.fSyncDestX = packMelee->fSyncDestX;
				pack.fSyncDestY = packMelee->fSyncDestY;
				pack.dwBlendDuration = packMelee->dwBlendDuration;

				sys_log(0, "[SERVER_ATTACK_BROADCAST] Sending to nearby players");
				ch->PacketAround(&pack, sizeof(TPacketGCAttack));
			}
			break;

		case HEADER_CG_SHOOT:
			{
				const TPacketCGShoot* const packShoot = reinterpret_cast<const TPacketCGShoot*>(data);

				ch->Shoot(packShoot->bType);
			}
			break;
	}
}

// ============================================================================
// NEXT STEPS TO COMPLETE THE FIX:
//
// 1. Find actual map base coordinates:
//    - Search for: LPSECTREE_MAP structure definition
//    - Look for: m_setting.iBaseX and m_setting.iBaseY (or similar)
//    - Replace TEMP_MAP_BASE_X and TEMP_MAP_BASE_Y with actual values
//
// 2. Implement resync packet when desync detected:
//    - Send HEADER_GC_SYNC_POSITION to force client resync
//
// 3. Implement hack counter:
//    - Track failed integrity checks per player
//    - Auto-ban after N failures
//
// 4. Test and tune thresholds:
//    - fDiffX/fDiffY tolerance (currently 10 pixels)
//    - fKnockbackDist max (currently 800 pixels)
//    - fSourceDist tolerance (currently 300 pixels)
// ============================================================================
