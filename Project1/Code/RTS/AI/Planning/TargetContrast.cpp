// $Id: //depot/Projects/StarWars_Steam/FOC/Code/RTS/AI/Planning/TargetContrast.cpp#1 $
///////////////////////////////////////////////////////////////////////////////////////////////////
//
// (C) Petroglyph Games, Inc.
//
//
//  *****           **                          *                   *
//  *   **          *                           *                   *
//  *    *          *                           *                   *
//  *    *          *     *                 *   *          *        *
//  *   *     *** ******  * **  ****      ***   * *      * *****    * ***
//  *  **    *  *   *     **   *   **   **  *   *  *    * **   **   **   *
//  ***     *****   *     *   *     *  *    *   *  *   **  *    *   *    *
//  *       *       *     *   *     *  *    *   *   *  *   *    *   *    *
//  *       *       *     *   *     *  *    *   *   * **   *   *    *    *
//  *       **       *    *   **   *   **   *   *    **    *  *     *   *
// **        ****     **  *    ****     *****   *    **    ***      *   *
//                                          *        *     *
//                                          *        *     *
//                                          *       *      *
//                                      *  *        *      *
//                                      ****       *       *
//
///////////////////////////////////////////////////////////////////////////////////////////////////
// C O N F I D E N T I A L   S O U R C E   C O D E -- D O   N O T   D I S T R I B U T E
///////////////////////////////////////////////////////////////////////////////////////////////////
//
//              $File: //depot/Projects/StarWars_Steam/FOC/Code/RTS/AI/Planning/TargetContrast.cpp $
//
//    Original Author: Brian Hayes
//
//            $Author: Brian_Hayes $
//
//            $Change: 637819 $
//
//          $DateTime: 2017/03/22 10:16:16 $
//
//          $Revision: #1 $
//
///////////////////////////////////////////////////////////////////////////////////////////////////
/** @file */

#pragma hdrstop
#include "TargetContrast.h"
#include "AI/AIPlayer.h"
#include "AI/AITargetLocation.h"
#include "AI/TacticalAIManager.h"
#include "AI/Planning/PlanDefinition.h"
#include "AI/Perception/AIPerceptionSystem.h"
#include "AI/Perception/PerceptionContext.h"
#include "AI/Perception/Evaluators/PerceptionEvaluationState.h"
#include "GameObjectCategoryType.h"
#include "DynamicEnum.h"
#include "LuaScript.h"
#include "UtilityCommands.h"
#include "GameObject.h"
#include "GameObjectType.h"
#include "PlanetaryBehavior.h"
#include "AI/LuaScript/Commands/WeightedTypeList.h"
#include "AI/LuaScript/Commands/GlobalCommands.h"
#include "XML.h"
#include "AI/AILog.h"
#include "MapEnvironmentTypeConverter.h"
#include "GameObjectTypeManager.h"
#include "AI/Learning/AILearningSystem.h"
#include "DifficultyAdjustment.h"

float TargetContrastClass::MinContrastFactor = 0.0;
float TargetContrastClass::MaxContrastFactor = 0.0;
TargetContrastClass::ContrastType TargetContrastClass::ContrastTypeList;
TargetContrastClass::TerrainEffectivenessTableType TargetContrastClass::TerrainEffectiveness;

/**
 * System Initialize
 * @since 8/26/2004 11:39:15 AM -- BMH
 */
bool TargetContrastClass::System_Initialize(void)
{
	return true;
}

/**
 * System Shutdown
 * @since 8/26/2004 11:39:15 AM -- BMH
 */
void TargetContrastClass::System_Shutdown(void)
{
	ContrastTypeList.clear();
}

/**
 * Read in the Rock-Paper-Scissors values from the PGBaseDefinitions.lua
 * file.
 * 
 * @param global_factor
 * @param ctype_factors
 * @param result
 * @since 7/5/2004 6:24:17 PM -- BMH
 */
