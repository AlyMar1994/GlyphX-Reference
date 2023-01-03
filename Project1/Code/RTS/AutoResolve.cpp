/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  P E T R O G L Y P H   G A M E S, inc.        ***
 ***********************************************************************************************/
/** @file
 * 
 *		The AutoResolveClass functions and methods are defined in this file.
 */

#pragma hdrstop
#include	"Always.h"
#include	"AutoResolve.h"
#include	"BaseCombatant.h"
#include	"SquadronCombatant.h"
#include	"CompanyCombatant.h"
#include	"GameConstants.h"
#include "DynamicEnum.h"
#include "MapEnvironmentTypeConverter.h"
#include "GameRandom.h"
#include "PlanetaryBehavior.h"
#include "AI/Planning/PotentialPlan.h"
#include "Utils.h"
#include "GameObjectTypeManager.h"
#include "GameObjectCategoryType.h"
#include "AI/AIPlayer.h"
#include "AI/TacticalAIManager.h"
#include "AI/Learning/AILearningSystem.h"
#include "GameObjectManager.h"
#include "TransportBehavior.h"
#include "FleetBehavior.h"
#include "BombingRunManager.h"
#include "BombingRunEvent.h"
#include "GameScoringManager.h"

/*
**	See documentation in AutoResolve.h
*/
HRESULT AutoResolveClass::Prepare_For_Space(void)
{
	if (mIsCombatPrepared) {
		return(E_AUTORESOLVE_NOT_READY);
	}
	mIsCombatPrepared = true;
	mIsSpace = true;
	mRetreatingPlayer = -1;
	mWinningPlayer = -1;
	mEscortFire = -1;
	mUnitFire = -1;
	mIonCannon = 0;
	mHyperGun = 0;
	mHyperDamage = 0;
	mHitRoundCounter = 0;
	mBombingRun = -1;
	mBomberType = NULL;
	mStunRoundCounter = 0;
	mRetreatInProgress = false;
	mPlanetOwner = -1;
	mMidTactical = false;
	for (int index = 0; index < ARRAY_SIZE(mSides); ++index) {
		mSides[index].Init();
	}
	mTerrainType = MAP_TYPE_INVALID;
	mTransportCategory = GAME_OBJECT_CATEGORY_NONE;
	mStartFrame = FrameSynchronizer.Get_Current_Frame();
	mBattleFought = false;

	mBattleID++;
	mBattleID = mBattleID % MAX_HISTORY;
	mBattleHistory[mBattleID].Planet = NULL;
	mBattleHistory[mBattleID].Killed.clear();

	return(S_OK);
}


/*
**	See documentation in AutoResolve.h
*/
HRESULT AutoResolveClass::Prepare_For_Land(void)
{
	if (mIsCombatPrepared) {
		return(E_AUTORESOLVE_NOT_READY);
	}
	mIsCombatPrepared = true;
	mIsSpace = false;
	mRetreatingPlayer = -1;
	mWinningPlayer = -1;
	mEscortFire = -1;
	mUnitFire = -1;
	mRetreatInProgress = false;
	mIonCannon = 0;
	mHyperGun = 0;
	mBombingRun = -1;
	mBomberType = NULL;
	mHyperDamage = 0;
	mHitRoundCounter = 0;
	mStunRoundCounter = 0;
	mPlanetOwner = -1;
	mPlanet = 0;
	mMidTactical = false;
	for (int index = 0; index < ARRAY_SIZE(mSides); ++index) {
		mSides[index].Init();
	}
	mTerrainType = MAP_TYPE_INVALID;
	mTransportCategory = GAME_OBJECT_CATEGORY_NONE;
	mStartFrame = FrameSynchronizer.Get_Current_Frame();
	mBattleFought = false;

	mBattleID++;
	mBattleID = mBattleID % MAX_HISTORY;
	mBattleHistory[mBattleID].Planet = NULL;
	mBattleHistory[mBattleID].Killed.clear();

	return(S_OK);
}

void AutoResolveClass::Add_Tactical_Combatants(void)
{
	GameModeClass *this_mode = GameModeManager.Get_Active_Mode();
	FAIL_IF(this_mode == NULL) return;

	GameModeClass *parent_mode = GameModeManager.Get_Parent_Game_Mode(this_mode);
	FAIL_IF(parent_mode == NULL) return;

	ConflictInfoStruct *conflict_info = NULL;
	if (this_mode->Get_Sub_Type() == SUB_GAME_MODE_SPACE)
	{
		mIsSpace = true;
		conflict_info = parent_mode->Get_Object_Manager().Get_Space_Conflict_Info();
		mPlanet = parent_mode->Get_Object_Manager().Get_Object_From_ID(conflict_info->LocationID);
	}
	else if (this_mode->Get_Sub_Type() == SUB_GAME_MODE_LAND)
	{
		mIsSpace = false;
		conflict_info = parent_mode->Get_Object_Manager().Get_Land_Invasion_Info();
		mPlanet = parent_mode->Get_Object_Manager().Get_Object_From_ID(conflict_info->LocationID);

		if ( this_mode->Get_Land_Bombing_Run_Manager() != NULL )
		{
			if (this_mode->Get_Land_Bombing_Run_Manager()->Is_Bombing_Run_Available(conflict_info->InvadingPlayerID))
			{
				mBomberType = this_mode->Get_Land_Bombing_Run_Manager()->Get_Bombing_Run_Type(conflict_info->InvadingPlayerID);
				if (mBomberType)
				{
					mBombingRun = conflict_info->InvadingPlayerID;
				}
			}
		}
	}
	FAIL_IF(!mPlanet || !conflict_info) return;

	mBattleHistory[mBattleID].Planet = mPlanet->Get_Type();
	mSides[0].mOwnerID = conflict_info->DefendingPlayerID;
	mSides[1].mOwnerID = conflict_info->InvadingPlayerID;
	mMidTactical = true;

	int i;
	ObjectPersistenceClass *persist_objects = GameModeManager.Get_Parent_Mode_Persistent_Objects(this_mode);
	DynamicVectorClass<ObjectPersistenceClass::PersistentUnit> &attacking_reinforcements = persist_objects->Get_Reinforcements(conflict_info->InvadingPlayerID);
	for (i = 0; i < attacking_reinforcements.Get_Count(); i++)
	{
		int pid = attacking_reinforcements.Get_At(i).ObjectID;
		if (pid == INVALID_OBJECT_ID) continue;
		GameObjectClass *object = parent_mode->Get_Object_Manager().Get_Object_From_ID(pid);
		FAIL_IF(!object) continue;
		TransportBehaviorClass *tbehave = (TransportBehaviorClass *)object->Get_Behavior(BEHAVIOR_TRANSPORT);
		FleetBehaviorClass *fbehave = (FleetBehaviorClass *)object->Get_Behavior(BEHAVIOR_FLEET);
		if (tbehave && mIsSpace == false)
		{
			for (int t = 0; t < (int)tbehave->Get_Contained_Objects_Count(); t++)
			{
				GameObjectClass *unit = tbehave->Get_Contained_Object(object, t);
				Add_Individual(unit);
			}
		}
		else if (fbehave && mIsSpace)
		{
			for (int t = 0; t < (int)fbehave->Get_Contained_Objects_Count(); t++)
			{
				GameObjectClass *unit = fbehave->Get_Contained_Object(object, t);
				Add_Individual(unit);
			}
		}
		else
		{
			Add_Individual(object);
		}
	}

	DynamicVectorClass<ObjectPersistenceClass::PersistentUnit> &defending_reinforcements = persist_objects->Get_Reinforcements(conflict_info->DefendingPlayerID);
	for (i = 0; i < defending_reinforcements.Get_Count(); i++)
	{
		int pid = defending_reinforcements.Get_At(i).ObjectID;
		if (pid == INVALID_OBJECT_ID) continue;
		GameObjectClass *object = parent_mode->Get_Object_Manager().Get_Object_From_ID(pid);
		FAIL_IF(!object) continue;
		TransportBehaviorClass *tbehave = (TransportBehaviorClass *)object->Get_Behavior(BEHAVIOR_TRANSPORT);
		FleetBehaviorClass *fbehave = (FleetBehaviorClass *)object->Get_Behavior(BEHAVIOR_FLEET);
		if (tbehave && mIsSpace == false)
		{
			for (int t = 0; t < (int)tbehave->Get_Contained_Objects_Count(); t++)
			{
				GameObjectClass *unit = tbehave->Get_Contained_Object(object, t);
				Add_Individual(unit);
			}
		}
		else if (fbehave && mIsSpace)
		{
			for (int t = 0; t < (int)fbehave->Get_Contained_Objects_Count(); t++)
			{
				GameObjectClass *unit = fbehave->Get_Contained_Object(object, t);
				Add_Individual(unit);
			}
		}
		else
		{
			Add_Individual(object);
		}
	}

	for (i = 0; i < this_mode->Get_Object_Manager().Get_Object_Count(); i++)
	{
		GameObjectClass *object = this_mode->Get_Object_Manager().Get_Object_At_Index(i);
		GameObjectClass *parent = object->Get_Parent_Container_Object();

		if (object->Get_Type()->Is_Decoration() == false && 
			 object->Get_Behavior(BEHAVIOR_PROJECTILE) == NULL && 
			 object->Get_Behavior(BEHAVIOR_PARTICLE) == NULL &&
			 object->Get_Behavior(BEHAVIOR_TEAM) == NULL &&
			 object->Get_Behavior(BEHAVIOR_FLEET) == NULL &&
			 (!parent || parent->Get_Behavior(BEHAVIOR_TRANSPORT) == NULL) && 
			 (object->Get_Owner() == mSides[0].mOwnerID || object->Get_Owner() == mSides[1].mOwnerID))
		{
			Add_Individual(object);
		}
	}
}


