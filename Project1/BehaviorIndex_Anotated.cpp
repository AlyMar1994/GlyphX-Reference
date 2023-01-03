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
BEHAVIOR_LOCO = 1,
//!< Locomotor control (generic)
BEHAVIOR_SELECTABLE = 2,
//!< Selectable behavior
BEHAVIOR_PLANET = 3,
//!< Planetary control
BEHAVIOR_FLEET = 4,
//!< Fleet behavior (not movement)
BEHAVIOR_TRANSPORT = 5,
//!< Transportation behavior
BEHAVIOR_PRODUCTION = 6,
//!< Factory production
BEHAVIOR_IDLE = 7,
//!< Idle behavior, mostly for doing random idle animations over time
BEHAVIOR_PROJECTILE = 8,
//!< A straight-moving weapon emission - like a bullet or a laser burst
BEHAVIOR_TARGETING = 9,
//!< Gives an object the ability to find and track another target object - uses TacticalCombatant data pack
BEHAVIOR_WEAPON = 10,
//!< Gives an object the ability to track weapons for attacking - uses TacticalCombatant data pack
BEHAVIOR_TURRET = 11,
//!< Gives an object the ability to control turret(s) - uses TacticalCombatant data pack
BEHAVIOR_PARTICLE = 12,
//!< A game object that represents an in-world object particle effect
BEHAVIOR_SURFACE_FX = 13,
   //!< Gives an object an ability to have  surface effects (dynamic tracks, particles etc)
BEHAVIOR_WIND_DISTURBANCE = 14,
//!< The object can introduce wind disturbances to the scene
BEHAVIOR_ENERGY_POOL = 15,
//!< The object has an energy pool that recharges itself over time (the energy can be consumed by weapons/shields/engines etc)
BEHAVIOR_SHIELD = 16,
//!< The object has shields which absorb damage and recharge themselves over time
BEHAVIOR_MARKER = 17,                          //!< 
BEHAVIOR_UNIT_AI = 18,
//!< The object interacts with the AI as an individual
BEHAVIOR_COMBATANT = 19,
//!< Combatant behavior (part of autoresolve)
BEHAVIOR_LAND_OBSTACLE = 20,                   //!< A simple pathfinding obstacle in land mode
BEHAVIOR_SQUAD_LEADER = 21,                    //!< A squad leader object
BEHAVIOR_TEAM = 22,
//!< A container behavior for team members
BEHAVIOR_WALL = 23,
//!< 
BEHAVIOR_REVEAL = 24,
//!< Reveals the fog of war around this unit
BEHAVIOR_HIDE_WHEN_FOGGED = 25,
//!< This object doesn't render when it is inside the fog of war
BEHAVIOR_SPAWN_SQUADRON = 26,
//!< This object can spawn reinforcement space fighter squadrons as needed
BEHAVIOR_SPAWN_INDIGENOUS_UNITS = 27,
//!< This object can spawn Indigenous units as needed
BEHAVIOR_DEPLOY_TROOPERS = 28,
//!< AT-AT ability to deploy stormtrooper companies
BEHAVIOR_BARRAGE_AREA = 29,
//!< Keep an area under constant fire...
BEHAVIOR_LANDING = 30,
//!< Deployed units should land correctly
BEHAVIOR_REPLENISH_WINGMEN = 31,
//<! Darth Vader Space
BEHAVIOR_INDIGENOUS_UNIT = 32,
//!< This object is an indigenous unit
BEHAVIOR_HUNT = 33,
//!< Autonomous behavior that sends units out looking for enemies to engage.
BEHAVIOR_GUARD_SPAWN_HOUSE = 34,
//!< Autonomous behavior that makes unit attack any enemy nearby and sends it back to the spawn house if no enemy is found in range
BEHAVIOR_LURE = 35,
BEHAVIOR_EXPLOSION = 36,
//!< This object is an explosion
BEHAVIOR_TREAD_SCROLL = 37,
//!< 
BEHAVIOR_SQUASH = 38, 
  //!< This object has the ability to squash squashable objects to death