void TargetContrastClass::Parse_Lua_Contrast_String(float &min_factor,
																	 float &max_factor,
																	 ContrastType & result)
{

	SmartPtr<LuaScriptClass> script = LuaScriptClass::Create_Script("PGAICommands");
	FAIL_IF(!script) return;

	UtilityCommandsClass::Register_Commands(script);
	GlobalCommandsClass::Register_Commands(script);

	LuaBool::Pointer bval = new LuaBool(true);
	script->Map_Global_To_Lua(bval, "PlanDefinitionLoad");
	script->Call_Function("Base_Definitions", NULL);

	LuaNumber::Pointer maxscale = LUA_SAFE_CAST(LuaNumber, script->Map_Global_From_Lua("MaxContrastScale"));
	LuaNumber::Pointer minscale = LUA_SAFE_CAST(LuaNumber, script->Map_Global_From_Lua("MinContrastScale"));
	LuaTable::Pointer enemy = LUA_SAFE_CAST(LuaTable, script->Map_Global_From_Lua("EnemyContrastTypes"));
	LuaTable::Pointer contrast = LUA_SAFE_CAST(LuaTable, script->Map_Global_From_Lua("FriendlyContrastTypes"));

	assert(minscale && maxscale && enemy && contrast);

	unsigned int eval = 0;
	result.clear();

	for (int i = 0; i < (int)enemy->Value.size(); i++) {
		LuaString::Pointer estr = LUA_SAFE_CAST(LuaString, enemy->Value[i]);
		SmartPtr<WeightedTypeListClass> ctypes = LUA_SAFE_CAST(WeightedTypeListClass, contrast->Value[i]);
		if (!estr || !ctypes) break;
		if (i >= (int)contrast->Value.size()) break;

		TheGameObjectCategoryTypeConverterPtr->String_To_UInt(estr->Value.c_str(), eval);

		result.insert(std::make_pair(eval, ctypes));
	}

	min_factor = minscale->Value;
	max_factor = maxscale->Value;

	script->Shutdown();
}

void TargetContrastClass::Init_Contrast_Type_List(void)
{
	if (ContrastTypeList.size() == 0) 
	{
		Parse_Lua_Contrast_String(MinContrastFactor, MaxContrastFactor, ContrastTypeList);
	}
}

/**
 * Build an opposing force estimate based on the plan definition target contrast
 * values and our notion of threat at the target.
 * 
 * @param ai_player AIPlayer that the opposing force will be estimated for.
 * @param target    the AI Target location.
 * @param def       Plan definition for the plan to attack the target.
 * @param result    result of the calculation and list of opposing forces.
 * @since 7/5/2004 6:26:45 PM -- BMH
 */
void TargetContrastClass::Build_Contrast_List(AIPlayerClass *ai_player, 
															 AITargetLocationClass *target,
															 PlanDefinitionClass *def,
															 ResultType & result)
{
	PG_PROFILE("Build_Contrast_List");

	if (ContrastTypeList.size() == 0) 
	{
		Parse_Lua_Contrast_String(MinContrastFactor, MaxContrastFactor, ContrastTypeList);
	}

	float gfactor = MaxContrastFactor;	
	if (def)
	{
		gfactor = def->Get_Contrast_Max_Factor();
		TacticalAIManagerClass *manager = ai_player->Get_Tactical_Manager_By_Mode(GameModeManager.Get_Sub_Type());
		gfactor += def->Get_Per_Failure_Contrast_Adjust() * manager->Get_Learning_System()->Get_Recent_Failures(def, target);
		gfactor *= ai_player->Get_Difficulty_Adjustment()->Get_Galactic_AI_Contrast_Multiplier();
	}
	const TargetContrastClass::ContrastType &ctypelist = def ? def->Get_Contrast_Type_List() : ContrastTypeList;

	result.resize(0);
	float ground_force = Get_Force_Perception(ai_player, target, 0.0, true);
	float space_force = Get_Force_Perception(ai_player, target, 0.0, false);
	
	result.push_back(ContrastForceStruct(0, ground_force, true));
	result.push_back(ContrastForceStruct(0, space_force, false));

	if (ground_force > 0.0) 
	{
		for (ContrastType::const_iterator i = ctypelist.begin(); i != ctypelist.end(); ++i)
		{
			ground_force = Get_Force_Perception(ai_player, target, AIPerceptionSystemClass::Enum_Value_To_Parameter_Value(i->first), true);
			ground_force *= gfactor;
			result.push_back(ContrastForceStruct(i->first, ground_force, true));
		}
	}
	else
	{
		for (ContrastType::const_iterator i = ctypelist.begin(); i != ctypelist.end(); ++i)
		{
			result.push_back(ContrastForceStruct(i->first, 0.0f, true));
		}
	}

	if (space_force > 0.0) 
	{
		for (ContrastType::const_iterator i = ctypelist.begin(); i != ctypelist.end(); ++i)
		{
			space_force = Get_Force_Perception(ai_player, target, AIPerceptionSystemClass::Enum_Value_To_Parameter_Value(i->first), false);
			space_force *= gfactor;
			result.push_back(ContrastForceStruct(i->first, space_force, false));
		}
	}
	else
	{
		for (ContrastType::const_iterator i = ctypelist.begin(); i != ctypelist.end(); ++i)
		{
			result.push_back(ContrastForceStruct(i->first, 0.0f, false));
		}
	}
}