/*
**	See documentation in AutoResolve.h
*/
HRESULT AutoResolveClass::Add_Combatant(GameObjectClass * object)
{
	if (!mIsCombatPrepared) return(E_AUTORESOLVE_NOT_READY);
	if (mIsCombatInitiated) return(E_AUTORESOLVE_COMBAT_STARTED);
	if (object == NULL) return(E_POINTER);

	if (mIsSpace && object->Get_Type()->Can_Participate_In_Space_Battle() == false)
		return S_OK;

	/*
	** If this is a fleet object if ths is for space combat, add in the ships. If this is for land
	**	combat, add in the land units.
	*/
	if (object->Behaves_Like(BEHAVIOR_FLEET)) {
		FleetDataPackClass * data = object->Get_Fleet_Data();
		assert(data != NULL);
		if (data == NULL) return(E_AUTORESOLVE_BAD_FLEET);

		if (mIsSpace && data->Is_Squadron()) {
			ICombatantBehaviorPtr p = new SquadronCombatantClass(object);
			assert(p != NULL);
			if (p == NULL) return(E_OUTOFMEMORY);

			HRESULT result = Add_Individual(p);
			assert(SUCCEEDED(result));
			if (FAILED(result)) return(result);
		}

		if (!data->Is_Squadron()) {

			object->Add_Ref();
			mFleets.insert(mFleets.end(), object);

			/*
			**	Otherwise, it must be a container of ships and squadrons, so step through the list
			**	and add each member.
			*/
			const DynamicVectorClass<GameObjectClass *> * list = data->Get_Starship_List();
			for (int index = 0; index < list->Get_Count(); ++index) {
				HRESULT result = Add_Combatant((*list)[index]);
				assert(SUCCEEDED(result));
				if (FAILED(result)) return(result);
			}
		}
	} else if (object->Behaves_Like(BEHAVIOR_PLANET)) {

		/*
		**	Add in any base present -- land or space as appropriate
		*/
		ICombatantBehaviorPtr p = new BaseCombatantClass(object, !mIsSpace);
		assert(p != NULL);
		if (p == NULL) return(E_OUTOFMEMORY);

		if (p->Is_Alive()) {
			HRESULT result = Add_Individual(p);
			assert(SUCCEEDED(result));
			if (FAILED(result)) return(result);
		}

		/*
		**	Fetch the planetary data for analysis.
		*/
		PlanetaryDataPackClass * pdata = object->Get_Planetary_Data();
		assert(pdata != NULL);
		if (pdata == NULL) return(E_AUTORESOLVE_BAD_PLANET);

		mPlanet = object;

		mBattleHistory[mBattleID].Planet = mPlanet->Get_Type();

		if (!mIsSpace) {

			SignalDispatcherClass::Get().Send_Signal( object, PG_SIGNAL_OBJECT_GROUND_CONFLICT_BEGIN, NULL );

			/*
			**	Add in any landed transports (i.e., ground companies)
			*/
			const DynamicVectorClass<GameObjectClass *> * landed = pdata->Get_Landed_Transports();
			assert(landed != NULL);
			if (landed == NULL) return(E_FAIL);

			int i;
			for (i = 0; i < landed->Get_Size(); ++i) {

				HRESULT result = Add_Combatant(landed->Get_At(i));
				assert(SUCCEEDED(result));
				if (FAILED(result)) return(result);
			}

			ConflictInfoStruct *conflict_info = NULL;
			conflict_info = GALACTIC_GAME_OBJECT_MANAGER.Get_Land_Invasion_Info();
			FAIL_IF(!conflict_info) return (E_FAIL);

			const DynamicVectorClass<GameObjectClass *> * fleets = pdata->Get_Orbiting_Fleets();
			if (fleets)
			{
				for (i = 0; i < fleets->Get_Count(); i++)
				{
					GameObjectClass *fleet = fleets->Get_At(i);
					if (!fleet) continue;
					if (fleet->Get_Owner() != conflict_info->InvadingPlayerID) continue;
					if (fleet->Get_Fleet_Data())
					{
						FleetDataPackClass *fdata = fleet->Get_Fleet_Data();
						const DynamicVectorClass<GameObjectClass *> * ships = fdata->Get_Starship_List();
						for (int t = 0; t < ships->Get_Count(); t++)
						{
							GameObjectClass *ship = ships->Get_At(t);
							if (!ship) continue;

							int current_tech_level = fleet->Get_Owner_Player()->Get_Tech_Level();

							for( int tech_level = 0; tech_level <= current_tech_level; tech_level ++ )
							{	
								int spawn_count = ship->Get_Type()->Get_Spawned_Unit_Type_Count_At_Tech_Level(tech_level);

								for(int spawn_index = 0; spawn_index < spawn_count; ++spawn_index)
								{
									const GameObjectTypeClass *squadron_unit_type = ship->Get_Type()->Get_Spawned_Unit_Type_At_Tech_Level(tech_level, spawn_index);

									if(NULL != squadron_unit_type)
									{
										for (int squad_member = 0; squad_member < squadron_unit_type->Get_Num_Squadron_Units(); ++squad_member)
										{
											GameObjectTypeClass *member_type = squadron_unit_type->Get_Squadron_Unit(squad_member);

											if (member_type->Get_Land_Bomber_Type())
											{
												mBomberType = member_type->Get_Land_Bomber_Type();
												break;
											}
										}
										if (mBomberType) break;
									}
								}
								if (mBomberType) break;
							}

							if (ship->Get_Fleet_Data())
							{
								const DynamicVectorClass<GameObjectClass *> * units = ship->Get_Fleet_Data()->Get_Starship_List();
								for (int s = 0; s < units->Get_Count(); s++)
								{
									GameObjectClass *unit = units->Get_At(s);
									if (!unit) continue;
									if (unit->Get_Type()->Get_Land_Bomber_Type())
									{
										mBomberType = unit->Get_Type()->Get_Land_Bomber_Type();
										break;
									}
								}
							}
							if (mBomberType) break;
						}
					}
					if (mBomberType) break;
				}
			}

			if (mBomberType)
			{
				mBombingRun = conflict_info->InvadingPlayerID;
			}

			for (i = 0; i < pdata->Get_Ground_Special_Structures().Size(); ++i)
			{
				GameObjectClass *structure = pdata->Get_Ground_Special_Structures().Get_At(i);
				FAIL_IF(!structure) { continue; }
				Add_Individual(structure, true);
			}

		} else {

			mPlanetOwner = object->Get_Owner();

			/*
			**	Track the "best" Ion cannon stun effect available.
			*/
			int ion = pdata->Get_Space_Autoresolve_Stun_Rate();
			if (mIonCannon == 0 || (ion > 0 && ion < mIonCannon)) {
				mStunRoundCounter = mIonCannon = ion;
			}

			/*
			**	Track the "best" hypervelocity gun available.
			*/
			int hg,hd;
			hg = pdata->Get_Space_Autoresolve_Hit_Rate(&hd);
			if (mHyperGun == 0 || (hg > 0 && hg < mHyperGun)) {
				mHitRoundCounter = mHyperGun = hg;
				mHyperDamage = hd;
			}
		}
		

	} else if (object->Behaves_Like(BEHAVIOR_TRANSPORT)) {

		if (mIsSpace) {
			HRESULT result = Add_Individual(object);
			assert(SUCCEEDED(result));
			if (FAILED(result)) return(result);
		} else {

			ICombatantBehaviorPtr p = new CompanyCombatantClass(object);
			assert(p != NULL);
			if (p == NULL) return(E_OUTOFMEMORY);

			HRESULT result = Add_Individual(p);
			assert(SUCCEEDED(result));
			if (FAILED(result)) return(result);

		}

	} else if (object->Behaves_Like(BEHAVIOR_DUMMY_STARSHIP)) {

		if (mIsSpace) {
			HRESULT result = Add_Individual(object);
			assert(SUCCEEDED(result));
			if (FAILED(result)) return(result);
		}
	} else {
		/*
		**	The object submitted is unrecognized. This is an error condition that indicates a
		**	larger error has occurred -- a bad object is in the list of objects in a fleet or on land.
		*/
		return(E_INVALIDARG);
	}
	return(S_OK);
}


/*
**	See documentation in AutoResolve.h
*/
HRESULT AutoResolveClass::Initiate_Combat(int aggressor)
{
	if (mIsCombatInitiated || !mIsCombatPrepared) {
		return(E_AUTORESOLVE_NOT_READY);
	}
	mIsCombatInitiated = true;
	mAggressor = aggressor;

	/*
	**	Verify that the specified aggressor is valid according to the participants.
	*/
	const SideStruct * side = Get_Side_By_Owner(aggressor);
	if (side == NULL) return(E_AUTORESOLVE_AGGRESSOR_ERROR);

	/*
	**	Verify that there are two sides and they each have at least one unit
	**	that is participating in combat.
	*/
	for (int index = 0; index < ARRAY_SIZE(mSides); ++index) {
		if (mSides[index].mOwnerID == -1) {
			return(E_AUTORESOLVE_NO_CONFLICT);
		}
		if (mSides[index].mQueue.size() == 0) {
			return(E_AUTORESOLVE_NO_CONFLICT);
		}
	}

	if (mPlanet) {
		TheMapEnvironmentTypeConverterPtr->String_To_Enum(*mPlanet->Get_Type()->Get_Terrain_Type(), mTerrainType);
	}

	PG_VERIFY(TheGameObjectCategoryTypeConverterPtr->String_To_Enum(std::string("Transport"), mTransportCategory));

	TheGameScoringManagerClass::Get().Reset_Tactical_Stats();

	// Clear out collected bounty
	for (int i=0; i<PlayerList.Get_Num_Players(); i++)
	{
		PlayerClass *player = PlayerList.Get_Player_By_Index(i);
		if (player)
		{
			player->Clear_Bounty_Collected();
		}
	}

	for (int index = 0; index < ARRAY_SIZE(mSides); ++index) {
		Calculate_Side_Force(mSides[index].mQueue, mSides[index].mTotalForce, mSides[index].mOwnerID, mSides[index].mWeakestUnit);
		Find_Special_Heroes(index);
	}

	bool found_super_weapon = mSides[0].mSuperWeapon || mSides[1].mSuperWeapon;

	if (!found_super_weapon && mIsSpace && mSides[0].mTotalForce[1].Force <= 0.0f && mSides[1].mTotalForce[1].Force <= 0.0f)
	{
		// Only transports on both sides.
		int loser = 0;
		int winner = 0;

		if (mSides[0].mQueue.size() > mSides[1].mQueue.size())
		{
			loser = 1; winner = 0;
		}
		else if (mSides[0].mQueue.size() == mSides[1].mQueue.size())
		{
			loser = mAggressor == mSides[0].mOwnerID ? 0 : 1;
			winner = loser == 0 ? 1 : 0;
		} 
		else 
		{
			loser = 0; winner = 1;
		}

		mWinningPlayer = mSides[winner].mOwnerID;
		Player_Retreats(mSides[loser].mOwnerID);

		mBattleFought = true;
	}

	return(S_OK);
}

