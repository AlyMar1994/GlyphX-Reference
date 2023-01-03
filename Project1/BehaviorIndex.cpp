/*!
** List of possible object behaviors. Each object behavior (interface) is identified
** with one of these values. In COM terms, this is equivalent to the UUID of the 
** interface. There can be one or more BehaviorClass objects that implement an 
** interface, so don't confuse these identifiers as if they identified actual
** specific derived BehaviorClass objects. [The locomotor behavior is a good example
** of one interface ID being implemented differently in multiple BehaviorClass objects.]
**
** @remarks
** These particular interface identifiers have a special property. For any specific 
** instance of a GameObject,there will never be more than one BehaviorClass object 
** associated for any of these interfaces. In other words, there may be interfaces 
** not available in a GameObject, but never will there be duplicate
** implementations of a "behavior" interface for the same GameObject. This property, as well
** as the fact that these interface identifiers start at zero, means that certain 
** efficiency shortcuts can be used, such as keeping a bitfield record of supported 
** behavior interfaces.
*/
typedef enum {
BEHAVIOR_NONE = -1,
//!< Magic ID for "no behavior" (useful in some instances)
BEHAVIOR_FIRST = 0,
//!< ID of first behavior interface

BEHAVIOR_GAME_OBJECT = BEHAVIOR_FIRST,
//!< Common interface to all game objects
BEHAVIOR_LOCO,
//!< Locomotor control (generic)
BEHAVIOR_SELECTABLE,
//!< Selectable behavior
BEHAVIOR_PLANET,
//!< Planetary control
BEHAVIOR_FLEET,
//!< Fleet behavior (not movement)
BEHAVIOR_TRANSPORT,
//!< Transportation behavior
BEHAVIOR_PRODUCTION,
//!< Factory production
BEHAVIOR_IDLE,
//!< Idle behavior, mostly for doing random idle animations over time
BEHAVIOR_PROJECTILE,
//!< A straight-moving weapon emission - like a bullet or a laser burst
BEHAVIOR_TARGETING,
//!< Gives an object the ability to find and track another target object - uses TacticalCombatant data pack
BEHAVIOR_WEAPON,
//!< Gives an object the ability to track weapons for attacking - uses TacticalCombatant data pack
BEHAVIOR_TURRET,
//!< Gives an object the ability to control turret(s) - uses TacticalCombatant data pack
BEHAVIOR_PARTICLE,
//!< A game object that represents an in-world object particle effect
BEHAVIOR_SURFACE_FX,
   //!< Gives an object an ability to have  surface effects (dynamic tracks, particles etc)
BEHAVIOR_WIND_DISTURBANCE,
//!< The object can introduce wind disturbances to the scene
BEHAVIOR_ENERGY_POOL,
//!< The object has an energy pool that recharges itself over time (the energy can be consumed by weapons/shields/engines etc)
BEHAVIOR_SHIELD,
//!< The object has shields which absorb damage and recharge themselves over time
BEHAVIOR_MARKER,                          //!< 
BEHAVIOR_UNIT_AI,
//!< The object interacts with the AI as an individual
BEHAVIOR_COMBATANT,
//!< Combatant behavior (part of autoresolve)
BEHAVIOR_LAND_OBSTACLE,                   //!< A simple pathfinding obstacle in land mode
BEHAVIOR_SQUAD_LEADER,                    //!< A squad leader object
BEHAVIOR_TEAM,
//!< A container behavior for team members
BEHAVIOR_WALL,
//!< 
BEHAVIOR_REVEAL,
//!< Reveals the fog of war around this unit
BEHAVIOR_HIDE_WHEN_FOGGED,
//!< This object doesn't render when it is inside the fog of war
BEHAVIOR_SPAWN_SQUADRON,
//!< This object can spawn reinforcement space fighter squadrons as needed
BEHAVIOR_SPAWN_INDIGENOUS_UNITS,
//!< This object can spawn Indigenous units as needed
BEHAVIOR_DEPLOY_TROOPERS,
//!< AT-AT ability to deploy stormtrooper companies
BEHAVIOR_BARRAGE_AREA,
//!< Keep an area under constant fire...
BEHAVIOR_LANDING,
//!< Deployed units should land correctly
BEHAVIOR_REPLENISH_WINGMEN,
//<! Darth Vader Space
BEHAVIOR_INDIGENOUS_UNIT,
//!< This object is an indigenous unit
BEHAVIOR_HUNT,
//!< Autonomous behavior that sends units out looking for enemies to engage.
BEHAVIOR_GUARD_SPAWN_HOUSE,
//!< Autonomous behavior that makes unit attack any enemy nearby and sends it back to the spawn house if no enemy is found in range
BEHAVIOR_LURE,
BEHAVIOR_EXPLOSION,
//!< This object is an explosion
BEHAVIOR_TREAD_SCROLL,
//!< 
BEHAVIOR_SQUASH, 
  //!< This object has the ability to squash squashable objects to death
BEHAVIOR_AMBIENT_SFX,
//!< This object has the ability to emit ambient SFXEvents
BEHAVIOR_DEBRIS,
//!< This object behaves as a moving piece of debris, with an optional lifetime - used for broken off hard points but can have other applications 
BEHAVIOR_TACTICAL_BUILD_OBJECTS,
//!< This object can build other object in tactical modes
BEHAVIOR_TACTICAL_UNDER_CONSTRUCTION,
//!< This object can be under construction in tactical, by an object with the TACTICAL_BUILD_OBJECTS behavior
BEHAVIOR_TACTICAL_SELL,
//!< This objects was built during tactical and can be sold back for credits
BEHAVIOR_TACTICAL_SUPER_WEAPON,
//!< This object is a special case tactical superweapon that obliterates other objects, not unsing conventional targeting and weapons systems
BEHAVIOR_BURNING,
//!< It burns! It burns! ow!
BEHAVIOR_ASTEROID_FIELD_DAMAGE,
BEHAVIOR_SPECIAL_WEAPON,
//!< This object behaves like a special weapon (ex. Planetary Ion Cannon) during specific tactical modes.
//!< The behavior should only be attached for those modes in which it is valid for this object to behave like the special weapon.
BEHAVIOR_TELEKINESIS_TARGET,
//!< Objects with this behavior can be the target of a telekinesis effect (causes them to levitate off the ground and optionally take damage)
BEHAVIOR_EARTHQUAKE_TARGET,
//!< Objects with this behavior can be the target of an Earthquake Attack effect (causes them to shake and optionally take damage)
BEHAVIOR_VEHICLE_THIEF,
//!< Objects with this behavior can steal enemy vehicles (this controls the object as it walks up to the vehicle and takes it over)
BEHAVIOR_DEMOLITION,
//!< Objects with this behavior (and the Demolition special ability) can plant explosives on buildings and do massive damage
BEHAVIOR_BOMB,
//!< This behavior is used by BEHAVIOR_DEMOLITION to coordinate the application of damage to a target
BEHAVIOR_JETPACK_LOCOMOTOR,
BEHAVIOR_TRANSPORT_LANDING,
//!< This behavior controls the transport drop-off sequence in land mode
BEHAVIOR_LOBBING_SUPERWEAPON,             //!< This behavior is for super weapons with lobbing projectiles. A different kind of directed targeting behavior
BEHAVIOR_STUNNABLE,
//!< This behavior allows the object to be stunned
BEHAVIOR_BASE_SHIELD,
//!< The behavior that implements the logic behind base shields (ie. shields created by the Base Shield Generator structure)
BEHAVIOR_GRAVITY_CONTROL_FIELD,
//!< This behavior allows the object to emit a land-based gravity control field when it is powered
BEHAVIOR_SKY_DOME,
//!< Sky dome behavior, does things like update its sun billboard to match the current lighting.
BEHAVIOR_CASH_POINT,
//!< Cash point behavior gives a cash bonus to the player inside the cash point radius
BEHAVIOR_CAPTURE_POINT,
//!< Generic Capture point behavior.  Changes ownership based on objects in range.
BEHAVIOR_DAMAGE_TRACKING,
//!< Keeps track of damage taken by an object.  For querying by other behaviors and AI.
BEHAVIOR_TERRAIN_TEXTURE_MODIFICATION,    //!< This object can modify the texture of the terrain beneath it >
BEHAVIOR_IMPOSING_PRESENCE,
//!< Presence of this object modifies the idle behavior of others nearby and may cause audio triggers
BEHAVIOR_HARASS,
BEHAVIOR_AVOID_DANGER,
//!< Scout ability - avoid danger(move away from attacking objects)
BEHAVIOR_SELF_DESTRUCT,
//!< Countdowns for certain amount of time and self-destructs
BEHAVIOR_DEATH,
//!< A special "cloned" game object hat goes through some kind of death, created at time of death
//!< This behavior is automatically attached to "clone" at death of original object, and not assigned in XML data
//!< WARNING: Do not attach DEATH behavior to normal game object types
BEHAVIOR_ION_STUN_EFFECT,
//!< Babysits the particles displayed for the ion cannon stun effect (from the planetary ion cannon)
BEHAVIOR_ABILITY_COUNTDOWN,
//!< Allows objects to have per-ability countdown timers so that they activated intermittently
BEHAVIOR_AFFECTED_BY_SHIELD,
//!< Objects with this behavior can be the target of a telekinesis effect (causes them to levitate off the ground and optionally take damage)
BEHAVIOR_NEBULA,
//!< objects with this behavior are affected by nebulas
BEHAVIOR_REINFORCEMENT_POINT,
//!< Enables reinforcementsn within some range
BEHAVIOR_HINT,
//!< Generic Hint behavior that can be set per-object instance
BEHAVIOR_INVULNERABLE,
//!< Makes the object immune to damage, usually attached dynamically during gameplay as the cause of an effect
BEHAVIOR_SPACE_OBSTACLE,
/// Allows the object to occupy one or more layers of the obstacle tracking system
BEHAVIOR_MULTIPLAYER_BEACON,
//!< This is a multiplayer beacon object, used for teammates to communicate visually with each other.
BEHAVIOR_HOLSTER_WEAPON,
// Handles drawing a weapon and putting it away

BEHAVIOR_GARRISON_VEHICLE,
// Allows vehicles to be garrisoned.
BEHAVIOR_GARRISON_STRUCTURE,
// Allows structures to be garrisoned.
BEHAVIOR_GARRISON_UNIT,
// Allows units to garrison vehicles or structures.

BEHAVIOR_DISABLE_FORCE_ABILITIES,
// Disables the force abilites of any unit within range.
BEHAVIOR_CONFUSE,
BEHAVIOR_INFECTION,
BEHAVIOR_PROXIMITY_MINE,
BEHAVIOR_GARRISON_HOVER_TRANSPORT,
BEHAVIOR_BUZZ_DROIDS,
BEHAVIOR_CORRUPT_SYSTEMS,
BEHAVIOR_BOARDABLE,
BEHAVIOR_DYNAMIC_TRANSFORM,

//
// Dummy behaviors are behaviors tied to GameObjectTypes, but not actually physically code-supported. They act more as type categories.
//
BEHAVIOR_DUMMY_GROUND_BASE,
//!< Ground base - A GameObjectType definition, but never actually can be a GameObject
BEHAVIOR_DUMMY_STAR_BASE,
//!< Starbase - A GameObjectType definition, but never actually can be a GameObject
BEHAVIOR_DUMMY_GROUND_COMPANY,
//!< Ground combat company 
BEHAVIOR_DUMMY_SPACE_FIGHTER_SQUADRON,
//!< Space fighter/bomber squadron 
BEHAVIOR_DUMMY_GROUND_STRUCTURE,
//!< Special structure for planet gounrd/surface
BEHAVIOR_DUMMY_ORBITAL_STRUCTURE,
//!< Special structure for planet orbit
BEHAVIOR_DUMMY_STARSHIP,  
//!< Indicator that this is an "independent" ship in space, not a figther squadron or part of a fighter squadron
BEHAVIOR_DUMMY_LAND_BASE_LEVEL_COMPONENT,
//!< This object is a special structure that makes up a land base - 5 per faction
BEHAVIOR_DUMMY_SPECIAL_WEAPON_SOURCE,
//!< This object serves as the location from which a special weapon will fire
BEHAVIOR_DUMMY_TACTICAL_SUPER_WEAPON_TARGET, //!< This object can be targeted and attacked by objects with BEHAVIOR_TACTICAL_SUPER_WEAPON behavior
BEHAVIOR_DUMMY_UPGRADE,
//!< This object is an "upgrade" that enhances some gameplay element for its owner and/or its owner's team.
BEHAVIOR_DUMMY_DESTROY_AFTER_BOMBING_RUN,
//!< This object should be destroyed after its owner completes a bombing run.
BEHAVIOR_DUMMY_DESTROY_AFTER_SPECIAL_WEAPON_FIRED,
//!< This object should be destroyed after its owner fires a special weapon.
BEHAVIOR_DUMMY_TOOLTIP,
//!< This object can be tooltipped without being selectable
BEHAVIOR_DUMMY_DESTROY_AFTER_PLANETARY_BOMBARD, //!< This object should be destroyed after its owner completes a planetary bombard.
BEHAVIOR_COUNT
//!< Maximum number of behavior interfaces

} BehaviorType;