/**
 * Build an opposing force estimate based on the plan definition target contrast
 * values and our notion of threat at the target.
 * 
 * @param ai_player AIPlayer that the opposing force will be estimated for.
 * @param target    the AI Target location.
 * @param def       Plan definition for the plan to attack the target.
 * @param result    result of the calculation and list of opposing forces.
 * @param surrounding_forces_only
 *                  Query force of the target or the threat around the target?
 * @since 9/17/2004 2:50:46 PM -- BMH
 */
void TargetContrastClass::Build_Space_Contrast_List(AIPlayerClass *ai_player, 
                                                    AITargetLocationClass *target,
                                                    PlanDefinitionClass *def,
                                                    ResultType & result,
                                                    bool surrounding_forces_only)
{
	PG_PROFILE("Build_Space_Contrast_List");

	if (ContrastTypeList.size() == 0) 
	{
		Parse_Lua_Contrast_String(MinContrastFactor, MaxContrastFactor, ContrastTypeList);
	}

	SubGameModeType mode = target->Get_Perception_System()->Get_Manager()->Get_Game_Mode();
	bool ground = (mode == SUB_GAME_MODE_LAND);

	float gfactor = MaxContrastFactor;	
	if (def)
	{
		gfactor = def->Get_Contrast_Max_Factor();
		TacticalAIManagerClass *manager = ai_player->Get_Tactical_Manager_By_Mode(GameModeManager.Get_Sub_Type());
		gfactor += def->Get_Per_Failure_Contrast_Adjust() * manager->Get_Learning_System()->Get_Recent_Failures(def, target);
		const DifficultyAdjustmentClass *difficulty = ai_player->Get_Difficulty_Adjustment();
		gfactor *= mode == SUB_GAME_MODE_LAND ? difficulty->Get_Land_AI_Contrast_Multiplier() : difficulty->Get_Space_AI_Contrast_Multiplier();
	}
	const TargetContrastClass::ContrastType &ctypelist = def ? def->Get_Contrast_Type_List() : ContrastTypeList;

	result.resize(0);
	float force = Get_Space_Force_Perception(ai_player, target, 0.0, surrounding_forces_only);

	result.push_back(ContrastForceStruct(0, force, ground));

	if (force > 0.0f)
	{
		for (ContrastType::const_iterator i = ctypelist.begin(); i != ctypelist.end(); ++i)
		{
			force = Get_Space_Force_Perception(ai_player, target, AIPerceptionSystemClass::Enum_Value_To_Parameter_Value(i->first), surrounding_forces_only);
			force *= gfactor;
			result.push_back(ContrastForceStruct(i->first, force, ground));
		}
	}
	else
	{
		for (ContrastType::const_iterator i = ctypelist.begin(); i != ctypelist.end(); ++i)
		{
			result.push_back(ContrastForceStruct(i->first, 0.0f, ground));
		}
	}
}

/**
 * Query the perception system for the amount of threat at the target.
 * 
 * @param ai_player AIPlayer that the opposing force will be estimated for.
 * @param target    the AI Target location.
 * @param category  category type to query for.
 * @param surrounding_forces_only
 *                  Query force of the target or the threat around the target?
 * @return unnormalized force value
 * @since 9/17/2004 2:51:58 PM -- BMH
 */