void AutoResolveClass::Update_Battle_History(const GameObjectClass *object)
{
	if (object->Get_Behavior(BEHAVIOR_PLANET))
	{
		if (object->Get_Planetary_Data() && object->Get_Planetary_Data()->Get_StarBase() && 
			 object->Get_Planetary_Data()->Get_StarBase()->GameObjectType)
		{
			mBattleHistory[mBattleID].Killed.push_back(std::make_pair(object->Get_Planetary_Data()->Get_StarBase()->GameObjectType, object->Get_Owner()));
		}
		return;
	}

	TransportBehaviorClass *tbehave = (TransportBehaviorClass *)object->Get_Behavior(BEHAVIOR_TRANSPORT);
	FleetBehaviorClass *fbehave = (FleetBehaviorClass *)object->Get_Behavior(BEHAVIOR_FLEET);
	if (tbehave)
	{
		for (int i = 0; i < (int)tbehave->Get_Contained_Objects_Count(); i++)
		{
			const GameObjectClass *unit = tbehave->Get_Contained_Object(const_cast<GameObjectClass *>(object), i);
			mBattleHistory[mBattleID].Killed.push_back(std::make_pair(unit->Get_Original_Object_Type(), unit->Get_Owner()));
		}
	}
	else if (fbehave)
	{
		for (int i = 0; i < (int)fbehave->Get_Contained_Objects_Count(); i++)
		{
			const GameObjectClass *unit = fbehave->Get_Contained_Object(const_cast<GameObjectClass *>(object), i);
			mBattleHistory[mBattleID].Killed.push_back(std::make_pair(unit->Get_Original_Object_Type(), unit->Get_Owner()));
		}
	}
	else
	{
		mBattleHistory[mBattleID].Killed.push_back(std::make_pair(object->Get_Original_Object_Type(), object->Get_Owner()));
	}
}

bool AutoResolveClass::Apply_Transport_Losses(std::vector<ICombatantBehaviorPtr> &units, bool is_pirate, ICombatantBehaviorPtr killer)
{
	float transport_losses = TheGameConstants.Get_Auto_Resolve_Transport_Losses();

	if (mIsSpace == false) 
		return false;

	int tcnt = 0;
	int i = 0;
	for (i = 0; i < (int)units.size(); i++)
	{
		const GameObjectClass *object = units[i]->Get_Object();
		// const GameObjectTypeClass *type = object->Behaves_Like(BEHAVIOR_PLANET) ? units[i]->Get_Object_Type() : object->Get_Original_Object_Type();

		// if ((unsigned int)type->Get_Category_Mask() & (unsigned int)mTransportCategory)
		if (object->Behaves_Like(BEHAVIOR_TRANSPORT))
		{
			tcnt++;
		}
	}

	int rcnt = tcnt == 1 ? 0 : (int)(((float)tcnt) * (1.0f - transport_losses) + 0.5f);

	if (is_pirate)
	{
		rcnt = 0;
	}
	tcnt = 0;
	bool any_left = false;

	for (i = 0; i < (int)units.size() && tcnt < rcnt; i++)
	{
		const GameObjectClass *object = units[i]->Get_Object();
		if (object->Behaves_Like(BEHAVIOR_TRANSPORT) && object->Contains_Named_Hero())
		{
			++tcnt;
			units.erase(units.begin() + i);
			--i;
			any_left = true;
		}
	}

	for (i = 0; i < (int)units.size(); i++)
	{
		const GameObjectClass *object = units[i]->Get_Object();

		if (object->Behaves_Like(BEHAVIOR_TRANSPORT))
		{
			if (tcnt >= rcnt || is_pirate)
			{
				Update_Battle_History(object);
				units[i]->Take_Damage(DAMAGE_MISC, BIG_FLOAT, killer);
			}
			else
			{
				any_left = true;
			}

			tcnt++;
		}
	}
	return any_left;
}

void AutoResolveClass::Kill_Retreating_Units(void)
{
	if (mRetreatingPlayer == -1) return;
	int winner = mSides[0].mOwnerID == mRetreatingPlayer ? 1 : 0;
	int loser = mSides[1].mOwnerID == mRetreatingPlayer ? 1 : 0;
	if (loser == winner) return;

	if (mSides[winner].mQueue.size() == 0) return;
	ICombatantBehaviorPtr wobj = mSides[winner].mQueue[0];

	for (unsigned int i = 0; i < mSides[loser].mQueue.size(); ++i)
	{
		mSides[loser].mQueue[i]->Take_Damage(DAMAGE_MISC, BIG_FLOAT, wobj);
		Update_Battle_History(mSides[loser].mQueue[i]->Get_Object());
	}

	mRetreatInProgress = false;
	mRetreatingPlayer = -1;
}


bool AutoResolveClass::Apply_Attrition(std::vector<ICombatantBehaviorPtr> &units, TargetContrastClass::ResultType &current, 
													ICombatantBehaviorPtr &weakest_unit, bool is_loser, int index, ICombatantBehaviorPtr killer)
{
	float attrition_allowance_factor = TheGameConstants.Get_Auto_Resolve_Attrition_Allowance_Factor();

	bool any_survivors = false;
	bool weak_killed = false;
	bool is_structure = false;

	int global_index = mIsSpace ? 1 : 0;

	// MLL: Hack to make tactical auto resolve less efficient.
	if (mMidTactical)
	{
		current[global_index].Force *= TheGameConstants.Get_Auto_Resolve_Tactical_Multiplier();

	}

	static std::vector<ICombatantBehaviorPtr> left_overs;

	while (units.size() != 0)
	{
		int unit_index = -1;
		is_structure = false;

		for (unsigned int i = 0; i < units.size(); ++i)
		{
			const GameObjectClass *object = units[i]->Get_Object();
			if (object && object->Contains_Named_Hero())
			{
				unit_index = static_cast<int>(i);
				break;
			}
			if (object && object->Get_Behavior(BEHAVIOR_DUMMY_GROUND_STRUCTURE))
			{
				unit_index = static_cast<int>(i);
				is_structure = true;
				break;
			}
		}

		if (unit_index == -1)
		{
			unit_index = SyncRandom.Get(0, units.size()-1);
		}

		const GameObjectClass *object = units[unit_index]->Get_Object();
		const GameObjectTypeClass *type = Get_Type_From_Combatant(units[unit_index]);
		PlayerClass *owner = PlayerList.Get_Player_By_ID(object->Get_Owner());

		bool kill_unit = false;
		bool kill_base = false;

		if (mIsSpace == false || object->Behaves_Like(BEHAVIOR_TRANSPORT) == false)
		{
			if (is_loser && !owner->Get_Faction()->Is_Playable() && !object->Behaves_Like(BEHAVIOR_PLANET))
			{
				//Non-playable factions are wiped out when they lose.
				kill_unit = true;
			}
			else if (type->Is_Super_Weapon() == true)
			{
				kill_unit = false;
				if (is_loser && mSides[index ? 0 : 1].mSuperWeaponKiller)
				{
					kill_unit = true;
					if (weakest_unit == units[i]) weakest_unit.Release_Ref();
				}
			}
			else if (is_loser && object->Behaves_Like(BEHAVIOR_PLANET))
			{
				//Loser base is always completely destroyed
				PlanetaryBehaviorClass *behave = (PlanetaryBehaviorClass *)object->Get_Behavior(BEHAVIOR_PLANET);
				if (killer)
				{
					const_cast<GameObjectClass *>(object)->Set_Final_Blow_Player_ID(killer->Get_Object()->Get_Owner());
				}

				Update_Battle_History(object);

				//Always destroy the star base: mostly it probably doesn't exist anyway, but in the case of a raid
				//it might and we had better make sure it goes away because the planet is about to have a new owner
				behave->Set_Current_Starbase_Level(const_cast<GameObjectClass *>(object), BASE_NONE, NULL);
				behave->Clear_Space_Special_Structures(const_cast<GameObjectClass *>(object));
				if (!mIsSpace)
				{
					behave->Clear_Ground_Special_Structures(const_cast<GameObjectClass *>(object));
				}

				kill_base = true;
				kill_unit = false;
			}
			else if (is_loser && is_structure)
			{
				// MLL: Kill all structures if the player is the loser.
				kill_unit = true;
			}
			else
			{
				kill_unit = true;

				//Apply garrison units
				bool add_garrison = (object->Get_Parent_Mode_ID() == INVALID_OBJECT_ID);
				if (add_garrison && object->Get_Type()->Behaves_Like(BEHAVIOR_DUMMY_STAR_BASE) && mMidTactical)
				{
					add_garrison = false;
				}
				int tech_level = owner->Get_Tech_Level();
				int garrison_type_count = type->Get_Spawned_Unit_Type_Count(tech_level);
				if (add_garrison)
				{
					for (int j = 0; j < garrison_type_count; ++j)
					{
						const GameObjectTypeClass *garrison_type = type->Get_Spawned_Unit_Type(tech_level, j);
						FAIL_IF(!garrison_type) { continue; }

						float total_force = type->Get_Spawned_Unit_Starting_Count(tech_level, garrison_type) * garrison_type->Get_AI_Combat_Power_Metric();
						current[global_index].Force -= total_force;
						current[global_index].Force = Max(current[global_index].Force, 0.0f);
					}
				}

				PlanetaryBehaviorClass *behave = (PlanetaryBehaviorClass *)object->Get_Behavior(BEHAVIOR_PLANET);
				if (behave)
				{
					//Possibly reduce level of star base.  We'll make ground bases all or nothing
					if (mIsSpace)
					{
						const GameObjectTypeClass *new_type = behave->Get_Base_Type_For_Combat_Rating(object, !mIsSpace, current[global_index].Force, attrition_allowance_factor);

						if (new_type != type)
						{
							if (killer)
							{
								const_cast<GameObjectClass *>(object)->Set_Final_Blow_Player_ID(killer->Get_Object()->Get_Owner());
							}
							behave->Set_Current_Starbase_Level(const_cast<GameObjectClass *>(object), PlanetBaseLevelType(new_type ? new_type->Get_Base_Level() : BASE_NONE), new_type);
						}

						if (new_type)
						{
							current[global_index].Force -= new_type->Get_AI_Combat_Power_Metric();
						} 
						else 
						{ 
							kill_base = true;
						}
					}
					kill_unit = false;
				} 
				else if (current[global_index].Force - (type->Get_AI_Combat_Power_Metric() * attrition_allowance_factor) > 0.0f)
				{
					current[global_index].Force -= type->Get_AI_Combat_Power_Metric();
					current[global_index].Force = Max(current[global_index].Force, 0.0f);
					kill_unit = false;
				}
			}

			if (mMidTactical == false && owner && owner->Get_Is_AI_Controlled())
			{
				AIPlayerClass *ai_player = owner->Get_AI_Player();
				TacticalAIManagerClass *tactical_manager = ai_player->Get_Tactical_Manager_By_Mode(SUB_GAME_MODE_GALACTIC);
				if (tactical_manager)
				{
					AILearningSystemClass *learning_system = tactical_manager->Get_Learning_System();
					const GameObjectTypeClass *survival_type = type;
					if (survival_type->Get_Num_Ground_Company_Units() > 0 &&
							!survival_type->Get_Ground_Company_Unit(0)->Get_Create_Team())
					{
						survival_type = survival_type->Get_Ground_Company_Unit(0);
					}
					learning_system->Register_Unit_Survival(survival_type, !kill_unit && !kill_base);
				}
			}
		}

		if (kill_unit)
		{
			if (weakest_unit != units[unit_index])
			{
				if (units[unit_index]->Get_Object_Ptr())
				{
					// MLL: Don't allow survivors if auto resolving.
					units[unit_index]->Get_Object_Ptr()->Set_Allow_Survivors_Upon_Death(false);
				}
				units[unit_index]->Take_Damage(DAMAGE_MISC, BIG_FLOAT, killer);				
				Update_Battle_History(units[unit_index]->Get_Object());
			}
			else 
			{
				weak_killed = true;
			}
		}
		else if (!kill_base)
		{
			left_overs.push_back(units[unit_index]);
		}

		units.erase(units.begin()+unit_index);
	}

	units = left_overs;
	any_survivors = left_overs.size() != 0;
	left_overs.resize(0);

	if (any_survivors && weakest_unit && weak_killed)
	{
		// MLL: Don't allow survivors if auto resolving.
		if (weakest_unit->Get_Object_Ptr())
		{
			weakest_unit->Get_Object_Ptr()->Set_Allow_Survivors_Upon_Death(false);
		}
		weakest_unit->Take_Damage(DAMAGE_MISC, BIG_FLOAT, killer);
		Update_Battle_History(weakest_unit->Get_Object());
	}

	if (!any_survivors && !is_loser && weakest_unit)
	{
		units.push_back(weakest_unit);
		return true;
	}

	return any_survivors;
}


