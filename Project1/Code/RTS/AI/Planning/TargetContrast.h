// $Id: //depot/Projects/StarWars_Steam/FOC/Code/RTS/AI/Planning/TargetContrast.h#1 $
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
//              $File: //depot/Projects/StarWars_Steam/FOC/Code/RTS/AI/Planning/TargetContrast.h $
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


// given a goal and target_contrast_scalar, evaluate the target unit composition.
// for each unit category
//		get contrast unit category
// 
//
// the lua plan can specify our minimum target contrast.  1.0 would be an equal match.
// 
// change lua Definitions function to call Base_Definitions then Base_Definitions will call Definitions.


#ifndef __TARGET_CONTRAST_H__
#define __TARGET_CONTRAST_H__



#include "LuaScriptVariable.h"
#include "GameMode.h"
#include "MovementClassType.h"
#include "MapEnvironmentTypes.h"

/**
 * Forward Declarations
 */
class AIPlayerClass;
class AITargetLocationClass;
class PlanDefinitionClass;
class WeightedTypeListClass;
class GameObjectTypeClass;

using namespace std;

class TargetContrastClass
{
public:

	struct ContrastForceStruct
	{
		ContrastForceStruct() : Category(0), Force(0.0), Ground(false) {}
		ContrastForceStruct(unsigned int category, float force, bool ground) : Category(category), Force(force), Ground(ground) {}

		unsigned int	Category;
		float				Force;
		bool				Ground;
	};

	typedef stdext::hash_map<MovementClassType, float> EffectivenessType;
	typedef stdext::hash_map<MapEnvironmentType, EffectivenessType> TerrainEffectivenessTableType;
	typedef stdext::hash_map<unsigned int, SmartPtr<WeightedTypeListClass> > ContrastType;
	typedef std::vector<ContrastForceStruct> ResultType;


	static void Parse_Lua_Contrast_String(float &min_factor, float &max_factor, ContrastType & result);
	static void Parse_Terrain_Effectiveness();
	static void Init_Contrast_Type_List(void);
	static void Build_Contrast_List(AIPlayerClass *ai_player, AITargetLocationClass *target, PlanDefinitionClass *def, ResultType & result);
	static void Build_Space_Contrast_List(AIPlayerClass *ai_player, AITargetLocationClass *target, PlanDefinitionClass *def, ResultType & result, bool surrounding_forces_only);
	static float Get_Force_Perception(AIPlayerClass *ai_player, AITargetLocationClass *target, float category, bool ground_forces);
	static float Get_Space_Force_Perception(AIPlayerClass *ai_player, AITargetLocationClass *target, float category, bool surrounding_forces_only);

	static float Get_Best_Contrast_Factor(const GameObjectTypeClass *type, PlanDefinitionClass *def, unsigned int category);
	static float Get_Average_Contrast_Factor(const GameObjectTypeClass *type, PlanDefinitionClass *def, unsigned int category);

	static float Get_Effectiveness_On_Terrain(const GameObjectTypeClass *type, MapEnvironmentType terrain_type);

	static bool System_Initialize(void);
	static void System_Shutdown(void);

	static float MinContrastFactor;
	static float MaxContrastFactor;
	static ContrastType ContrastTypeList;
	static TerrainEffectivenessTableType TerrainEffectiveness;
};





#endif // __TARGET_CONTRAST_H__