BEHAVIOR_AMBIENT_SFX = 39,
//!< This object has the ability to emit ambient SFXEvents
BEHAVIOR_DEBRIS = 40,
//!< This object behaves as a moving piece of debris, with an optional lifetime - used for broken off hard points but can have other applications 
BEHAVIOR_TACTICAL_BUILD_OBJECTS = 41,
//!< This object can build other object in tactical modes
BEHAVIOR_TACTICAL_UNDER_CONSTRUCTION = 42,
//!< This object can be under construction in tactical, by an object with the TACTICAL_BUILD_OBJECTS behavior
BEHAVIOR_TACTICAL_SELL = 43,
//!< This objects was built during tactical and can be sold back for credits
BEHAVIOR_TACTICAL_SUPER_WEAPON = 44,
//!< This object is a special case tactical superweapon that obliterates other objects, not unsing conventional targeting and weapons systems
BEHAVIOR_BURNING = 45,
//!< It burns! It burns! ow!
BEHAVIOR_ASTEROID_FIELD_DAMAGE = 46,
BEHAVIOR_SPECIAL_WEAPON = 47,
//!< This object behaves like a special weapon (ex. Planetary Ion Cannon) during specific tactical modes.
//!< The behavior should only be attached for those modes in which it is valid for this object to behave like the special weapon.
BEHAVIOR_TELEKINESIS_TARGET = 48,
//!< Objects with this behavior can be the target of a telekinesis effect (causes them to levitate off the ground and optionally take damage)
BEHAVIOR_EARTHQUAKE_TARGET = 49,
//!< Objects with this behavior can be the target of an Earthquake Attack effect (causes them to shake and optionally take damage)
BEHAVIOR_VEHICLE_THIEF = 50,
//!< Objects with this behavior can steal enemy vehicles (this controls the object as it walks up to the vehicle and takes it over)
BEHAVIOR_DEMOLITION = 51,
//!< Objects with this behavior (and the Demolition special ability) can plant explosives on buildings and do massive damage
BEHAVIOR_BOMB = 52,
//!< This behavior is used by BEHAVIOR_DEMOLITION to coordinate the application of damage to a target
BEHAVIOR_JETPACK_LOCOMOTOR = 53,
BEHAVIOR_TRANSPORT_LANDING = 54,
//!< This behavior controls the transport drop-off sequence in land mode
BEHAVIOR_LOBBING_SUPERWEAPON = 55,             //!< This behavior is for super weapons with lobbing projectiles. A different kind of directed targeting behavior
BEHAVIOR_STUNNABLE = 56,
//!< This behavior allows the object to be stunned
BEHAVIOR_BASE_SHIELD = 57,
//!< The behavior that implements the logic behind base shields (ie. shields created by the Base Shield Generator structure)
BEHAVIOR_GRAVITY_CONTROL_FIELD = 58,
//!< This behavior allows the object to emit a land-based gravity control field when it is powered
BEHAVIOR_SKY_DOME = 59,
//!< Sky dome behavior, does things like update its sun billboard to match the current lighting.
BEHAVIOR_CASH_POINT = 60,
//!< Cash point behavior gives a cash bonus to the player inside the cash point radius
BEHAVIOR_CAPTURE_POINT = 61,
//!< Generic Capture point behavior.  Changes ownership based on objects in range.
BEHAVIOR_DAMAGE_TRACKING = 62,
//!< Keeps track of damage taken by an object.  For querying by other behaviors and AI.
BEHAVIOR_TERRAIN_TEXTURE_MODIFICATION = 63,    //!< This object can modify the texture of the terrain beneath it >
BEHAVIOR_IMPOSING_PRESENCE = 64,
//!< Presence of this object modifies the idle behavior of others nearby and may cause audio triggers
BEHAVIOR_HARASS = 65,
BEHAVIOR_AVOID_DANGER = 66,
//!< Scout ability - avoid danger(move away from attacking objects)
BEHAVIOR_SELF_DESTRUCT = 67,
//!< Countdowns for certain amount of time and self-destructs
BEHAVIOR_DEATH = 68,
//!< A special "cloned" game object hat goes through some kind of death, created at time of death
//!< This behavior is automatically attached to "clone" at death of original object, and not assigned in XML data
//!< WARNING: Do not attach DEATH behavior to normal game object types
BEHAVIOR_ION_STUN_EFFECT = 69,
//!< Babysits the particles displayed for the ion cannon stun effect (from the planetary ion cannon)
BEHAVIOR_ABILITY_COUNTDOWN = 70,
//!< Allows objects to have per-ability countdown timers so that they activated intermittently
BEHAVIOR_AFFECTED_BY_SHIELD = 71,
//!< Objects with this behavior can be the target of a telekinesis effect (causes them to levitate off the ground and optionally take damage)
BEHAVIOR_NEBULA = 72,
//!< objects with this behavior are affected by nebulas
BEHAVIOR_REINFORCEMENT_POINT = 73,
//!< Enables reinforcementsn within some range
BEHAVIOR_HINT = 74,
//!< Generic Hint behavior that can be set per-object instance
BEHAVIOR_INVULNERABLE = 75,
//!< Makes the object immune to damage, usually attached dynamically during gameplay as the cause of an effect
BEHAVIOR_SPACE_OBSTACLE = 76,
/// Allows the object to occupy one or more layers of the obstacle tracking system
BEHAVIOR_MULTIPLAYER_BEACON = 77,
//!< This is a multiplayer beacon object, used for teammates to communicate visually with each other.
BEHAVIOR_HOLSTER_WEAPON = 78,
// Handles drawing a weapon and putting it away