float TargetContrastClass::Get_Space_Force_Perception(AIPlayerClass *ai_player, 
															  AITargetLocationClass *target, 
															  float category, 
															  bool surrounding_forces_only)
{
	if ( !ai_player ) 
		return 0.0;
	
	assert(GameModeManager.Get_Sub_Type() != SUB_GAME_MODE_GALACTIC);

	TacticalAIManagerClass *manager = ai_player->Get_Tactical_Manager_By_Mode( GameModeManager.Get_Sub_Type() );
	AIPerceptionSystemClass *perception_system = manager->Get_Perception_System();

	// build a perception context for the query we're about to make		
	PerceptionContextClass context( perception_system );
	context.Add_Context( PERCEPTION_TOKEN_VARIABLE_TARGET, target->Get_Evaluator() );
	context.Set_Player(ai_player->Get_Player());

	// Variable_Target.EnemyForce.SpaceTotalUnnormalized
	static PerceptionTokenVector tokens;
	tokens.resize(0);
	tokens.push_back( PERCEPTION_TOKEN_VARIABLE_TARGET );
	if (surrounding_forces_only && target->Get_Target_Game_Object()) {
		tokens.push_back( PERCEPTION_TOKEN_LOCATION );
		tokens.push_back( PERCEPTION_TOKEN_ENEMY_FORCE_UNNORMALIZED );
	} else if (target->Get_Target_Game_Object()) {
		tokens.push_back( PERCEPTION_TOKEN_FORCE_UNNORMALIZED );
	} else {
		tokens.push_back( PERCEPTION_TOKEN_ENEMY_FORCE_UNNORMALIZED );
	}
				
	PerceptionEvaluationStateClass evaluation_state( &context, tokens );
	// category is the enum value of the category we wish to query.
	if (category != 0.0) {
		evaluation_state.Add_Parameter(PERCEPTION_TOKEN_PARAMETER_CATEGORY, category);
	}
				
	// make the query
	float query_result = 0.0;
	perception_system->Evaluate_Perception( evaluation_state, query_result );

	return query_result;
}

/**
 * Query the perception system for the amount of threat at the target.
 * 
 * @param ai_player AIPlayer that the opposing force will be estimated for.
 * @param target    the AI Target location.
 * @param category  category type to query for.
 * @param ground_forces
 *                  Query for ground or space force?
 * 
 * @return unnormalized force value
 * @since 7/5/2004 6:27:16 PM -- BMH
 */
float TargetContrastClass::Get_Force_Perception(AIPlayerClass *ai_player, 
															  AITargetLocationClass *target, 
															  float category, 
															  bool ground_forces)
{
	if ( !ai_player ) 
		return 0.0;
	
	TacticalAIManagerClass *manager = ai_player->Get_Tactical_Manager_By_Mode( SUB_GAME_MODE_GALACTIC );
	AIPerceptionSystemClass *perception_system = manager->Get_Perception_System();

	// build a perception context for the query we're about to make		
	PerceptionContextClass context( perception_system );
	context.Add_Context( PERCEPTION_TOKEN_VARIABLE_TARGET, target->Get_Evaluator() );
	context.Set_Player(ai_player->Get_Player());

	// Variable_Target.EnemyForce.SpaceTotalUnnormalized
	static PerceptionTokenVector tokens;
	tokens.resize(0);
	tokens.push_back( PERCEPTION_TOKEN_VARIABLE_TARGET );
	tokens.push_back( PERCEPTION_TOKEN_ENEMY_FORCE );
	tokens.push_back( ground_forces ? PERCEPTION_TOKEN_GROUND_TOTAL_UNNORMALIZED : 
							PERCEPTION_TOKEN_SPACE_TOTAL_UNNORMALIZED );
				
	PerceptionEvaluationStateClass evaluation_state( &context, tokens );
	// category is the enum value of the category we wish to query.
	if (category != 0.0) {
		evaluation_state.Add_Parameter(PERCEPTION_TOKEN_PARAMETER_CATEGORY, category);
	}
				
	// make the query
	float query_result = 0.0;
	perception_system->Evaluate_Perception( evaluation_state, query_result );

	//Add in the base if the category matches
	PlanetaryBehaviorClass *planet_behavior = static_cast<PlanetaryBehaviorClass*>(target->Get_Target_Game_Object()->Get_Behavior(BEHAVIOR_PLANET));

	if (planet_behavior->Get_Allegiance().Is_Ally(ai_player->Get_Player()))
	{
		return query_result;
	}

	int category_as_int = AIPerceptionSystemClass::Parameter_Value_To_Enum_Value(category);
	if (category_as_int == 0)
	{
		category_as_int = GAME_OBJECT_CATEGORY_ALL;
	}

	if (ground_forces)
	{
		PlanetaryDataPackClass *planet_data = target->Get_Target_Game_Object()->Get_Planetary_Data();
		FAIL_IF( planet_data == NULL )	{ return query_result; }

		for (int i = 0; i < planet_data->Get_Ground_Special_Structures().Size(); ++i)
		{
			GameObjectClass *structure = planet_data->Get_Ground_Special_Structures().Get_At(i);
			FAIL_IF(!structure) { continue; }

			if ((structure->Get_Type()->Get_Category_Mask() & category_as_int) == 0)
			{
				continue;
			}

			query_result += structure->Get_Type()->Get_AI_Combat_Power_Metric();
		}

		query_result += planet_data->Get_Persistent_Built_Tactical_Object_Combat_Power(category_as_int);
	}
	else
	{
		const GameObjectTypeClass *base_type = planet_behavior->Get_Current_Starbase_Type(target->Get_Target_Game_Object());

		if (base_type)
		{
			if ((category_as_int & base_type->Get_Category_Mask()) != 0)
			{
				query_result += base_type->Get_AI_Combat_Power_Metric();
			}
		}
	}

	return query_result;
}

