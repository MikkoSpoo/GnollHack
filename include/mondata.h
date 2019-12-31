/* GnollHack 4.0	mondata.h	$NHDT-Date: 1550524558 2019/02/18 21:15:58 $  $NHDT-Branch: GnollHack-3.6.2-beta01 $:$NHDT-Revision: 1.37 $ */
/* Copyright (c) 1989 Mike Threepoint				  */
/* GnollHack may be freely redistributed.  See license for details. */

#ifndef MONDATA_H
#define MONDATA_H

#define verysmall(ptr) ((ptr)->msize < MZ_SMALL)
#define bigmonst(ptr) ((ptr)->msize >= MZ_LARGE)
#define hugemonst(ptr) ((ptr)->msize >= MZ_HUGE)

#define pm_resistance(ptr, typ) (((ptr)->mresists & (typ)) != 0)

#define resists_fire(mon) \
    ((((mon)->data->mresists | (mon)->mextrinsics) & MR_FIRE) != 0)
#define resists_cold(mon) \
    ((((mon)->data->mresists | (mon)->mextrinsics) & MR_COLD) != 0)
#define resists_sleep(mon) \
    ((((mon)->data->mresists | (mon)->mextrinsics) & MR_SLEEP) != 0)
#define resists_disint(mon) \
    ((((mon)->data->mresists | (mon)->mextrinsics) & MR_DISINT) != 0)
#define resists_elec(mon) \
    ((((mon)->data->mresists | (mon)->mextrinsics) & MR_ELEC) != 0)
#define resists_death(mon) \
    ((((mon)->data->mresists | (mon)->mextrinsics) & MR_DEATH) != 0)
#define resists_lycanthropy(mon) \
    ((((mon)->data->mresists | (mon)->mextrinsics) & MR_LYCANTHROPY) != 0)
#define resists_poison(mon) \
    ((((mon)->data->mresists | (mon)->mextrinsics) & MR_POISON) != 0)
#define resists_acid(mon) \
    ((((mon)->data->mresists | (mon)->mextrinsics) & MR_ACID) != 0)
#define resists_ston(mon) \
    ((((mon)->data->mresists | (mon)->mextrinsics) & MR_STONE) != 0)
#define resists_magicmissile(mon) \
    ((((mon)->data->mresists | (mon)->mextrinsics) & MR_MAGICMISSILE) != 0)
#define resists_magic(mon) \
    ((((mon)->data->mresists | (mon)->mextrinsics) & MR_MAGIC) != 0)
#define resists_charm(mon) \
    ((((mon)->data->mresists | (mon)->mextrinsics) & MR_CHARM) != 0)

#define confers_strength(ptr) \
    (((ptr)->mconveys & MR_STRENGTH) != 0)
#define confers_dexterity(mon) \
    (((ptr)->mconveys & MR_DEXTERITY) != 0)
#define confers_constitution(mon) \
    (((ptr)->mconveys & MR_CONSTITUTION) != 0)
#define confers_intelligence(mon) \
    (((ptr)->mconveys & MR_INTELLIGENCE) != 0)
#define confers_wisdom(mon) \
    (((ptr)->mconveys & MR_WISDOM) != 0)
#define confers_charisma(mon) \
    (((ptr)->mconveys & MR_CHARISMA) != 0)

#define is_lminion(mon) \
    (is_minion((mon)->data) && mon_aligntyp(mon) == A_LAWFUL)
#define is_flyer(ptr) (((ptr)->mflags1 & M1_FLY) != 0L)
#define is_floater(ptr) ((ptr)->mlet == S_EYE || (ptr)->mlet == S_LIGHT)
#define is_clinger(ptr) (((ptr)->mflags1 & M1_CLING) != 0L)
#define is_swimmer(ptr) (((ptr)->mflags1 & M1_SWIM) != 0L)
#define breathless(ptr) (((ptr)->mflags1 & M1_BREATHLESS) != 0L)
#define amphibious(ptr) \
    (((ptr)->mflags1 & (M1_AMPHIBIOUS | M1_BREATHLESS)) != 0L)