void AutoResolveClass::Find_Contrast_Index(float remaining_power,
														 const GameObjectTypeClass *type,
                                           const TargetContrastClass::ResultType &current,
                                           int &best_category)
{
	float best_weight = 0.0f;
	for (unsigned int i = 0; i < current.size(); ++i)
	{
		if (current[i].Category == GAME_OBJECT_CATEGORY_NONE)
		{
			continue;
		}
		float remaining = current[i].Force;
		if (remaining <= 0.0f)
		{
			continue;
		}

		float contrast_weight = TargetContrastClass::Get_Average_Contrast_Factor(type, NULL, current[i].Category);
		if (mTerrainType != MAP_TYPE_INVALID)
		{
			contrast_weight *= TargetContrastClass::Get_Effectiveness_On_Terrain(type, mTerrainType);
		}

		if (contrast_weight <= 0.0f)
		{
			continue;
		}

		float build_weight = (remaining - remaining_power * contrast_weight) / 
									Max(remaining, remaining_power * contrast_weight);
		build_weight *= -build_weight;
		build_weight += 1.0;

		build_weight = Max(build_weight, 0.0f);

		build_weight *= contrast_weight;

		if (build_weight > best_weight)
		{
			best_weight = build_weight;
			best_category = i;
		}		
	}
}


void AutoResolveClass::Side_Attack(std::vector<ICombatantBehaviorPtr> &units, TargetContrastClass::ResultType &target_force, 
                                   TargetContrastClass::ResultType &result, int player_id)
{
	result = target_force;
	PlayerClass *owner = PlayerList.Get_Player_By_ID(player_id);

	static std::vector<float> cat_table;
	cat_table.resize(0);
	cat_table.resize(32, 0.0f);

	// search for heroes...
	int i = 0;
	for (i = 0; i < (int)units.size(); i++)
	{
		const GameObjectClass *object = units[i]->Get_Object();
		const GameObjectTypeClass *type = Get_Type_From_Combatant(units[i]);

		type = PotentialPlanClass::Get_Hero_Type_From_Build_Type(const_cast<GameObjectTypeClass *>(type));
	
		// If this unit exists as an acutal unit then
		// get the real game object and pass it with our special ability queries.
		if (type->Has_Special_Ability(PotentialPlanClass::Get_Hero_Object_From_Build_Object(const_cast<GameObjectClass *>(object))) == false)
			continue;
	
		// query the weight a unit applies
		// query function should return a std::vector<std::pair<category, float> >
		const std::vector<std::pair<GameObjectCategoryType, float> > &vec = 
			type->Get_Special_Ability_Unit_Strength_Factors(PotentialPlanClass::Get_Hero_Object_From_Build_Object(const_cast<GameObjectClass *>(object)), mPlanet);
	
		// when we go to factor in the weight we have to add 1.0 back then multiply.
		for (int t = 0; t < (int)vec.size(); t++) {
			float weight = vec[t].second - 1.0f;
			int idx = GET_FIRST_BIT_SET((unsigned int)(vec[t].first));
			if (idx > -1 && weight > cat_table[idx]) {
				cat_table[idx] = weight;
			}
		}
	}

	result[0].Ground = true;
	result[1].Ground = false;

	if (mBombingRun == player_id && mBomberType)
	{
		int best_category = -1;
		float remaining_power = mBomberType->Get_AI_Combat_Power_Metric();
		while (remaining_power > 0.0f && best_category != 0)
		{
			Find_Contrast_Index(remaining_power, mBomberType, result, best_category);
			Apply_Unit_Contrast(remaining_power, mBomberType, result, best_category, cat_table, mTerrainType);
			best_category = 0;
		}	
	}

	for (i = 0; i < (int)units.size(); i++)
	{
		const GameObjectClass *object = units[i]->Get_Object();
		const GameObjectTypeClass *type = Get_Type_From_Combatant(units[i]);

		int best_category = -1;

		if (!mIsSpace && object->Behaves_Like(BEHAVIOR_PLANET))
		{
			//Use turrets to compute ground base strength
			const PlanetaryDataPackClass *planet_data = object->Get_Planetary_Data();
			ENFORCED_IF(planet_data)
			{
				int tactical_built_object_count = planet_data->Get_Persistent_Built_Tactical_Object_Count();
				for (int j = 0; j < tactical_built_object_count; ++j)
				{
					const GameObjectTypeClass *built_type = planet_data->Get_Persistent_Built_Tactical_Object_Type_At_Index(j);
					FAIL_IF(!built_type) { continue; }
					float remaining_power = built_type->Get_AI_Combat_Power_Metric();
					while (remaining_power > 0.0f && best_category != 0)
					{
						Find_Contrast_Index(remaining_power, built_type, result, best_category);
						Apply_Unit_Contrast(remaining_power, built_type, result, best_category, cat_table, mTerrainType);
						best_category = 0;
					}	
				}
			}
			continue;
		}

		//Apply garrison units
		bool add_garrison = (object->Get_Parent_Mode_ID() == INVALID_OBJECT_ID);
		if (add_garrison && object->Get_Type()->Behaves_Like(BEHAVIOR_DUMMY_STAR_BASE) && mMidTactical)
		{
			add_garrison = false;
		}
		int tech_level = owner->Get_Tech_Level();
		int garrison_type_count = type->Get_Spawned_Unit_Type_Count(tech_level);
		if (add_garrison)
		{
			for (int j = 0; j < garrison_type_count; ++j)
			{
				const GameObjectTypeClass *garrison_type = type->Get_Spawned_Unit_Type(tech_level, j);
				FAIL_IF(!garrison_type) { continue; }

				int active_count = type->Get_Spawned_Unit_Starting_Count(tech_level, garrison_type);
				for (int k = 0; k < active_count; ++k)
				{
					float remaining_power = garrison_type->Get_AI_Combat_Power_Metric();
					while (remaining_power > 0.0f && best_category != 0)
					{
						Find_Contrast_Index(remaining_power, garrison_type, result, best_category);
						Apply_Unit_Contrast(remaining_power, garrison_type, result, best_category, cat_table, mTerrainType);
						best_category = 0;
					}
				}
			}
		}

		if (type->Behaves_Like(BEHAVIOR_TRANSPORT) && mIsSpace) continue;

		float remaining_power = type->Get_AI_Combat_Power_Metric();
		while (remaining_power > 0.0f && best_category != 0)
		{
			Find_Contrast_Index(remaining_power, type, result, best_category);
			Apply_Unit_Contrast(remaining_power, type, result, best_category, cat_table, mTerrainType);
			best_category = 0;
		}
	}
}


bool Contains_Super_Weapon_Killer(const GameObjectTypeClass *type)
{
	int i;
	for (i = 0; i < type->Get_Num_Ground_Company_Units(); i++)
	{
		if (type->Get_Ground_Company_Unit(i)->Is_Super_Weapon_Killer())
		{
			return true;
		}
	}

	for (i = 0; i < type->Get_Num_Squadron_Units(); i++)
	{
		if (type->Get_Squadron_Unit(i)->Is_Super_Weapon_Killer())
		{
			return true;
		}
	}
	return false;
}


void AutoResolveClass::Find_Special_Heroes(int index)
{
	for (int i = 0; i < (int)mSides[index].mQueue.size(); i++)
	{
		const GameObjectClass *object = mSides[index].mQueue[i]->Get_Object();
		const GameObjectTypeClass *otype = object->Behaves_Like(BEHAVIOR_PLANET) ? mSides[index].mQueue[i]->Get_Object_Type() : object->Get_Original_Object_Type();

		const GameObjectTypeClass *type = PotentialPlanClass::Get_Hero_Type_From_Build_Type(const_cast<GameObjectTypeClass *>(otype));

		if (type->Is_Super_Weapon())
		{
			mSides[index].mSuperWeapon = mSides[index].mQueue[i];
		}

		if (Contains_Super_Weapon_Killer(otype))
		{
			mSides[index].mSuperWeaponKiller = mSides[index].mQueue[i];
		}
	}
}