float TargetContrastClass::Get_Best_Contrast_Factor(const GameObjectTypeClass *type, PlanDefinitionClass *def, unsigned int category)
{
	const TargetContrastClass::ContrastType &ctypelist = def ? def->Get_Contrast_Type_List() : 
																					ContrastTypeList;
	ContrastType::const_iterator it = ctypelist.find(category);
	if (it == ctypelist.end())
	{
		return 0.0f;
	}

	WeightedTypeListClass *weight_list = it->second;

	float best_weight = 0.0f;
	for (int i = 0; i < weight_list->Get_Weight_Count(); ++i)
	{
		//Hand back the weight immediately if this type is mentioned explicitly
		if (type == weight_list->Get_Type_At_Index(i))
		{
			return weight_list->Get_Weight_At_Index(i);
		}

		//Otherwise use the best category based weight
		if (type->Get_Category_Mask() & weight_list->Get_Category_At_Index(i))
		{
			best_weight = Max(best_weight, weight_list->Get_Weight_At_Index(i));
		}
	}
	return best_weight;
}

float TargetContrastClass::Get_Average_Contrast_Factor(const GameObjectTypeClass *type, PlanDefinitionClass *def, unsigned int category)
{
	const TargetContrastClass::ContrastType &ctypelist = def ? def->Get_Contrast_Type_List() : 
																					ContrastTypeList;
	ContrastType::const_iterator it = ctypelist.find(category);
	if (it == ctypelist.end())
	{
		return 0.0f;
	}

	WeightedTypeListClass *weight_list = it->second;

	float total_weight = 0.0;
	int weight_count = 0;
	bool matches_contrast = false;
	for (int i = 0; i < weight_list->Get_Weight_Count(); ++i)
	{
		//Hand back the weight immediately if this type is mentioned explicitly
		if (type == weight_list->Get_Type_At_Index(i))
		{
			return weight_list->Get_Weight_At_Index(i);
		}

		//Otherwise use the best category based weight
		if (type->Get_Category_Mask() & weight_list->Get_Category_At_Index(i))
		{
			matches_contrast = true;
			//Ignore weights of 1.  If we don't put them in we'll be unable to know that land units
			//can't attack space units (and vice versa), but we don't want to bias our RPS adjustment
			//factor.
			if (weight_list->Get_Weight_At_Index(i) != 1.0f)
			{
				total_weight += weight_list->Get_Weight_At_Index(i);
				++weight_count;
			}
		}
	}
	if (weight_count == 0)
	{
		if (matches_contrast)
		{
			return 1.0f;
		}
		else
		{
			return 0.0f;
		}
	}
	else
	{
		return total_weight / weight_count;
	}
}