#define passes_walls(ptr) (((ptr)->mflags1 & M1_WALLWALK) != 0L)
#define amorphous(ptr) (((ptr)->mflags1 & M1_AMORPHOUS) != 0L)
#define tunnels(ptr) (((ptr)->mflags1 & M1_TUNNEL) != 0L)
#define needspick(ptr) (((ptr)->mflags1 & M1_NEEDPICK) != 0L)
#define hides_under(ptr) (((ptr)->mflags1 & M1_CONCEAL) != 0L)
#define is_hider(ptr) (((ptr)->mflags1 & M1_HIDE) != 0L)
#define haseyes(ptr) (((ptr)->mflags1 & M1_NOEYES) == 0L)
#define eyecount(ptr)                                         \
    (!haseyes(ptr) ? 0 : ((ptr) == &mons[PM_CYCLOPS]          \
                          || (ptr) == &mons[PM_FLOATING_EYE]) \
                             ? 1                              \
                             : 2)
#define nolimbs(ptr) (((ptr)->mflags1 & M1_NOLIMBS) == M1_NOLIMBS) /* two bits */
#define nohands(ptr) (((ptr)->mflags1 & M1_NOHANDS) != 0L) /* also covers no limbs because of the bit in M1_NOLIMBS  */
#define notake(ptr) (((ptr)->mflags1 & M1_NOTAKE) != 0L)
#define has_head(ptr) (((ptr)->mflags1 & M1_NOHEAD) == 0L)
#define has_horns(ptr) (num_horns(ptr) > 0)
#define is_whirly(ptr) \
    ((ptr)->mlet == S_VORTEX || (ptr) == &mons[PM_AIR_ELEMENTAL])
#define flaming(ptr)                                                     \
    ((ptr) == &mons[PM_FIRE_VORTEX] || (ptr) == &mons[PM_FLAMING_SPHERE] \
     || (ptr) == &mons[PM_FIRE_ELEMENTAL] || (ptr) == &mons[PM_SALAMANDER])
#define is_silent(ptr) ((ptr)->msound == MS_SILENT)
#define unsolid(ptr) (((ptr)->mflags1 & M1_UNSOLID) != 0L)
#define mindless(ptr) (((ptr)->mflags1 & M1_MINDLESS) != 0L)
#define humanoid(ptr) (((ptr)->mflags1 & M1_HUMANOID) != 0L)
#define is_animal(ptr) (((ptr)->mflags1 & M1_ANIMAL) != 0L)
#define slithy(ptr) (((ptr)->mflags1 & M1_SLITHY) != 0L)
#define is_wooden(ptr) ((ptr) == &mons[PM_WOOD_GOLEM])
#define thick_skinned(ptr) (((ptr)->mflags1 & M1_THICK_HIDE) != 0L)
#define hug_throttles(ptr) ((ptr) == &mons[PM_ROPE_GOLEM])
#define slimeproof(ptr) \
    ((ptr) == &mons[PM_GREEN_SLIME] || flaming(ptr) || noncorporeal(ptr))
#define lays_eggs(ptr) (((ptr)->mflags1 & M1_OVIPAROUS) != 0L)
#define eggs_in_water(ptr) \
    (lays_eggs(ptr) && (ptr)->mlet == S_EEL && is_swimmer(ptr))
#define regenerates(ptr) (((ptr)->mflags1 & M1_REGEN) != 0L)
#define perceives(ptr) (((ptr)->mflags1 & M1_SEE_INVIS) != 0L)
#define can_teleport(ptr) (((ptr)->mflags1 & M1_TPORT) != 0L)
#define control_teleport(ptr) (((ptr)->mflags1 & M1_TPORT_CNTRL) != 0L)
#define blind_telepathic(ptr)                                                \
     (((ptr)->mflags3 & M3_BLIND_TELEPATHIC) != 0L)
#define unblind_telepathic(ptr)                                                \
     (((ptr)->mflags3 & M3_UNBLIND_TELEPATHIC) != 0L)
#define telepathic(ptr) unblind_telepathic(ptr) 
#define is_armed(ptr) attacktype(ptr, AT_WEAP)
#define acidic(ptr) (((ptr)->mflags1 & M1_ACID) != 0L)
#define poisonous(ptr) (((ptr)->mflags1 & M1_POIS) != 0L)
#define carnivorous(ptr) (((ptr)->mflags1 & M1_CARNIVORE) != 0L)
#define herbivorous(ptr) (((ptr)->mflags1 & M1_HERBIVORE) != 0L)
#define metallivorous(ptr) (((ptr)->mflags1 & M1_METALLIVORE) != 0L)

