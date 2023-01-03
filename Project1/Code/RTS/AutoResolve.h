/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  P E T R O G L Y P H   G A M E S, inc.        ***
 ***********************************************************************************************/
/** @file
 * 
 *		This file contains the auto-resolve class declaration.
 */
//#pragma once

#ifndef AUTO_RESOLVE_H
#define AUTO_RESOLVE_H

#include	"IAutoResolve.h"

#include "AI/Planning/TargetContrast.h"
#include	"GameObject.h"
#include	<vector>

#define MAX_HISTORY 8

struct AutoResolveKilled
{
	const GameObjectTypeClass *Object;
	int								Owner;
};

typedef std::vector<AutoResolveKilled> AutoResolveKilledVector;
typedef std::vector<std::pair<const GameObjectTypeClass *, int> > KilledListType;

struct AutoResolveBattle
{
		const GameObjectTypeClass *Planet;
		KilledListType Killed;
};

class AutoResolveClass : public IAutoResolveClass
{
	public:
		AutoResolveClass(void) : mIsCombatPrepared(false), mIsCombatInitiated(false), mRetreatInProgress(false),
										 mWinningPlayer(-1), mIonCannon(false), mHyperGun(false), mBattleID(-1) {}
		~AutoResolveClass(void) {Cleanup_Combat();}

	/*
	**	These methods are used by the system to start, carry out, and terminate an autoresolve
	**	combat.
	*/
	public:

		virtual HRESULT Prepare_For_Space(void);
		virtual HRESULT Prepare_For_Land(void);
		virtual HRESULT Add_Combatant(GameObjectClass * object);
		virtual HRESULT Initiate_Combat(int aggressor);
		virtual HRESULT Player_Retreats(int id);
		virtual HRESULT Combat_Round(bool instant);
		virtual HRESULT Cleanup_Combat(void);
		virtual void Add_Tactical_Combatants(void);
		virtual void Kill_Retreating_Units(void);


		bool Apply_Attrition(std::vector<ICombatantBehaviorPtr> &units, TargetContrastClass::ResultType &current, ICombatantBehaviorPtr &weakest_unit, 
									bool is_loser, int index, ICombatantBehaviorPtr killer);
		bool Apply_Transport_Losses(std::vector<ICombatantBehaviorPtr> &units, bool is_pirate, ICombatantBehaviorPtr killer);
      void Find_Contrast_Index(float remaining_power, const GameObjectTypeClass *type, const TargetContrastClass::ResultType &current, 
											int &best_category);
		void Apply_Unit_Contrast(float &remaining_power, const GameObjectTypeClass *type, TargetContrastClass::ResultType &current, 
											int best_category, const std::vector<float> &factor_table, MapEnvironmentType terrain);
		void Side_Attack(std::vector<ICombatantBehaviorPtr> &units, TargetContrastClass::ResultType &target_force, TargetContrastClass::ResultType &result, int player_id);
		void Calculate_Side_Force(std::vector<ICombatantBehaviorPtr> &units, TargetContrastClass::ResultType &result, 
                                int player_id, ICombatantBehaviorPtr &weakest_unit);
		void Find_Special_Heroes(int index);
		void Update_Battle_History(const GameObjectClass *object);

		virtual int Who_Won(void) const;
		virtual const ICombatantBehaviorClass * Get_Escort(int side) const;
		virtual const ICombatantBehaviorClass * Get_Escort_Target(int side) const;
		virtual const ICombatantBehaviorClass * Get_Unit(int side) const;
		virtual const ICombatantBehaviorClass * Get_Unit_Target(int side) const;
		virtual const ICombatantBehaviorClass * Get_Queue(int side, unsigned position) const;
		virtual bool Was_Participant_Damaged(const ICombatantBehaviorClass * participant) const;
		virtual bool Was_Participant_Destroyed(const ICombatantBehaviorClass * participant) const;
		virtual bool Was_Participant_Evacuated(const ICombatantBehaviorClass * participant) const;
		virtual int Get_Visible_Queue_Size(int side) const;
		virtual float Get_Health_Ratio(int side) const;
		virtual int Side_Is_Retreating(void) const;
		virtual const ICombatantBehaviorClass * Which_Escort_Fires_First(void) const;
		virtual const ICombatantBehaviorClass * Which_Unit_Fires_First(void) const;
		virtual int Get_Side_A(void) const;
		virtual int Get_Side_B(void) const;
		virtual int Get_Side_Aggressor(void) const;
		virtual int Get_Side_Defender(void) const;
		virtual int Get_Is_Active(void) const;

		virtual int Get_Battle_ID(void) const { return mBattleID; }
		virtual AutoResolveBattle* Get_Battle(int id) { return &mBattleHistory[id]; }

