# Server-Side Attack Validation Patch

## 📋 Overview

This patch fixes critical PvP combat issues in Metin2:
- ✅ Attack packets not being sent to server (FIXED in client)
- ✅ Position desynchronization between clients
- ✅ No damage being dealt
- ✅ Coordinate system mismatch (global vs local)
- ✅ Anti-cheat validation system

## 🔧 What This Patch Does

### Client-Side Fixes (Already Applied)
1. **Removed early returns in `ActorInstance.cpp`**
   - Fixed lines 1290 and 1324 that prevented `__OnHit()` from being called for PvP
   - Attack packets are now sent correctly for player vs player combat

2. **Fixed source Y coordinate inversion**
   - `sBlending.source.y` is already positive from `NEW_GetCurPixelPositionRef()`
   - Only `sBlending.dest.y` needs inversion

### Server-Side Fixes (Apply This Patch)

**File:** `server/game/src/input_main.cpp`

**Function:** `void CInputMain::Attack(LPCHARACTER ch, const BYTE header, const char* data)`

**Changes:**
1. **Triple Validation System**
   - Packet Integrity Check (anti-cheat: packet manipulation)
   - Knockback Distance Check (anti-cheat: teleport hack)
   - Source Position Check (anti-desync & anti-speed-hack)

2. **Fixed Logic Flow**
   - Moved `CheckSkillHitCount` BEFORE `Attack()` (was after)
   - Removed duplicate `Attack()` and `PacketAround()` calls
   - Fixed `pack` variable declaration order

3. **Coordinate System Fix**
   - Uses `lX, lY` (global) for `BlendSync()` instead of `fSyncDestX/Y` (local)
   - Validates consistency between global and local coordinates

4. **Comprehensive Logging**
   - All validation steps logged for debugging
   - Easy to identify why attacks are rejected

## 📝 Installation Instructions

### 1. Apply to Server

```bash
# Navigate to server source
cd /usr/metin2/src/server/game/src

# Backup original file
cp input_main.cpp input_main.cpp.backup

# Edit input_main.cpp
nano input_main.cpp  # or vim

# Find the function: void CInputMain::Attack(...)
# In the HEADER_CG_ATTACK case, replace the entire attack handling code
# with the code from: SERVER_PATCH_input_main_Attack.cpp
```

### 2. Compile Server

```bash
cd /usr/metin2/src/server/game
gmake clean
gmake
```

### 3. Restart Server

```bash
# Stop game server
killall game

# Start game server
cd /usr/metin2/game/channel1
./game &
```

### 4. Test

1. Login with 2 characters (e.g., Ninja and Warrior)
2. Enable PK mode on both: `/pkmode 2`
3. Attack each other
4. Check server logs: `tail -f /usr/metin2/game/channel1/log/syserr.txt | grep SERVER_ATTACK`

Expected output:
```
[SERVER_ATTACK_RECV] Type:0 Attacker:Ninja(...) VictimVID:...
[SERVER_ATTACK_INTEGRITY] ... Diff:(...)
[SERVER_ATTACK_KNOCKBACK] Distance:... MaxAllowed:800
[SERVER_ATTACK_SOURCE] ... Dist:... MaxAllowed:300
[SERVER_ATTACK_EXECUTE] Calling Attack()
[SERVER_ATTACK_BROADCAST] Sending to nearby players
```

## ⚠️ Important Notes

### Temporary Hardcoded Values

The patch currently uses **temporary hardcoded** map base offsets:

```cpp
const long TEMP_MAP_BASE_X = 921600;
const long TEMP_MAP_BASE_Y = 204800;
```

These values were extracted from log analysis and work for testing, but you should replace them with **actual values** from your `SECTREE_MAP` structure:

```cpp
LPSECTREE_MAP pMap = SECTREE_MANAGER::instance().GetMap(ch->GetMapIndex());
long lBaseX = pMap->m_setting.iBaseX;  // Find actual field name
long lBaseY = pMap->m_setting.iBaseY;
```

**To find the correct field names:**
```bash
grep -rn "struct.*SECTREE_MAP\|m_setting" sectree*.h
```

### Validation Thresholds

You may need to tune these based on your server:

```cpp
// Packet integrity tolerance (currently 10 pixels)
if (fDiffX > 10.0f || fDiffY > 10.0f)

// Max knockback distance (currently 800 pixels)
if (fKnockbackDist > 800)

// Source position tolerance (currently 300 pixels)
if (fSourceDist > 300)
```

## 🐛 Troubleshooting

### Issue: All attacks rejected with "Distance check failed"

**Cause:** Using wrong map base offsets

**Solution:**
1. Check server log for actual coordinates
2. Calculate: `BaseOffset = ServerGlobalPos - ClientLocalPos`
3. Update `TEMP_MAP_BASE_X` and `TEMP_MAP_BASE_Y`

### Issue: Skills work but normal attacks don't

**Cause:** Normal attacks have `dwBlendDuration > 0`, skills have `dwBlendDuration = 0`

**Solution:** Check if validation is only applied when `dwBlendDuration > 0`

### Issue: Knockback rejected as "too large"

**Cause:** Max threshold (800) is too low for your game balance

**Solution:** Increase threshold or check if Force values are higher than expected

## 📊 Validation System Explained

### Validation #1: Packet Integrity
```
Client sends BOTH global (lX, lY) and local (fSyncDestX, fSyncDestY) coordinates.
Server checks: lX - BaseOffsetX ≈ fSyncDestX ?
If NO → Packet was manipulated by hacker → REJECT
```

### Validation #2: Knockback Distance
```
Server checks: |Source - Destination| < 800 pixels ?
If NO → Unrealistic knockback (teleport hack) → REJECT
```

### Validation #3: Source Position
```
Server checks: |ServerPosition - ClientClaimedPosition| < 300 pixels ?
If NO → Player is desynced or using speed-hack → REJECT
```

## 🔜 Future Improvements

- [ ] Replace hardcoded base offsets with dynamic values from SECTREE_MAP
- [ ] Implement automatic resync packet when desync detected
- [ ] Add hack counter and auto-ban system
- [ ] Create admin command to view validation statistics
- [ ] Add configurable thresholds via config file

## 📄 Commit History

- **1c047b8** - Add server-side attack validation patch
- **469d4a0** - CRITICAL FIX: Remove early returns blocking __OnHit for PvP
- **e7fe9dc** - Fix source Y coordinate inversion in attack packets

## 🆘 Support

If you encounter issues, check:
1. Server logs: `/usr/metin2/game/channel1/log/syserr.txt`
2. Client logs: Look for `[SERVER_ATTACK_*]` messages
3. Ensure both client AND server patches are applied

---

**Author:** Claude (AI Assistant)
**Date:** 2026-01-10
**Version:** 1.0
**License:** MIT