void AutoResolveClass::Calculate_Side_Force(std::vector<ICombatantBehaviorPtr> &units, TargetContrastClass::ResultType &result, 
														  int player_id, ICombatantBehaviorPtr &weakest_unit)
{
	PlayerClass *owner = PlayerList.Get_Player_By_ID(player_id);
	TargetContrastClass::Init_Contrast_Type_List();

	result.resize(0);
	result.resize(TargetContrastClass::ContrastTypeList.size() + 2);

	int i = 0;
	result[0].Ground = true;
	result[1].Ground = false;

	float weakest_val = BIG_FLOAT;

	for (i = 0; i < (int)units.size(); i++)
	{
		const GameObjectClass *object = units[i]->Get_Object();
		const GameObjectTypeClass *type = Get_Type_From_Combatant(units[i]);

		if (!mIsSpace && object->Behaves_Like(BEHAVIOR_PLANET))
		{
			//Use turrets to compute ground base strength
			const PlanetaryDataPackClass *planet_data = object->Get_Planetary_Data();
			ENFORCED_IF(planet_data)
			{
				int tactical_built_object_count = planet_data->Get_Persistent_Built_Tactical_Object_Count();
				for (int j = 0; j < tactical_built_object_count; ++j)
				{
					const GameObjectTypeClass *built_type = planet_data->Get_Persistent_Built_Tactical_Object_Type_At_Index(j);
					FAIL_IF(!built_type) { continue; }

					int cidx = 2;
					for (TargetContrastClass::ContrastType::const_iterator it = TargetContrastClass::ContrastTypeList.begin();
							it != TargetContrastClass::ContrastTypeList.end();
							it++, cidx++)
					{
						unsigned int category = it->first;
						if ((built_type->Get_Category_Mask() & category) != 0)
						{
							result[cidx].Force += built_type->Get_AI_Combat_Power_Metric();
							result[0].Force += built_type->Get_AI_Combat_Power_Metric();
							break;
						}
					}
				}
			}
			continue;
		}

		//Apply garrison units
		bool add_garrison = (object->Get_Parent_Mode_ID() == INVALID_OBJECT_ID);
		if (add_garrison && object->Get_Type()->Behaves_Like(BEHAVIOR_DUMMY_STAR_BASE) && mMidTactical)
		{
			add_garrison = false;
		}
		int tech_level = owner->Get_Tech_Level();
		int garrison_type_count = type->Get_Spawned_Unit_Type_Count(tech_level);
		if (add_garrison)
		{
			for (int j = 0; j < garrison_type_count; ++j)
			{
				const GameObjectTypeClass *garrison_type = type->Get_Spawned_Unit_Type(tech_level, j);
				FAIL_IF(!garrison_type) { continue; }

				int cidx = 2;
				for (TargetContrastClass::ContrastType::const_iterator it = TargetContrastClass::ContrastTypeList.begin();
						it != TargetContrastClass::ContrastTypeList.end();
						it++, cidx++)
				{

					unsigned int category = it->first;
					if ((unsigned int)garrison_type->Get_Category_Mask() & category)
					{
						float total_force = type->Get_Spawned_Unit_Starting_Count(tech_level, garrison_type) * garrison_type->Get_AI_Combat_Power_Metric();
						result[cidx].Force += total_force;
						result[mIsSpace ? 1 : 0].Force += total_force;
						break;
					}
				}
			}
		}

		if (type->Behaves_Like(BEHAVIOR_TRANSPORT) && mIsSpace) continue;

		if (type->Get_AI_Combat_Power_Metric() < weakest_val)
		{
			weakest_unit = units[i];
			weakest_val = type->Get_AI_Combat_Power_Metric();
		}

		int cidx = 2;
		for (TargetContrastClass::ContrastType::const_iterator it = TargetContrastClass::ContrastTypeList.begin();
				it != TargetContrastClass::ContrastTypeList.end();
				it++, cidx++)
		{

			unsigned int category = it->first;
			result[cidx].Category = category;
			result[cidx].Ground = !mIsSpace;
			if ((unsigned int)type->Get_Category_Mask() & category)
			{
				result[cidx].Force += type->Get_AI_Combat_Power_Metric();
				result[mIsSpace ? 1 : 0].Force += type->Get_AI_Combat_Power_Metric();
				break;
			}
		}
	}
}


/*
**	See documentation in AutoResolve.h
*/
HRESULT AutoResolveClass::Combat_Round(bool instant)
{
	if (!mIsCombatInitiated) {
		return(E_AUTORESOLVE_NOT_READY);
	}

	float elapsed_time = ((float)(FrameSynchronizer.Get_Current_Frame() - mStartFrame) / (float)FrameSynchronizer.Get_Logical_FPS());

	if (!mRetreatInProgress && !instant && elapsed_time < TheGameConstants.Get_Auto_Resolve_Display_Time() * 0.6666f)
	{
		return(S_OK);
	}

	float extra_time = TheGameConstants.Get_Auto_Resolve_Display_Time() * 0.6666f - elapsed_time;
	if (extra_time > 0.0f)
	{
		mStartFrame -= ((int)(extra_time * (float)FrameSynchronizer.Get_Logical_FPS()));
	}

	if (!mBattleFought)
	{
		float loser_attrition_value = mRetreatInProgress ? TheGameConstants.Get_Retreat_Auto_Resolve_Loser_Attrition() : 
			TheGameConstants.Get_Auto_Resolve_Loser_Attrition();
		float winner_attrition_value = mRetreatInProgress ? TheGameConstants.Get_Retreat_Auto_Resolve_Winner_Attrition() :
			TheGameConstants.Get_Auto_Resolve_Winner_Attrition();

		TargetContrastClass::ResultType results[2];
		Side_Attack(mSides[0].mQueue, mSides[1].mTotalForce, results[1], mSides[0].mOwnerID);
		Side_Attack(mSides[1].mQueue, mSides[0].mTotalForce, results[0], mSides[1].mOwnerID);

		int winner = Determine_Winner_Index(results[0], results[1]);
		int loser = (winner == 0 ? 1 : 0);
      
		PlayerClass *lplayer = PlayerList.Get_Player_By_ID(mSides[loser].mOwnerID);
		bool pirate_player = false;

		if (!lplayer->Get_Faction()->Is_Playable())
		{
			pirate_player = true;
		}

		mWinningPlayer = mSides[winner].mOwnerID;

		ICombatantBehaviorPtr wobj, lobj;
		wobj = mSides[winner].mQueue[0];
		lobj = mSides[loser].mQueue[0];

		if (Apply_Transport_Losses(mSides[loser].mQueue, pirate_player, wobj)) {
			Player_Retreats(mSides[loser].mOwnerID);
		}

		int global_index = mIsSpace ? 1 : 0;
		results[loser][global_index].Force += ((mSides[loser].mTotalForce[global_index].Force - results[loser][global_index].Force) * 
															(1.0f - loser_attrition_value));

		results[winner][global_index].Force += ((mSides[winner].mTotalForce[global_index].Force - results[winner][global_index].Force) * 
															(1.0f - winner_attrition_value));

		mSides[loser].mWeakestUnit.Release_Ref();
		if (pirate_player)
		{
			results[loser][global_index].Force = 0.0f;
		}

		for (int i = 0; i < ARRAY_SIZE(mSides); i++)
		{
			bool any_left = Apply_Attrition(mSides[i].mQueue, results[i], mSides[i].mWeakestUnit, i == loser, i, i == loser ? wobj : lobj);

			if (any_left && i == loser) 
				Player_Retreats(mSides[i].mOwnerID);
		}

		if (mSides[loser].mQueue.size() == 0 && mRetreatInProgress)
		{
			mRetreatInProgress = false;
			mRetreatingPlayer = -1;
		}

		if (!mIsSpace && mPlanet)
		{
			PlanetaryDataPackClass *planet_data = mPlanet->Get_Planetary_Data();
			ENFORCED_IF(planet_data)
			{
				planet_data->Fix_Built_Tactical_Object_Persistence_For_Winner(mSides[winner].mOwnerID);
			}
		}

		// MLL: If the battle is auto resolved with Luke and the Death Star, make sure that the victory condition is triggered if the Rebels win.
		if (mSides[loser].mSuperWeapon && mSides[winner].mSuperWeaponKiller)
		{
			if (GameModeManager.Get_Game_Mode_By_Sub_Type(SUB_GAME_MODE_GALACTIC) && GameModeManager.Get_Game_Mode_By_Sub_Type(SUB_GAME_MODE_GALACTIC)->Get_Victory_Monitor())
			{
				VictoryConditionClass victory(VICTORY_GALACTIC_SUPER_WEAPON_DESTRUCTION, mSides[winner].mOwnerID, 5);
				GameModeManager.Get_Game_Mode_By_Sub_Type(SUB_GAME_MODE_GALACTIC)->Get_Victory_Monitor()->Register_Pending_Victory(victory);
			}
		}

		mBattleFought = true;
	}

	if (!instant && elapsed_time < TheGameConstants.Get_Auto_Resolve_Display_Time())
	{
		return(S_OK);
	}

	if (mRetreatInProgress) {
		return(S_AUTORESOLVE_COMBAT_RETREAT);
	} else {
		return(S_AUTORESOLVE_COMBAT_OVER);
	}
}


/*
**	See documentation in AutoResolve.h
*/
HRESULT AutoResolveClass::Player_Retreats(int id)
{
	if (!mIsCombatInitiated) {
		return(E_AUTORESOLVE_NOT_READY);
	}
	if (mRetreatInProgress) {
		return(E_AUTORESOLVE_RETREATING);
	}

	mRetreatInProgress = true;
	mRetreatingPlayer = id;

	return(S_OK);
}


/*
**	See documentation in AutoResolve.h
*/
HRESULT AutoResolveClass::Cleanup_Combat(void)
{
	// TODO: Apply recorded damage to individual units if necessary

	if (mMidTactical == false)
	{
		if (mPlanet)
		{
			if (mIsSpace) {
				SignalDispatcherClass::Get().Send_Signal( mPlanet, PG_SIGNAL_OBJECT_SPACE_CONFLICT_END, NULL );
			} else {
				SignalDispatcherClass::Get().Send_Signal( mPlanet, PG_SIGNAL_OBJECT_GROUND_CONFLICT_END, NULL );
			}
		}
	}

	/*
	**	Signal that the fleets are no longer involved in combat. Their contents may already
	**	have been obliterated, but what the hey.
	*/
	for (GameObjectVectorType::iterator ptr = mFleets.begin(); ptr != mFleets.end(); ++ptr) {

		if (mMidTactical == false)
		{
			if (mIsSpace) {
				SignalDispatcherClass::Get().Send_Signal( *ptr, PG_SIGNAL_OBJECT_SPACE_CONFLICT_END, NULL );
			} else {
				SignalDispatcherClass::Get().Send_Signal( *ptr, PG_SIGNAL_OBJECT_GROUND_CONFLICT_END, NULL );
			}
		}
		
		(*ptr)->Release_Ref();
	}
	mFleets.clear();

	/*
	**	Release any reference locks on the objects used in the autoresolve.
	*/
	for (int index = 0; index < ARRAY_SIZE(mSides); ++index) {
		SideStruct * side = &mSides[index];

		side->Clear_Combat_Info();

		side->Remove_Combatant(side->mUnit);
		side->Remove_Combatant(side->mEscort);
//       for (unsigned i = 0; i < side->mQueue.size(); ++i) {
//          side->mQueue[i]->Allow_Delete(true);
//       }
		side->mQueue.clear();
		side->mOwnerID = -1;
		side->mUnitCount = 0;
	}

	mRetreatInProgress = false;
	mIsCombatPrepared = false;
	mIsCombatInitiated = false;

	/*
	** Free ref count on combat location
	*/
	mPlanet = NULL;

	return(S_OK);
}


/*
**	See documentation for ICombatantBehaviorClass
*/
int AutoResolveClass::Get_Side_A(void) const
{
	return(mSides[0].mOwnerID);
}


/*
**	See documentation for ICombatantBehaviorClass
*/
int AutoResolveClass::Get_Side_B(void) const
{
	return(mSides[1].mOwnerID);
}


/*
**	See documentation for ICombatantBehaviorClass
*/
int AutoResolveClass::Get_Side_Aggressor(void) const
{
	return(mAggressor);
}


/*
**	See documentation for ICombatantBehaviorClass
*/
int AutoResolveClass::Get_Side_Defender(void) const
{
	if (mSides[0].mOwnerID == mAggressor) {
		return(mSides[1].mOwnerID);
	} else {
		return(mSides[0].mOwnerID);
	}
}