/**************************************************************************************************
* TargetContrastClass::Get_Effectiveness_On_Terrain -- Get the power multiplier for a unit type on a
*	particular terrain type
*
* In:			
*
* Out:		
*
* History: 11/23/2004 1:48PM JSY
**************************************************************************************************/
float TargetContrastClass::Get_Effectiveness_On_Terrain(const GameObjectTypeClass *type, MapEnvironmentType terrain_type)
{
	TerrainEffectivenessTableType::const_iterator per_terrain_table = TerrainEffectiveness.find(terrain_type);
	if (per_terrain_table != TerrainEffectiveness.end())
	{
		MovementClassType move_type = MOVEMENT_CLASS_INVALID;
		if (type->Get_Num_Ground_Company_Units() > 0)
		{
			move_type = type->Get_Ground_Company_Unit(0)->Get_Movement_Class();
		}
		else
		{
			move_type = type->Get_Movement_Class();
		}
		EffectivenessType::const_iterator effectiveness = per_terrain_table->second.find(move_type);
		if (effectiveness != per_terrain_table->second.end())
		{
			return effectiveness->second;
		}
	}

	return 1.0;
}

/**************************************************************************************************
* TargetContrastClass::Parse_Terrain_Effectiveness -- Read the AI unit power adjustments based on movement class
*	vs terrain type from XML
*
* In:			
*
* Out:		
*
* History: 11/23/2004 1:48PM JSY
**************************************************************************************************/
void TargetContrastClass::Parse_Terrain_Effectiveness()
{
	std::string filename = ".\\Data\\XML\\AITerrainEffectiveness.xml";

	XMLDatabase terrain_effectiveness_db;

	HRESULT result = terrain_effectiveness_db.Read(filename);
	if (!SUCCEEDED(result))
	{
		AIERROR( ("Failed to open xml file %s for reading unit terrain effectiveness.", filename.c_str()) );
		return;
	}

	//Format has all movement types listed for each terrain type
	while (SUCCEEDED(result))
	{
		std::string key_name;
		result = terrain_effectiveness_db.Get_Key(&key_name);
		if (!SUCCEEDED(result))
		{
			AIERROR( ("Error encountered reading xml file %s.", filename.c_str()) );
			continue;
		}

		MapEnvironmentType terrain_type;
		if (!TheMapEnvironmentTypeConverterPtr->String_To_Enum(key_name, terrain_type))
		{
			AIERROR( ("Unrecognized map environment type %s.", key_name.c_str()) );
			continue;
		}

		result = terrain_effectiveness_db.Descend();

		//Got the terrain, now the movement classes
		while (SUCCEEDED(result))
		{
			result = terrain_effectiveness_db.Get_Key(&key_name);
			if (!SUCCEEDED(result))
			{
				AIERROR( ("Error encountered reading xml file %s.", filename.c_str()) );
				continue;
			}

			MovementClassType move_type;
			if (!TheMovementClassTypeConverterPtr->String_To_Enum(key_name, move_type))
			{
				AIERROR( ("Unrecognized movement class %s.", key_name.c_str()) );
				continue;
			}

			result = terrain_effectiveness_db.Get_Value(&key_name);
			if (!SUCCEEDED(result))
			{
				AIERROR( ("Error encountered reading xml file %s.", filename.c_str()) );
				continue;
			}

			float effectiveness = static_cast<float>(atof(key_name.c_str()));

			if (effectiveness < 0.0)
			{
				AIERROR( ("Effectiveness for movement class %s in environment %s is less than 0.  This is silly.", TheMovementClassTypeConverterPtr->Enum_To_String(move_type).c_str(), TheMapEnvironmentTypeConverterPtr->Enum_To_String(terrain_type).c_str()) );
				effectiveness = 0.0;
			}

			TerrainEffectiveness[terrain_type].insert(std::make_pair(move_type, effectiveness));

			result = terrain_effectiveness_db.Next_Node();
		}

		terrain_effectiveness_db.Ascend();
		result = terrain_effectiveness_db.Next_Node();
	}
}