BEHAVIOR_GARRISON_VEHICLE = 79,
// Allows vehicles to be garrisoned.
BEHAVIOR_GARRISON_STRUCTURE = 80,
// Allows structures to be garrisoned.
BEHAVIOR_GARRISON_UNIT = 81,
// Allows units to garrison vehicles or structures.

BEHAVIOR_DISABLE_FORCE_ABILITIES = 82,
// Disables the force abilites of any unit within range.
BEHAVIOR_CONFUSE = 83,
BEHAVIOR_INFECTION = 84,
BEHAVIOR_PROXIMITY_MINE = 85,
BEHAVIOR_GARRISON_HOVER_TRANSPORT = 86,
BEHAVIOR_BUZZ_DROIDS = 87,
BEHAVIOR_CORRUPT_SYSTEMS = 88,
BEHAVIOR_BOARDABLE = 89,
BEHAVIOR_DYNAMIC_TRANSFORM = 90,

//
// Dummy behaviors are behaviors tied to GameObjectTypes, but not actually physically code-supported. They act more as type categories.
//
BEHAVIOR_DUMMY_GROUND_BASE = 91,
//!< Ground base - A GameObjectType definition, but never actually can be a GameObject
BEHAVIOR_DUMMY_STAR_BASE = 92,
//!< Starbase - A GameObjectType definition, but never actually can be a GameObject
BEHAVIOR_DUMMY_GROUND_COMPANY = 93,
//!< Ground combat company 
BEHAVIOR_DUMMY_SPACE_FIGHTER_SQUADRON = 94,
//!< Space fighter/bomber squadron 
BEHAVIOR_DUMMY_GROUND_STRUCTURE = 95,
//!< Special structure for planet gounrd/surface
BEHAVIOR_DUMMY_ORBITAL_STRUCTURE = 96,
//!< Special structure for planet orbit
BEHAVIOR_DUMMY_STARSHIP = 97,  
//!< Indicator that this is an "independent" ship in space, not a figther squadron or part of a fighter squadron
BEHAVIOR_DUMMY_LAND_BASE_LEVEL_COMPONENT = 98,
//!< This object is a special structure that makes up a land base - 5 per faction
BEHAVIOR_DUMMY_SPECIAL_WEAPON_SOURCE = 99,
//!< This object serves as the location from which a special weapon will fire
BEHAVIOR_DUMMY_TACTICAL_SUPER_WEAPON_TARGET = 100, //!< This object can be targeted and attacked by objects with BEHAVIOR_TACTICAL_SUPER_WEAPON behavior
BEHAVIOR_DUMMY_UPGRADE = 101,
//!< This object is an "upgrade" that enhances some gameplay element for its owner and/or its owner's team.
BEHAVIOR_DUMMY_DESTROY_AFTER_BOMBING_RUN = 102,
//!< This object should be destroyed after its owner completes a bombing run.
BEHAVIOR_DUMMY_DESTROY_AFTER_SPECIAL_WEAPON_FIRED = 103,
//!< This object should be destroyed after its owner fires a special weapon.
BEHAVIOR_DUMMY_TOOLTIP = 104,
//!< This object can be tooltipped without being selectable
BEHAVIOR_DUMMY_DESTROY_AFTER_PLANETARY_BOMBARD = 105, //!< This object should be destroyed after its owner completes a planetary bombard.
BEHAVIOR_COUNT = 106
//!< Maximum number of behavior interfaces

} BehaviorType;