/*
**	See documentation for ICombatantBehaviorClass
*/
float AutoResolveClass::Get_Health_Ratio(int) const 
{
	float elapsed_time = ((float)(FrameSynchronizer.Get_Current_Frame() - mStartFrame) / (float)FrameSynchronizer.Get_Logical_FPS());

	return 1.0f - (min(elapsed_time / (TheGameConstants.Get_Auto_Resolve_Display_Time() * 0.6666f), 1.0f));
}


/*
**	See documentation for ICombatantBehaviorClass
*/
int AutoResolveClass::Who_Won(void) const
{
	return(mWinningPlayer);
}


/*
**	See documentation for ICombatantBehaviorClass
*/
const ICombatantBehaviorClass * AutoResolveClass::Which_Escort_Fires_First(void) const
{
	if (mEscortFire == -1) return(NULL);
	const SideStruct * ptr = &mSides[mEscortFire];
	return (ptr->mEscort);
}


/*
**	See documentation for ICombatantBehaviorClass
*/
const ICombatantBehaviorClass * AutoResolveClass::Which_Unit_Fires_First(void) const
{
	if (mUnitFire == -1) return(NULL);
	const SideStruct * ptr = &mSides[mUnitFire];
	return (ptr->mUnit);
}


/*
**	See documentation for ICombatantBehaviorClass
*/
int AutoResolveClass::Side_Is_Retreating(void) const
{
	return(mRetreatingPlayer);
}


/*
**	See documentation for ICombatantBehaviorClass
*/
int AutoResolveClass::Get_Visible_Queue_Size(int side) const
{
	// TODO: return with size of visible queue -- player should be 5, opponent should be 1 unless hero influence
	side = side;

	return(3);
}


/*
**	See documentation for ICombatantBehaviorClass
*/
const ICombatantBehaviorClass * AutoResolveClass::Get_Queue(int side, unsigned position) const
{
	const SideStruct * sideptr = Get_Side_By_Owner(side);
	if (sideptr == NULL) return(NULL);

	if (position < sideptr->mQueue.size()) {
		return(sideptr->mQueue[position]);
	}
	return(NULL);
}


/*
**	See documentation for ICombatantBehaviorClass
*/
bool AutoResolveClass::Was_Participant_Damaged(const ICombatantBehaviorClass * participant) const
{
	for (int index = 0; index < ARRAY_SIZE(mSides); ++index) {
		if (mSides[index].mDamaged1 == participant) return(true);
		if (mSides[index].mDamaged2 == participant) return(true);
	}
	return(false);
}

/*
**	See documentation for ICombatantBehaviorClass
*/
bool AutoResolveClass::Was_Participant_Destroyed(const ICombatantBehaviorClass * participant) const
{
	for (int index = 0; index < ARRAY_SIZE(mSides); ++index) {
		if (mSides[index].mDestroyed1 == participant) return(true);
		if (mSides[index].mDestroyed2 == participant) return(true);
	}
	return(false);
}

/*
**	See documentation for ICombatantBehaviorClass
*/
bool AutoResolveClass::Was_Participant_Evacuated(const ICombatantBehaviorClass * participant) const
{
	for (int index = 0; index < ARRAY_SIZE(mSides); ++index) {
		if (mSides[index].mEvacuated1 == participant) return(true);
		if (mSides[index].mEvacuated2 == participant) return(true);
	}
	return(false);
}

/*
**	See documentation for ICombatantBehaviorClass
*/
const ICombatantBehaviorClass * AutoResolveClass::Get_Escort(int side) const
{
	const SideStruct * sideptr = Get_Side_By_Owner(side);
	if (sideptr == NULL) return(NULL);
	return(sideptr->mEscort);
}

/*
**	See documentation for ICombatantBehaviorClass
*/
const ICombatantBehaviorClass * AutoResolveClass::Get_Escort_Target(int side) const
{
	const SideStruct * sideptr = Get_Side_By_Owner(side);
	if (sideptr == NULL) return(NULL);
	return(sideptr->mEscortTarget);
}

/*
**	See documentation for ICombatantBehaviorClass
*/
const ICombatantBehaviorClass * AutoResolveClass::Get_Unit(int side) const
{
	const SideStruct * sideptr = Get_Side_By_Owner(side);
	if (sideptr == NULL) return(NULL);
	return(sideptr->mUnit);
}

/*
**	See documentation for ICombatantBehaviorClass
*/
const ICombatantBehaviorClass * AutoResolveClass::Get_Unit_Target(int side) const
{
	const SideStruct * sideptr = Get_Side_By_Owner(side);
	if (sideptr == NULL) return(NULL);

	return(sideptr->mUnitTarget);
}




int AutoResolveClass::Get_Side_Not_Retreating(void) const
{
	return((mSides[0].mOwnerID == mRetreatingPlayer) ? mSides[1].mOwnerID : mSides[0].mOwnerID);
}



//**********************************************************************************************
//! Adds an individual combatant to the fray
/*!
	This adds a combatant to the fray by placing it into the appropriate grouping (corner of
	the ring)

	@param	object		Pointer to the combatant to add to the fray. This object should not be
								a collection of combatants such as a fleet, but an actual combatant.
	@returns	Returns with the result code from the add operation.
	@retval	S_OK				The object was added to the fray without error
	@retval	S_FALSE			The object was not added, but not because of an error
*/
HRESULT AutoResolveClass::Add_Individual(ICombatantBehaviorPtr object, bool allow_all /*=false*/)
{
	if (object == NULL) return(E_POINTER);

	/*
	**	Determine if the unit can participate in space or land combat as the case may be.
	**	If the answer is no, then don't add the unit and return with a flag
	**	indicating this fact. This isn't a severe error condition since the object
	**	may participate in some combats if the conditions are right. The conditions just aren't
	**	right in this situation.
	*/
	if (mMidTactical == false && allow_all == false)
	{
		if (mIsSpace) {
			if (!object->Can_Participate_In_Space_Combat()) {
				return(S_FALSE);
			}
		} else {
			if (!object->Can_Participate_In_Land_Combat()) {
				return(S_FALSE);
			}
		}
	}

	/*
	**	Register (or confirm) that this side is a participant in the combat.
	*/
	HRESULT result = Owner_Enters_Fray(object->Get_Owner());
	assert(SUCCEEDED(result));
	if (FAILED(result)) return(result);

	/*
	**	Add the participant to the queue of the side that it belongs to.
	*/
	for (int index = 0; index < ARRAY_SIZE(mSides); ++index) {
		if (mSides[index].mOwnerID == object->Get_Owner()) {
			HRESULT result = mSides[index].Add_Combatant(object);
			assert(SUCCEEDED(result));
			if (FAILED(result)) return(result);
			break;
		}
	}
	return(S_OK);
}


//**********************************************************************************************
//! Adds or verifies a side that enters the fray
/*!
	This method will check the specified owner against the list of owners previosly recorded
	as participants in this combat. If a match could not be found, then the specified owner
	is added as a participant. As a side effect of this, an index into the mSides array is
	reserved for the owner in anticipation of combatants being added.

	@param	owner		The owning side that is confirmed to be a participant in the combat.
	@returns	Returns with the result code of the success or failure of this operation.
	@retval	S_OK		The side was confirmed or added as appropriate without error.
	@retval	E_AUTORESOLVE_TOO_MANY_SIDES	More than the maximum allowed sides are participating
														in this combat. This is an error and shouldn't
														occur due to other constraints on gameplay.
	@remarks	The maximum number of sides is inferred from the size of the mSides array.
*/
HRESULT AutoResolveClass::Owner_Enters_Fray(int owner)
{
	for (int index = 0; index < ARRAY_SIZE(mSides); ++index) {
		if (mSides[index].mOwnerID == owner) {
			return(S_OK);
		}
		if (mSides[index].mOwnerID == -1) {
			mSides[index].mOwnerID = owner;
			return(S_OK);
		}
	}
	return(E_AUTORESOLVE_TOO_MANY_SIDES);
}


void AutoResolveClass::Toggle_Escort_Fire(void)
{
	if (mEscortFire != -1) {
		mEscortFire = (mEscortFire == 0) ? 1 : 0;
	}
}


void AutoResolveClass::Toggle_Unit_Fire(void)
{
	if (mUnitFire != -1) {
		mUnitFire = (mUnitFire == 0) ? 1 : 0;
	}
}



// custom STL sort function so that higher priority combatants go first
static int gt(const ICombatantBehaviorPtr & lval, const ICombatantBehaviorPtr & rval)
{
	return (lval->Formation_Priority() > rval->Formation_Priority());
}


//**********************************************************************************************
//! Sorts the combat queue from largest to smallest
/*!
	This sorts the combatant queue so that larger objects appear first in the list. This is so
	that the player can get a sneak-peek at the kinds of units in the combat.

	@returns	Returns with the result code from the sorting operation.
	@retval	S_OK		The queue was sorted without error.
	@retval	S_FALSE	The queue is empty, so no sorting actually occurred.
*/
HRESULT AutoResolveClass::SideStruct::Sort_Queue(void)
{
	if (mQueue.empty()) return(S_FALSE);

	std::sort(mQueue.begin(), mQueue.end(), gt);

	return(S_OK);
}


int AutoResolveClass::SideStruct::Unit_Count(void) const
{
	int count = mQueue.size();
	if (mUnit != NULL && mUnit->Is_Alive()) ++count;
	if (mEscort != NULL && mEscort->Is_Alive()) ++count;
	return(count);
}