#define polyok(ptr) (((ptr)->mflags2 & M2_NOPOLY) == 0L)
#define is_shapeshifter(ptr) (((ptr)->mflags2 & M2_SHAPESHIFTER) != 0L)
#define is_undead(ptr) (((ptr)->mflags2 & M2_UNDEAD) != 0L)
#define is_were(ptr) (((ptr)->mflags2 & M2_WERE) != 0L)
#define is_elf(ptr) (((ptr)->mflags2 & M2_ELF) != 0L)
#define is_dwarf(ptr) (((ptr)->mflags2 & M2_DWARF) != 0L)
#define is_gnoll(ptr) (((ptr)->mflags2 & M2_GNOLL) != 0L)
#define is_orc(ptr) (((ptr)->mflags2 & M2_ORC) != 0L)
#define is_gnome(ptr) (((ptr)->mflags2 & M2_GNOME) != 0L)
#define is_human(ptr) (((ptr)->mflags2 & M2_HUMAN) != 0L)
#define your_race(ptr) (((ptr)->mflags2 & urace.selfmask) != 0L)
#define is_bat(ptr)                                         \
    ((ptr) == &mons[PM_BAT] || (ptr) == &mons[PM_GIANT_BAT] \
     || (ptr) == &mons[PM_VAMPIRE_BAT] || (ptr) == &mons[PM_HELL_BAT])
#define is_bird(ptr) ((ptr)->mlet == S_BAT && !is_bat(ptr))
#define is_giant(ptr) (((ptr)->mflags2 & M2_GIANT) != 0L)
#define is_golem(ptr) ((ptr)->mlet == S_GOLEM)
#define is_dragon(ptr) ((ptr)->mlet == S_DRAGON)
#define is_domestic(ptr) (((ptr)->mflags2 & M2_DOMESTIC) != 0L)
#define is_demon(ptr) (((ptr)->mflags2 & M2_DEMON) != 0L)
#define is_angel(ptr) ((ptr)->mlet == S_ANGEL)
#define is_mercenary(ptr) (((ptr)->mflags2 & M2_MERC) != 0L)
#define is_modron(ptr) (((ptr)->mflags2 & M2_MODRON) != 0L)
#define is_male(ptr) (((ptr)->mflags2 & M2_MALE) != 0L)
#define is_female(ptr) (((ptr)->mflags2 & M2_FEMALE) != 0L)
#define is_neuter(ptr) (((ptr)->mflags2 & M2_NEUTER) != 0L)
#define is_wanderer(ptr) (((ptr)->mflags2 & M2_WANDER) != 0L)
#define always_hostile(ptr) (((ptr)->mflags2 & M2_HOSTILE) != 0L)
#define always_peaceful(ptr) (((ptr)->mflags2 & M2_PEACEFUL) != 0L)
#define race_hostile(ptr) (((ptr)->mflags2 & urace.hatemask) != 0L)
#define race_peaceful(ptr) (((ptr)->mflags2 & urace.lovemask) != 0L)
#define extra_nasty(ptr) (((ptr)->mflags2 & M2_NASTY) != 0L)
/*#define strongmonst(ptr) (((ptr)->mflags2 & M2_STRONG) != 0L)*/
#define strongmonst(ptr) (((ptr)->str) >= 17)
#define can_breathe(ptr) attacktype(ptr, AT_BREA)
#define cantwield(ptr) (nohands(ptr) || verysmall(ptr))
#define could_twoweap(ptr) ((ptr)->mattk[1].aatyp == AT_WEAP)
#define cantweararm(ptr) (breakarm(ptr) || sliparm(ptr))
#define throws_rocks(ptr) (((ptr)->mflags2 & M2_ROCKTHROW) != 0L)
#define type_is_pname(ptr) (((ptr)->mflags2 & M2_PNAME) != 0L)
#define is_lord(ptr) (((ptr)->mflags2 & M2_LORD) != 0L)
#define is_prince(ptr) (((ptr)->mflags2 & M2_PRINCE) != 0L)
#define is_ndemon(ptr) \
    (is_demon(ptr) && (((ptr)->mflags2 & (M2_LORD | M2_PRINCE)) == 0L))