	private:
		/*!
		**	Each of the sides in the conflict is managed by one of these structures. It keeps track of who is
		**	engaged, who is in the queue, and what player owns the group.
		*/
		typedef struct SideStruct {
			SideStruct(void) : mOwnerID(-1), mUnitCount(0), mRepositionDelay(0), mNewUnit(false), mNewEscort(false) {}
			void Init(void);
			HRESULT Update_Positions(void);
			HRESULT Sort_Queue(void);
			HRESULT Add_Combatant(ICombatantBehaviorPtr combatant);
			int Unit_Count(void) const;
			void Clear_Combat_Info(void);
			bool New_Unit(void) const {return(mNewUnit);}
			bool New_Escort(void) const {return(mNewEscort);}
			void Remove_Combatant(ICombatantBehaviorPtr combatant);
			bool Has_Retreat_Protection(bool space) const;
			float Defense_Bonus(bool space) const;

			int mOwnerID;									//!< The owning player ID for these participants
			int mUnitCount;								//!< Original total number of ship/squadrons in fight
			int mRepositionDelay;						//!< Combat rounds to delay replacing lost front line units
			ICombatantBehaviorPtr mUnit;				//!< The main combat unit
			bool mNewUnit;
			ICombatantBehaviorPtr mEscort;			//!< The escorting combat unit
			bool mNewEscort;
			ICombatantBehaviorPtr mSuperWeapon;			
			ICombatantBehaviorPtr mSuperWeaponKiller;	
			TargetContrastClass::ResultType mTotalForce;
			CombatantBehaviorPtrContainerType mQueue;	//!< Combat units in the queue
			ICombatantBehaviorPtr mWeakestUnit;
			ICombatantBehaviorPtr mEscortTarget;
			ICombatantBehaviorPtr mUnitTarget;
			ICombatantBehaviorPtr mDestroyed1,mDestroyed2;
			ICombatantBehaviorPtr mDamaged1,mDamaged2;
			ICombatantBehaviorPtr mEvacuated1,mEvacuated2;
		} SideStruct;

		HRESULT Owner_Enters_Fray(int owner);
		HRESULT Add_Individual(ICombatantBehaviorPtr object, bool allow_all = false);
		HRESULT Apply_Results(SideStruct & side);
		const SideStruct * Get_Side_By_Owner(int owner) const;
		SideStruct * Get_Side_By_Owner(int owner);
		void Toggle_Escort_Fire(void);
		void Toggle_Unit_Fire(void);
		static bool Show_Damage_For(float oldratio, float newratio);
		int Get_Side_Not_Retreating(void) const;
		bool Escort_Can_Fire(const SideStruct * side) const;
		bool Unit_Can_Fire(const SideStruct * side) const;
		HRESULT Escort_Fire(SideStruct * side, SideStruct * otherside);
		HRESULT Unit_Fire(SideStruct * side, SideStruct * otherside);
		const GameObjectTypeClass *Get_Type_From_Combatant(ICombatantBehaviorPtr combatant);
		int Determine_Winner_Index(TargetContrastClass::ResultType &results_a, TargetContrastClass::ResultType &results_b);

		bool mIsCombatPrepared;		//!< Combat has been given a space or land context

		bool mIsCombatInitiated;	//!< Firing has begun

		bool mRetreatInProgress;	//!< Indicates if retreat is in progress

		int mAggressor;				//!< The player that is initiating the conflict
		int mRetreatingPlayer;		//!< The player that is retreating

		int mWinningPlayer;			//!< The player that won the combat

		int mEscortFire;				//!< Which escort fires first (mSide index)
		int mUnitFire;					//!< Which unit fires first (mSide index);

		int mIonCannon;				//!< Stun rate against invaders
		int mStunRoundCounter;		//!< Tracker for determining when stun effect occurs
		int mHyperGun;					//!< Hypergun fire rate against invaders
		int mHyperDamage;				//!< Damage to apply per hypergun fire
		int mHitRoundCounter;		//!< Tracker for determining when hit effect occurs

		int mPlanetOwner;				//!< Who owns the planet and thus benefits from special weapons

		int mStartFrame;
		bool mBattleFought;

		/*!
		**	Is this combat taking place in space?  If not then the presumption is on ground.
		*/
		bool mIsSpace;
		bool mMidTactical;
		int mBombingRun;
		const GameObjectTypeClass *mBomberType;



		MapEnvironmentType mTerrainType;
		GameObjectCategoryType mTransportCategory;

		/*!
		**	This records the planet (system) that the battle is taking place on or over.
		*/
		GameObjectClassPtr mPlanet;

		/*!
		**	All parent fleet objects that were added to this autoresolve
		**	are recorded here so that they may be properly "signaled"
		**	when the conflict ends.
		*/
		GameObjectVectorType mFleets;

		/*!
		**	This array lists the sides that are participating in the combat. The number of sides that can
		**	participate in combat is controlled by the size of this array. Since the display system
		**	is probably hard coded to a maximum number of sides, increasing the size of this
		**	array should not be done lightly.
		*/
		SideStruct mSides[2];

		// Battle history logging
		int 						mBattleID;
		AutoResolveBattle		mBattleHistory[MAX_HISTORY];
};

#endif AUTO_RESOLVE_H
		
		