//**********************************************************************************************
//! Updates main and escort unit slots
/*!
	This moves combatants from the queue to the main position and escort positions of the
	battle. The main slot or escort slot may remain empty after this method if there are not
	suitable combatants that can fill that slot. However, both slots will not be empty if there
	are any combatants at all in the queue.

	@returns	Returns with success code of update position.
	@retval	S_OK			The position slots have been updated without error
	@retval	S_FALSE		No update can occur because there are no combatants in the queue
*/
HRESULT AutoResolveClass::SideStruct::Update_Positions(void)
{
	mNewUnit = false;
	mNewEscort = false;

	/*
	**	If the queue is empty, then no positions can be updated since there
	**	is nothing to draw upon.
	*/
	if (mQueue.empty()) return(S_FALSE);

	if (mUnit == NULL && mEscort == NULL) {
		mRepositionDelay = 0;
	}

	/*
	**	If it is not yet time to bring in reinforcements, then don't do
	**	it yet.
	*/
	if (mRepositionDelay > 0) {
		mRepositionDelay -= 1;
		return(S_FALSE);
	}

	/*
	**	If the main combatant slot is empty, then find a replacement that
	**	generally follows the rules that non escort type units go first and
	**	pick lower priority rated units before higher priority rated units.
	*/
	if (mUnit == NULL) {

		CombatantBehaviorPtrContainerType::iterator candidate = mQueue.end();
		for (CombatantBehaviorPtrContainerType::iterator ptr = mQueue.begin(); ptr != mQueue.end(); ++ptr) {
			if (candidate == mQueue.end()) {

				/*
				**	This prepares for the case that if no ideal candidate can be found, the first one in the
				**	queue is used.
				*/
				candidate = ptr;
			} else {

				/*
				**	Non escort units bump out escort units for the main unit slot, regardless
				**	of their priority.
				*/
				if ((*candidate)->Is_Escort() && !(*ptr)->Is_Escort()) {
					candidate = ptr;
					continue;
				}

				/*
				**	Lower priority units bump out higher priority units for the main unit slot,
				**	but will never bump out a non-escort unit type.
				*/
				if ((!(*ptr)->Is_Escort() || ((*ptr)->Is_Escort() && (*candidate)->Is_Escort())) &&
					(*candidate)->Formation_Priority() > (*ptr)->Formation_Priority()) {
					candidate = ptr;
					continue;
				}
			}
		}
		assert(candidate != mQueue.end());		// mQueue both empty and not empty!?!
		mUnit = (*candidate);
		assert(mUnit->Is_Alive());
		mNewUnit = true;
		mQueue.erase(candidate);
	}

	/*
	**	If the escort slot is empty, then pick a replacement following the general
	**	guidelines that only escort units can fill the slot and lower priority
	**	rated combatants are chosen first.
	*/
	if (mEscort == NULL && !mQueue.empty()) {
		CombatantBehaviorPtrContainerType::iterator candidate = mQueue.end();
		for (CombatantBehaviorPtrContainerType::iterator ptr = mQueue.begin(); ptr != mQueue.end(); ++ptr) {

			/*
			**	Only escort capable units are allowed to be considered.
			*/
			if (!(*ptr)->Is_Escort()) {
					continue;
			}

			/*
			**	If this is the first escort type unit found, then it is the best so far -- record
			**	it and continue.
			*/
			if (candidate == mQueue.end()) {
				candidate = ptr;
				continue;
			}

			/*
			**	Lower priority escorts are chosen over higher priority ones.
			*/
			if ((*candidate)->Formation_Priority() > (*ptr)->Formation_Priority()) {
				candidate = ptr;
			}
		}

		/*
		**	It is possible that the escort slot was not filled. This can occur if there are
		**	units in the fleet that cannot fill the escort position.
		*/
		if (candidate != mQueue.end()) {
			mEscort = (*candidate);
			assert(mEscort->Is_Alive());
			mNewEscort = true;
			mQueue.erase(candidate);
		}
	}

	// verify that there is a combatant at the ready -- this routine should have ensured this as true
	assert(mEscort != NULL || mUnit != NULL);

	return(S_OK);
}


//**********************************************************************************************
//! Adds combatant to the queue.
/*!
	This adds the specified combatant to the queue of available combatants for the autoresolve
	process.

	@param	combatant	The candidate combatant to add to the queue.
	@returns	Returns with the result code from the adding process.
	@retval	S_OK								Added the combatant without error.
	@retval	E_AUTORESOLVE_ADDING_TWICE	The combatant is already in the queue!
*/
HRESULT AutoResolveClass::SideStruct::Add_Combatant(ICombatantBehaviorPtr combatant)
{
	/*
	**	Check to see if the object to add is already in the list of participants. It
	**	is an error condition if this is true.
	*/
	CombatantBehaviorPtrContainerType::iterator fnd = std::find(mQueue.begin(), mQueue.end(), combatant);
	assert(fnd == mQueue.end());
	if (fnd != mQueue.end()) return(E_AUTORESOLVE_ADDING_TWICE);

	/*
	**	Add the combatant -- at the end -- for speed reasons
	*/
	mQueue.insert(mQueue.end(), combatant);
	mUnitCount++;
	//combatant->Allow_Delete(false);
	return(S_OK);
}


AutoResolveClass::SideStruct * AutoResolveClass::Get_Side_By_Owner(int owner)
{
	for (int index = 0; index < ARRAY_SIZE(mSides); ++index) {
		if (mSides[index].mOwnerID == owner) {
			return(&mSides[index]);
		}
	}
	return(NULL);
}


const AutoResolveClass::SideStruct * AutoResolveClass::Get_Side_By_Owner(int owner) const
{
	for (int index = 0; index < ARRAY_SIZE(mSides); ++index) {
		if (mSides[index].mOwnerID == owner) {
			return(&mSides[index]);
		}
	}
	return(NULL);
}


void AutoResolveClass::SideStruct::Clear_Combat_Info(void)
{
	mEscortTarget.Release_Ref();
	mUnitTarget.Release_Ref();
	Remove_Combatant(mDestroyed1);
	Remove_Combatant(mDestroyed2);
	Remove_Combatant(mEvacuated1);
	Remove_Combatant(mEvacuated2);
	mDamaged1.Release_Ref();
	mDamaged2.Release_Ref();
	mWeakestUnit.Release_Ref();
	mSuperWeapon.Release_Ref();
	mSuperWeaponKiller.Release_Ref();
}

void AutoResolveClass::SideStruct::Init(void)
{
	mOwnerID = -1;
	mUnitCount = 0;
	mRepositionDelay = 0;
	mNewUnit = false;
	mNewEscort = false;
	Clear_Combat_Info();
	mEscort.Release_Ref();
	mEscortTarget.Release_Ref();
	mUnit.Release_Ref();
	mUnitTarget.Release_Ref();
}

void AutoResolveClass::SideStruct::Remove_Combatant(ICombatantBehaviorPtr combatant)
{
	if (combatant != NULL) {
		if (combatant == mUnit) mUnit.Release_Ref();
		if (combatant == mEscort) mEscort.Release_Ref();
		assert(mDestroyed1 == NULL || !mDestroyed1->Is_Alive());
		if (combatant == mDestroyed1) mDestroyed1.Release_Ref();
		assert(mDestroyed2 == NULL || !mDestroyed2->Is_Alive());
		if (combatant == mDestroyed2) mDestroyed2.Release_Ref();
		if (combatant == mDamaged1) mDamaged1.Release_Ref();
		if (combatant == mDamaged2) mDamaged2.Release_Ref();
		if (combatant == mEvacuated1) mEvacuated1.Release_Ref();
		if (combatant == mEvacuated2) mEvacuated2.Release_Ref();
		//combatant->Allow_Delete(true);
	}
}


bool AutoResolveClass::SideStruct::Has_Retreat_Protection(bool space) const
{
	if (space) {
		if (mUnit && mUnit->Is_Alive() && mUnit->Get_Space_Retreat_Immunity()) {
			return(true);
		}
		if (mEscort && mEscort->Is_Alive() && mEscort->Get_Space_Retreat_Immunity()) {
			return(true);
		}

		for (CombatantBehaviorPtrContainerType::const_iterator ptr = mQueue.begin(); ptr != mQueue.end(); ++ptr) {
			ICombatantBehaviorPtr combatant = (*ptr);
			if (combatant != NULL && combatant->Is_Alive() && combatant->Get_Space_Retreat_Immunity()) {
				return(true);
			}
		}
	} else {
		if (mUnit && mUnit->Is_Alive() && mUnit->Get_Land_Retreat_Immunity()) {
			return(true);
		}
		if (mEscort && mEscort->Is_Alive() && mEscort->Get_Land_Retreat_Immunity()) {
			return(true);
		}

		for (CombatantBehaviorPtrContainerType::const_iterator ptr = mQueue.begin(); ptr != mQueue.end(); ++ptr) {
			ICombatantBehaviorPtr combatant = (*ptr);
			if (combatant != NULL && combatant->Is_Alive() && combatant->Get_Land_Retreat_Immunity()) {
				return(true);
			}
		}
	}
	return(false);
}


float AutoResolveClass::SideStruct::Defense_Bonus(bool space) const
{
	float defense = 0.0f;
	if (space) {
		if (mUnit && mUnit->Is_Alive()) {
			float d = mUnit->Get_Space_Combat_Defensive_Bonus();
			if (d > defense) defense = d;
		}
		if (mEscort && mEscort->Is_Alive()) {
			float d = mEscort->Get_Space_Combat_Defensive_Bonus();
			if (d > defense) defense = d;
		}

		for (CombatantBehaviorPtrContainerType::const_iterator ptr = mQueue.begin(); ptr != mQueue.end(); ++ptr) {
			ICombatantBehaviorPtr combatant = (*ptr);
			if (combatant != NULL && combatant->Is_Alive()) {
				float d = combatant->Get_Space_Combat_Defensive_Bonus();
				if (d > defense) defense = d;
			}
		}
	} else {
		if (mUnit && mUnit->Is_Alive()) {
			float d = mUnit->Get_Land_Combat_Defensive_Bonus();
			if (d > defense) defense = d;
		}
		if (mEscort && mEscort->Is_Alive()) {
			float d = mEscort->Get_Land_Combat_Defensive_Bonus();
			if (d > defense) defense = d;
		}

		for (CombatantBehaviorPtrContainerType::const_iterator ptr = mQueue.begin(); ptr != mQueue.end(); ++ptr) {
			ICombatantBehaviorPtr combatant = (*ptr);
			if (combatant != NULL && combatant->Is_Alive()) {
				float d = combatant->Get_Land_Combat_Defensive_Bonus();
				if (d > defense) defense = d;
			}
		}
	}
	return(defense);
}


bool AutoResolveClass::Show_Damage_For(float oldratio, float newratio)
{
	if (oldratio > 0.75f && newratio <= 0.75f) return(true);
	if (oldratio > 0.5f && newratio <= 0.5f) return(true);
	if (oldratio > 0.25f && newratio <= 0.25f) return(true);
	return(false);
}


bool AutoResolveClass::Escort_Can_Fire(const SideStruct * side) const
{
	assert(side != NULL);

	return(side->mEscort != NULL && side->mEscort->Is_Alive() && (!mRetreatInProgress || side->mOwnerID != mRetreatingPlayer));
}

bool AutoResolveClass::Unit_Can_Fire(const SideStruct * side) const
{
	assert(side != NULL);

	return(side->mUnit != NULL && side->mUnit->Is_Alive() && (!mRetreatInProgress || side->mOwnerID != mRetreatingPlayer));
}