#define is_dlord(ptr) (is_demon(ptr) && is_lord(ptr))
#define is_dprince(ptr) (is_demon(ptr) && is_prince(ptr))
#define is_minion(ptr) (((ptr)->mflags2 & M2_MINION) != 0L)
#define likes_gold(ptr) (((ptr)->mflags2 & M2_GREEDY) != 0L)
#define likes_gems(ptr) (((ptr)->mflags2 & M2_JEWELS) != 0L)
#define likes_objs(ptr) (((ptr)->mflags2 & M2_COLLECT) != 0L || is_armed(ptr))
#define likes_magic(ptr) (((ptr)->mflags2 & M2_MAGIC) != 0L)
#define webmaker(ptr) (((ptr)->mflags3 & M3_WEBMAKER) != 0L)
#define is_unicorn(ptr) ((ptr)->mlet == S_UNICORN && likes_gems(ptr))
#define is_longworm(ptr)                                                   \
    (((ptr) == &mons[PM_BABY_LONG_WORM]) || ((ptr) == &mons[PM_LONG_WORM]) \
     || ((ptr) == &mons[PM_LONG_WORM_TAIL]))
#define is_covetous(ptr) (((ptr)->mflags3 & M3_COVETOUS))
#define wants_bell(ptr) (((ptr)->mflags3 & M3_WANTSBELL))
#define infravision(ptr) (((ptr)->mflags3 & M3_INFRAVISION))
#define infravisible(ptr) (((ptr)->mflags3 & M3_INFRAVISIBLE))
#define is_displacer(ptr) (((ptr)->mflags3 & M3_DISPLACES) != 0L)
#define noncorporeal(ptr) (((ptr)->mflags3 & M3_NONCORPOREAL) != 0L)   //((ptr)->mlet == S_GHOST)
#define is_nonliving(ptr) (((ptr)->mflags3 & M3_NONLIVING) != 0L)
#define is_multiweaponmonster(ptr) (((ptr)->mflags3 & M3_MULTIWEAPON) != 0L)
#define leaves_corpses_randomly(ptr) (((ptr)->mflags3 & M3_RANDOM_CORPSE) != 0L)
#define corpse_crumbles_to_dust(ptr) (((ptr)->mflags3 & M3_CORPSE_CRUMBLES_TO_DUST) != 0L)
#define is_speaking_monster(ptr) (((ptr)->mflags3 & M3_SPEAKING) != 0L)
#define is_reflecting(ptr) (((ptr)->mflags3 & M3_REFLECTING) != 0L)
#define is_constrictor(ptr) (((ptr)->mflags3 & M3_CONSTRICTOR) != 0L)
#define hug_requires_two_previous_attacks(ptr) (((ptr)->mflags3 & M3_HUG_HITS_IF_TWO_FIRST_ATTACKS_SUCCEEDED) != 0L)


#define is_brave(ptr) (((ptr)->mflags4 & M4_BRAVE) != 0L)
#define is_fearless(ptr) (((ptr)->mflags4 & M4_FEARLESS) != 0L)
#define has_bloodlust(ptr) (((ptr)->mflags4 & M4_BLOODLUST) != 0L)
#define disregards_own_health(ptr) (is_brave(ptr) || mindless(ptr))
#define disregards_enemy_strength(ptr) (is_fearless(ptr) || mindless(ptr))
#define mon_has_bloodlust(mtmp) (has_bloodlust((mtmp)->data) || (mtmp)->hasbloodlust)
#define mon_disregards_own_health(mtmp) (disregards_own_health((mtmp)->data) || (mtmp)->disregards_own_health)
#define mon_disregards_enemy_strength(mtmp) (disregards_enemy_strength((mtmp)->data) || (mtmp)->disregards_enemy_strength)
#define does_split_upon_hit(ptr) (((ptr)->mflags4 & M4_SPLITS_UPON_HIT) != 0L)
#define is_vegetarian_food(ptr) (((ptr)->mflags4 & M4_VEGETARIAN_FOOD) != 0L)
#define is_vegan_food(ptr) (((ptr)->mflags4 & M4_VEGAN_FOOD) != 0L)

#define is_mplayer(ptr) \
    (((ptr) >= &mons[PM_ARCHEOLOGIST]) && ((ptr) <= &mons[PM_WIZARD]))
#define is_watch(ptr) \
    ((ptr) == &mons[PM_WATCHMAN] || (ptr) == &mons[PM_WATCH_CAPTAIN])
#define is_rider(ptr)                                      \
    ((ptr) == &mons[PM_DEATH] || (ptr) == &mons[PM_FAMINE] \
     || (ptr) == &mons[PM_PESTILENCE])