HRESULT AutoResolveClass::Escort_Fire(SideStruct * side, SideStruct * otherside)
{
	if (Escort_Can_Fire(side)) {
		// pick target
		ICombatantBehaviorPtr target = otherside->mEscort;
		if (target == NULL || !target->Is_Alive()) target = otherside->mUnit;

		// assign target to mEscortTarget
		if (target != NULL) {
			side->mEscortTarget = target;

			if (target->Is_Alive()) {
				float oldratio = target->Get_Health_Ratio();
				float adj = 1.0f;
				if (!mIsSpace && otherside->mOwnerID == mPlanetOwner) {
					adj = TheGameConstants.Get_Default_Defense_Adjust();
				}
				adj = adj * (1 - otherside->Defense_Bonus(mIsSpace));
				side->mEscort->Fire_Upon(target, adj);
				float newratio = target->Get_Health_Ratio();
				if (target->Is_Alive()) {
					if (Show_Damage_For(oldratio, newratio)) {
						// TODO: This should be replaced with a more elegant solution
						if (otherside->mDamaged1 == NULL || otherside->mDamaged1 == target) {
							otherside->mDamaged1 = target;
						} else if (otherside->mDamaged2 == NULL || otherside->mDamaged2 == target) {
							otherside->mDamaged2 = target;
						}
					}
				} else {

//					Debug_Print("Killed: %s\n", target->Get_Object_Type()->Get_Name()->c_str());

					if (otherside->mDestroyed1 == NULL || otherside->mDestroyed1 == target) {
						otherside->mDestroyed1 = target;
					} else if (otherside->mDestroyed2 == NULL || otherside->mDestroyed2 == target) {
						otherside->mDestroyed2 = target;
					} else {
						assert(false);
					}
				}
			}
		}
	}

	if (side == &mSides[0]) {
		mEscortFire = 1;
	} else {
		mEscortFire = 0;
	}
	return(S_OK);
}



HRESULT AutoResolveClass::Unit_Fire(SideStruct * side, SideStruct * otherside)
{
	if (Unit_Can_Fire(side)) {
		// pick target
		ICombatantBehaviorPtr target = otherside->mEscort;
		if (target == NULL || !target->Is_Alive()) target = otherside->mUnit;

		// TODO: Pick best target

		// assign target to mUnitTarget
		if (target != NULL) {
			side->mUnitTarget = target;

			if (target->Is_Alive()) {

				/*
				**	Determine if the firing is aborted due to stun effect.
				*/
				bool stunned = false;
				if (side->mUnit->Get_Class_Type() == CLASS_CAPITAL) {
					if (mPlanetOwner == side->mOwnerID && mStunRoundCounter == 1) {
						stunned = true;
						mStunRoundCounter = mIonCannon;
					} else {
						mStunRoundCounter -= 1;
					}
				}

				/*
				**	Check for and apply any hypervelocity gun damage to the ship.
				*/
				if (side->mUnit->Get_Class_Type() == CLASS_CAPITAL) {
					if (mPlanetOwner == side->mOwnerID && mHitRoundCounter == 1) {

						/*
						**	Apply a hypervelocity gun hit to the ship.
						*/
						side->mUnit->Take_Damage(DAMAGE_MISC, (float)mHyperDamage, side->mUnit);
						if (!side->mUnit->Is_Alive()) {
							if (side->mDestroyed1 == NULL || side->mDestroyed1 == side->mUnit) {
								side->mDestroyed1 = side->mUnit;
							} else if (side->mDestroyed2 == NULL || side->mDestroyed2 == side->mUnit) {
								side->mDestroyed2 = side->mUnit;
							} else {
								assert(false);
							}

							Update_Battle_History(side->mUnit->Get_Object());
						}

						mHitRoundCounter = mHyperGun;
					} else {
						mHitRoundCounter -= 1;
					}
				}

				if (side->mUnit->Is_Alive() && !stunned) {
					float oldratio = target->Get_Health_Ratio();
					float adj = 1.0f;
					if (!mIsSpace && otherside->mOwnerID == mPlanetOwner) {
						adj = TheGameConstants.Get_Default_Defense_Adjust();
					}
					adj = adj * (1.0f-otherside->Defense_Bonus(mIsSpace));
					side->mUnit->Fire_Upon(target, adj);
					float newratio = target->Get_Health_Ratio();
					if (target->Is_Alive()) {
						if (Show_Damage_For(oldratio, newratio)) {
							// TODO: This should be replaced with a more elegant solution
							if (otherside->mDamaged1 == NULL || otherside->mDamaged1 == target) {
								otherside->mDamaged1 = target;
							} else if (otherside->mDamaged2 == NULL || otherside->mDamaged2 == target) {
								otherside->mDamaged2 = target;
							}
						}
					} else {

	//					Debug_Print("Killed: %s\n", target->Get_Object_Type()->Get_Name()->c_str());

						if (otherside->mDestroyed1 == NULL || otherside->mDestroyed1 == target) {
							otherside->mDestroyed1 = target;
						} else if (otherside->mDestroyed2 == NULL || otherside->mDestroyed2 == target) {
							otherside->mDestroyed2 = target;
						} else {
							assert(false);
						}
					}
				}
			}
		}
	}

	if (side == &mSides[0]) {
		mUnitFire = 1;
	} else {
		mUnitFire = 0;
	}
	return(S_OK);
}



int AutoResolveClass::Get_Is_Active(void) const
{
	return(mIsCombatPrepared);
}			  

const GameObjectTypeClass *AutoResolveClass::Get_Type_From_Combatant(ICombatantBehaviorPtr combatant)
{
	const GameObjectClass *object = combatant->Get_Object();
	if (object->Behaves_Like(BEHAVIOR_PLANET))
	{
		return combatant->Get_Object_Type();
	}

	const GameObjectTypeClass *type = object->Get_Original_Object_Type();

	//Get a unique container type if relevant
	if (type->Behaves_Like(BEHAVIOR_TRANSPORT))
	{
		DynamicVectorClass<GameObjectClass*> *contained_units = object->Get_Transport_Contents();
		if (contained_units)
		{
			for (int i = 0; i < contained_units->Size(); ++i)
			{
				GameObjectClass *land_unit = contained_units->Get_At(i);
				FAIL_IF(!land_unit) { continue; }
				if (mIsSpace && land_unit->Get_Type()->Get_Unique_Space_Container_Unit())
				{
					type = land_unit->Get_Type()->Get_Unique_Space_Container_Unit();
					break;
				}
				else if (!mIsSpace && land_unit->Get_Type()->Get_Unique_Ground_Container_Unit())
				{
					type = land_unit->Get_Type()->Get_Unique_Ground_Container_Unit();
					break;
				}
			}
		}
	}

	return type;
}

int AutoResolveClass::Determine_Winner_Index(TargetContrastClass::ResultType &results_a, TargetContrastClass::ResultType &results_b)
{
	// Death Star hack.  Death star always wins unless Luke is on the opposition.
	if (mSides[0].mSuperWeapon && !mSides[1].mSuperWeaponKiller) 
	{
		Player_Retreats(mSides[1].mOwnerID);
	} 
	else if (mSides[1].mSuperWeapon && !mSides[0].mSuperWeaponKiller) 
	{
		Player_Retreats(mSides[0].mOwnerID);
	}

	if (mRetreatInProgress)
	{
		return mRetreatingPlayer == mSides[0].mOwnerID ? 1 : 0;
	}
	else
	{
		float total_a = 0.0f;
		bool any_positive_a = false;
		for (unsigned int i = 0; i < results_a.size(); ++i)
		{
			if (results_a[i].Force > 0.0f)
			{
				any_positive_a = true;
				total_a += results_a[i].Force;
			}
		}

		float total_b = 0.0f;
		bool any_positive_b = false;
		for (unsigned int i = 0; i < results_b.size(); ++i)
		{
			if (results_b[i].Force > 0.0f)
			{
				any_positive_b = true;
				total_b += results_b[i].Force;
			}
		}

		if (any_positive_a && any_positive_b)
		{
			//Draw (sort of).
			PlayerClass *player_0 = PlayerList.Get_Player_By_ID(mSides[0].mOwnerID);
			PlayerClass *player_1 = PlayerList.Get_Player_By_ID(mSides[1].mOwnerID);
	
			//If there's a human involved then go by force remaining.  Otherwise we award the win
			//to the AI that's controlling the playable faction
			if (!player_0 || player_0->Is_Human() || 
					!player_1 || player_1->Is_Human())
			{
				return (total_a > total_b ? 0 : 1);
			}
			else if (!player_0->Get_Faction()->Is_Playable())
			{
				return 1;
			}
			else if (!player_1->Get_Faction()->Is_Playable())
			{
				return 0;
			}
			else
			{
				return (total_a > total_b ? 0 : 1);
			}
		}
		else if ((any_positive_a || any_positive_b) && total_a != total_b)
		{
			return (total_a > total_b ? 0 : 1);
		}
		else
		{
			return (mAggressor == mSides[0].mOwnerID ? 0 : 1);
		}
	}
}

void AutoResolveClass::Apply_Unit_Contrast(float &remaining_power, const GameObjectTypeClass *type, TargetContrastClass::ResultType &current, 
															int best_category, const std::vector<float> &factor_table, MapEnvironmentType terrain)
{
	FAIL_IF(!type) { return; }
	unsigned int ctype = static_cast<unsigned int>(type->Get_Category_Mask());
	int cval = GET_FIRST_BIT_SET(ctype);

	float original_power = remaining_power;

	float factor = 0.0;
	// search for the best factor category that this unit belongs too.
	while (cval > -1) 
	{
		if (factor_table[cval] > factor) 
		{
			factor = (factor_table[cval] + 1.0f);
		}

		// turn off that bit
		ctype &= (~(1 << cval));
		cval = GET_FIRST_BIT_SET(ctype);
	}

	if (factor != 0.0f)
	{
		remaining_power *= factor;
	}

	if (best_category > 0)
	{
		float contrast_weight = TargetContrastClass::Get_Average_Contrast_Factor(type, NULL, current[best_category].Category);
		if (current[best_category].Ground && terrain != MAP_TYPE_INVALID)
		{
			contrast_weight *= TargetContrastClass::Get_Effectiveness_On_Terrain(type, terrain);
		}
		remaining_power *= contrast_weight;

		float modified_force_applied = Min(current[best_category].Force, remaining_power);
		remaining_power = (1.0f - modified_force_applied / remaining_power) * original_power;

		current[best_category].Force -= modified_force_applied;
		
		for (unsigned int i = 0; i < current.size(); ++i)
		{
			if (current[i].Category == GAME_OBJECT_CATEGORY_NONE &&
					current[i].Ground == current[best_category].Ground)
			{
				current[i].Force -= (modified_force_applied + remaining_power);
				break;
			}
		}
	}
	else
	{
		bool ground = (type->Get_Num_Ground_Company_Units() > 0 || type->Behaves_Like(BEHAVIOR_DUMMY_GROUND_BASE));

		for (unsigned int i = 0; i < current.size(); ++i)
		{
			if (current[i].Category == GAME_OBJECT_CATEGORY_NONE &&
					current[i].Ground == ground)
			{
				current[i].Force -= remaining_power;
				break;
			}
		}
		
		remaining_power = 0.0f;
	}
}