#define is_placeholder(ptr)                             \
    ((ptr) == &mons[PM_ORC] || (ptr) == &mons[PM_GIANT] \
     || (ptr) == &mons[PM_ELF] || (ptr) == &mons[PM_HUMAN])
/* return TRUE if the monster tends to revive */
#define is_reviver(ptr) (((ptr)->mflags3 & M3_REVIVES_FROM_DEAD) != 0)
//(is_rider(ptr) || (ptr) == &mons[PM_PHOENIX] || (ptr)->mlet == S_TROLL)
/* monsters whose corpses and statues need special handling;
   note that high priests and the Wizard of Yendor are flagged
   as unique even though they really aren't; that's ok here */
#define unique_corpstat(ptr) (((ptr)->geno & G_UNIQ) != 0)

/* this returns the light's range, or 0 if none; if we add more light emitting
   monsters, we'll likely have to add a new light range field to mons[] */
#define emits_light(ptr)                                          \
    (((ptr)->mlet == S_LIGHT || (ptr) == &mons[PM_FLAMING_SPHERE] \
      || (ptr) == &mons[PM_SHOCKING_SPHERE]                       \
      || (ptr) == &mons[PM_FIRE_VORTEX])                          \
         ? 1                                                      \
         : ((ptr) == &mons[PM_FIRE_ELEMENTAL]) ? 1 : 0)
/*	[note: the light ranges above were reduced to 1 for performance...] */
#define likes_lava(ptr) \
    (ptr == &mons[PM_FIRE_ELEMENTAL] || ptr == &mons[PM_SALAMANDER])
#define pm_invisible(ptr) \
    ((ptr) == &mons[PM_STALKER] || (ptr) == &mons[PM_BLACK_LIGHT])

/* could probably add more */
#define likes_fire(ptr)                                                  \
    ((ptr) == &mons[PM_FIRE_VORTEX] || (ptr) == &mons[PM_FLAMING_SPHERE] \
     || likes_lava(ptr))

#define touch_petrifies(ptr) \
    ((ptr) == &mons[PM_COCKATRICE] || (ptr) == &mons[PM_CHICKATRICE])

#define is_mind_flayer(ptr) \
    ((ptr) == &mons[PM_MIND_FLAYER] || (ptr) == &mons[PM_MASTER_MIND_FLAYER] || (ptr) == &mons[PM_ILLITHILICH])

#define is_mimic(ptr) \
    ((ptr->mlet == S_MIMIC)


#define is_vampire(ptr) ((ptr)->mlet == S_VAMPIRE)

#define hates_light(ptr) ((ptr) == &mons[PM_GREMLIN])

/* used to vary a few messages, not affected by death attacks*/
#define is_not_living(ptr) \
    (is_undead(ptr) || is_nonliving(ptr))

#define is_living(ptr) !is_not_living(ptr)

/* no corpse (ie, blank scrolls) if killed by fire */
#define completelyburns(ptr) \
    ((ptr) == &mons[PM_PAPER_GOLEM] || (ptr) == &mons[PM_STRAW_GOLEM])

/* Used for conduct with corpses, tins, and digestion attacks */
/* G_NOCORPSE monsters might still be swallowed as a purple worm */
/* Maybe someday this could be in mflags... */
#define vegan(ptr)                                                 \
    (is_vegan_food(ptr) || noncorporeal(ptr))

#define vegetarian(ptr) \
    (vegan(ptr) || is_vegetarian_food(ptr))

/* monkeys are tameable via bananas but not pacifiable via food,
   otherwise their theft attack could be nullified too easily;
   dogs and cats can be tamed by anything they like to eat and are
   pacified by any other food;
   horses can be tamed by always-veggy food or lichen corpses but
   not tamed or pacified by other corpses or tins of veggy critters */
#define befriend_with_obj(ptr, obj) \
    (((ptr) == &mons[PM_MONKEY] || (ptr) == &mons[PM_APE])               \
     ? (obj)->otyp == BANANA                                             \
     : (is_domestic(ptr) && (obj)->oclass == FOOD_CLASS                  \
        && ((ptr)->mlet != S_UNICORN                                     \
            || objects[(obj)->otyp].oc_material == MAT_VEGGY                 \
            || ((obj)->otyp == CORPSE && (obj)->corpsenm == PM_LICHEN))))

#endif /* MONDATA_H */
