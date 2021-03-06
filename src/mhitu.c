/* GnollHack 4.0	mhitu.c	$NHDT-Date: 1556649298 2019/04/30 18:34:58 $  $NHDT-Branch: GnollHack-3.6.2-beta01 $:$NHDT-Revision: 1.164 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2012. */
/* GnollHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "artifact.h"

STATIC_VAR NEARDATA struct obj *mon_currwep = (struct obj *) 0;

STATIC_DCL boolean FDECL(u_slip_free, (struct monst *, struct attack *));
STATIC_DCL int FDECL(passiveum, (struct permonst *, struct monst *,
                                 struct attack *));
STATIC_DCL void FDECL(mayberem, (struct monst *, const char *,
                                 struct obj *, const char *));
STATIC_DCL boolean FDECL(diseasemu, (struct permonst *));
STATIC_DCL int FDECL(hitmu, (struct monst *, struct attack *, struct obj*));
STATIC_DCL int FDECL(gulpmu, (struct monst *, struct attack *));
STATIC_DCL int FDECL(explmu, (struct monst *, struct attack *, BOOLEAN_P));
STATIC_DCL void FDECL(missmu, (struct monst *, BOOLEAN_P, struct attack *));
STATIC_DCL void FDECL(mswings, (struct monst *, struct obj *));
STATIC_DCL void FDECL(wildmiss, (struct monst *, struct attack *));
STATIC_DCL void FDECL(hitmsg, (struct monst *, struct attack *, int));

/* See comment in mhitm.c.  If we use this a lot it probably should be */
/* changed to a parameter to mhitu. */
static int dieroll;

STATIC_OVL void
hitmsg(mtmp, mattk, damage)
struct monst *mtmp;
struct attack *mattk;
int damage;
{
    int compat;
    const char *pfmt = 0;
    char *Monst_name = Monnam(mtmp);

    /* Note: if opposite gender, "seductively" */
    /* If same gender, "engagingly" for nymph, normal msg for others */
    if ((compat = could_seduce(mtmp, &youmonst, mattk)) != 0
        && !has_cancelled(mtmp) && !mtmp->mspec_used) {
        pline("%s %s you %s.", Monst_name,
              !Blind ? "smiles at" : !Deaf ? "talks to" : "touches",
              (compat == 2) ? "engagingly" : "seductively");
    } else {
		if (damage < 1)
		{
			switch (mattk->aatyp) {
			case AT_BITE:
					pfmt = "%s bites!";
				break;
			case AT_KICK:
				pline("%s kicks%c", Monst_name,
					  thick_skinned(youmonst.data) ? '.' : '!');
				break;
			case AT_STNG:
				pfmt = "%s stings!";
				break;
			case AT_BUTT:
				pfmt = "%s butts!";
				break;
			case AT_TUCH:
				pfmt = "%s touches you!";
				break;
			case AT_TENT:
				pfmt = "%s tentacles suck you!";
				Monst_name = s_suffix(Monst_name);
				break;
			case AT_EXPL:
			case AT_BOOM:
				pfmt = "%s explodes!";
				break;
			default:
				pfmt = "%s hits!";
			}
			if (pfmt)
				pline(pfmt, Monst_name);
			}
		else
		{
			switch (mattk->aatyp) {
			case AT_BITE:
				pfmt = "%s bites for %d damage!";
				break;
			case AT_KICK:
				pfmt = "%s kicks for %d damage!";
				break;
			case AT_STNG:
				pfmt = "%s stings for %d damage!";
				break;
			case AT_BUTT:
				pfmt = "%s butts for %d damage!";
				break;
			case AT_TUCH:
				pfmt = "%s touches you for %d damage!";
				break;
			case AT_TENT:
				pfmt = "%s tentacles suck you for %d damage!";
				Monst_name = s_suffix(Monst_name);
				break;
			case AT_EXPL:
			case AT_BOOM:
				pfmt = "%s explodes for %d damage!";
				break;
			default:
				pfmt = "%s hits for %d damage!";
			}
			if (pfmt)
				pline(pfmt, Monst_name, damage);

		}
    }
}

/* monster missed you */
STATIC_OVL void
missmu(mtmp, nearmiss, mattk)
struct monst *mtmp;
boolean nearmiss;
struct attack *mattk;
{
    if (!canspotmon(mtmp))
        map_invisible(mtmp->mx, mtmp->my);

    if (could_seduce(mtmp, &youmonst, mattk) && !has_cancelled(mtmp))
        pline("%s pretends to be friendly.", Monnam(mtmp));
    else
        pline("%s %smisses!", Monnam(mtmp),
              (nearmiss && flags.verbose) ? "just " : "");

    stop_occupation();
}

/* monster swings obj */
STATIC_OVL void
mswings(mtmp, otemp)
struct monst *mtmp;
struct obj *otemp;
{
    if (flags.verbose && !Blind && mon_visible(mtmp)) 
	{
        pline("%s %s %s%s %s.", Monnam(mtmp),
              (objects[otemp->otyp].oc_dir & PIERCE) ? "thrusts" : "swings",
              (otemp->quan > 1L) ? "one of " : "", mhis(mtmp), xname(otemp));
    }
}

/* return how a poison attack was delivered */
const char *
mpoisons_subj(mtmp, mattk)
struct monst *mtmp;
struct attack *mattk;
{
    if (mattk->aatyp == AT_WEAP) {
        struct obj *mwep = (mtmp == &youmonst) ? uwep : MON_WEP(mtmp);
        /* "Foo's attack was poisoned." is pretty lame, but at least
           it's better than "sting" when not a stinging attack... */
        return (!mwep || !mwep->opoisoned) ? "attack" : "weapon";
    } else {
        return (mattk->aatyp == AT_TUCH) ? "contact"
                  : (mattk->aatyp == AT_GAZE) ? "gaze"
                       : (mattk->aatyp == AT_BITE) ? "bite" : "sting";
    }
}

/* called when your intrinsic speed is taken away */
void
u_slow_down()
{
    HFast = 0L;
    if (!Fast)
        You("slow down.");
    else /* speed boots */
        Your("quickness feels less natural.");
    exercise(A_DEX, FALSE);
}

/* monster attacked your displaced image */
STATIC_OVL void
wildmiss(mtmp, mattk)
struct monst *mtmp;
struct attack *mattk;
{
    int compat;
    const char *Monst_name; /* Monnam(mtmp) */

    /* no map_invisible() -- no way to tell where _this_ is coming from */

    if (!flags.verbose)
        return;
    if (!cansee(mtmp->mx, mtmp->my))
        return;
    /* maybe it's attacking an image around the corner? */

    compat = ((mattk->adtyp == AD_SEDU || mattk->adtyp == AD_SSEX)
              ? could_seduce(mtmp, &youmonst, mattk) : 0);
    Monst_name = Monnam(mtmp);

    if (is_blinded(mtmp) || (Invis && !has_see_invisible(mtmp))) {
        const char *swings = (mattk->aatyp == AT_BITE) ? "snaps"
                             : (mattk->aatyp == AT_KICK) ? "kicks"
                               : (mattk->aatyp == AT_STNG
                                  || mattk->aatyp == AT_BUTT
                                  || nolimbs(mtmp->data)) ? "lunges"
                                 : "swings";

        if (compat)
            pline("%s tries to touch you and misses!", Monst_name);
        else
            switch (rn2(3)) {
            case 0:
                pline("%s %s wildly and misses!", Monst_name, swings);
                break;
            case 1:
                pline("%s attacks a spot beside you.", Monst_name);
                break;
            case 2:
                pline("%s strikes at %s!", Monst_name,
                      (levl[mtmp->mux][mtmp->muy].typ == WATER)
                        ? "empty water"
                        : "thin air");
                break;
            default:
                pline("%s %s wildly!", Monst_name, swings);
                break;
            }

    } else if (Displaced) {
        /* give 'displaced' message even if hero is Blind */
        if (compat)
            pline("%s smiles %s at your %sdisplaced image...", Monst_name,
                  (compat == 2) ? "engagingly" : "seductively",
                  Invis ? "invisible " : "");
        else
            pline("%s strikes at your %sdisplaced image and misses you!",
                  /* Note:  if you're both invisible and displaced, only
                   * monsters which see invisible will attack your displaced
                   * image, since the displaced image is also invisible. */
                  Monst_name, Invis ? "invisible " : "");

    } else if (Underwater) {
        /* monsters may miss especially on water level where
           bubbles shake the player here and there */
        if (compat)
            pline("%s reaches towards your distorted image.", Monst_name);
        else
            pline("%s is fooled by water reflections and misses!",
                  Monst_name);

    } else
        impossible("%s attacks you without knowing your location?",
                   Monst_name);
}

void
expels(mtmp, mdat, message)
struct monst *mtmp;
struct permonst *mdat; /* if mtmp is polymorphed, mdat != mtmp->data */
boolean message;
{
    if (message) {
        if (is_animal(mdat)) {
            You("get regurgitated!");
        } else {
            char blast[40];
            struct attack *attk = attacktype_fordmg(mdat, AT_ENGL, AD_ANY);

            blast[0] = '\0';
            if (!attk) {
                impossible("Swallower has no engulfing attack?");
            } else {
                if (is_whirly(mdat)) {
                    switch (attk->adtyp) {
                    case AD_ELEC:
                        Strcpy(blast, " in a shower of sparks");
                        break;
                    case AD_COLD:
                        Strcpy(blast, " in a blast of frost");
                        break;
                    }
                } else {
                    Strcpy(blast, " with a squelch");
                }
                You("get expelled from %s%s!", mon_nam(mtmp), blast);
            }
        }
    }
    unstuck(mtmp); /* ball&chain returned in unstuck() */
    mnexto(mtmp);
    newsym(u.ux, u.uy);
    spoteffects(TRUE);
    /* to cover for a case where mtmp is not in a next square */
    if (um_dist(mtmp->mx, mtmp->my, 1))
        pline("Brrooaa...  You land hard at some distance.");
}

/* select a monster's next attack, possibly substituting for its usual one */
struct attack *
getmattk(magr, mdef, indx, prev_result, alt_attk_buf)
struct monst *magr, *mdef;
int indx, prev_result[];
struct attack *alt_attk_buf;
{
    struct permonst *mptr = magr->data;
    struct attack *attk = &mptr->mattk[indx];
    struct obj *weap = (magr == &youmonst) ? uwep : MON_WEP(magr);

    /* honor SEDUCE=0 */
    if (!SYSOPT_SEDUCE) {
        extern const struct attack sa_no[NATTK];

        /* if the first attack is for SSEX damage, all six attacks will be
           substituted (expected succubus/incubus handling); if it isn't
           but another one is, only that other one will be substituted */
        if (mptr->mattk[0].adtyp == AD_SSEX) {
            *alt_attk_buf = sa_no[indx];
            attk = alt_attk_buf;
        } else if (attk->adtyp == AD_SSEX) {
            *alt_attk_buf = *attk;
            attk = alt_attk_buf;
            attk->adtyp = AD_DRLI;
        }
    }

    /* prevent a monster with two consecutive disease or hunger attacks
       from hitting with both of them on the same turn; if the first has
       already hit, switch to a stun attack for the second */
    if (indx > 0 && prev_result[indx - 1] > 0
        && (attk->adtyp == AD_DISE || attk->adtyp == AD_PEST
            || attk->adtyp == AD_FAMN)
        && attk->adtyp == mptr->mattk[indx - 1].adtyp) {
        *alt_attk_buf = *attk;
        attk = alt_attk_buf;
        attk->adtyp = AD_STUN;

    /* make drain-energy damage be somewhat in proportion to energy */
    } else if (attk->adtyp == AD_DREN && mdef == &youmonst) {
        int ulev = max(u.ulevel, 6);

        *alt_attk_buf = *attk;
        attk = alt_attk_buf;
        /* 3.6.0 used 4d6 but since energy drain came out of max energy
           once current energy was gone, that tended to have a severe
           effect on low energy characters; it's now 2d6 with ajustments */
        if (u.uen <= 5 * ulev && attk->damn > 1) {
            attk->damn -= 1; /* low energy: 2d6 -> 1d6 */
            if (u.uenmax <= 2 * ulev && attk->damd > 3)
                attk->damd -= 3; /* very low energy: 1d6 -> 1d3 */
        } else if (u.uen > 12 * ulev) {
            attk->damn += 1; /* high energy: 2d6 -> 3d6 */
            if (u.uenmax > 20 * ulev)
                attk->damd += 3; /* very high energy: 3d6 -> 3d9 */
            /* note: 3d9 is slightly higher than previous 4d6 */
        }

    } else if (attk->aatyp == AT_ENGL && magr->mspec_used) {
        /* can't re-engulf yet; switch to simpler attack */
        *alt_attk_buf = *attk;
        attk = alt_attk_buf;
        if (attk->adtyp == AD_ACID || attk->adtyp == AD_ELEC
            || attk->adtyp == AD_COLD || attk->adtyp == AD_FIRE) {
            attk->aatyp = AT_TUCH;
        } else {
            attk->aatyp = AT_CLAW; /* attack message will be "<foo> hits" */
            attk->adtyp = AD_PHYS;
        }
        attk->damn = 1; /* relatively weak: 1d6 */
        attk->damd = 6;

    /* barrow wight, Nazgul, erinys have weapon attack for non-physical
       damage; force physical damage if attacker has been cancelled or
       if weapon is sufficiently interesting; a few unique creatures
       have two weapon attacks where one does physical damage and other
       doesn't--avoid forcing physical damage for those */
    } else if (indx == 0 && magr != &youmonst
               && attk->aatyp == AT_WEAP && attk->adtyp != AD_PHYS
               && !(mptr->mattk[1].aatyp == AT_WEAP
                    && mptr->mattk[1].adtyp == AD_PHYS)
               && (has_cancelled(magr)
                   || (weap && ((weap->otyp == CORPSE
                                 && touch_petrifies(&mons[weap->corpsenm]))
                                || weap->oartifact == ART_STORMBRINGER
                                || weap->oartifact == ART_VORPAL_BLADE)))) {
        *alt_attk_buf = *attk;
        attk = alt_attk_buf;
        attk->adtyp = AD_PHYS;
    }
    return attk;
}

/*
 * mattacku: monster attacks you
 *      returns 1 if monster dies (e.g. "yellow light"), 0 otherwise
 *      Note: if you're displaced or invisible the monster might attack the
 *              wrong position...
 *      Assumption: it's attacking you or an empty square; if there's another
 *              monster which it attacks by mistake, the caller had better
 *              take care of it...
 */
int
mattacku(mtmp)
register struct monst *mtmp;
{
    struct attack *mattk, alt_attk;
    int i, j = 0, tmp, sum[NATTK];
    struct permonst *mdat = mtmp->data;
    boolean ranged = (distu(mtmp->mx, mtmp->my) > 3);
    /* Is it near you?  Affects your actions */
    boolean range2 = !monnear(mtmp, mtmp->mux, mtmp->muy);
    /* Does it think it's near you?  Affects its actions */
    boolean foundyou = (mtmp->mux == u.ux && mtmp->muy == u.uy);
    /* Is it attacking you or your image? */
    boolean youseeit = canseemon(mtmp);
    /* Might be attacking your image around the corner, or
     * invisible, or you might be blind....
     */
    boolean skipnonmagc = FALSE;
    /* Are further physical attack attempts useless? */

    if (!ranged)
        nomul(0);
    if (DEADMONSTER(mtmp) || (Underwater && !is_swimmer(mtmp->data)))
        return 0;

    /* If swallowed, can only be affected by u.ustuck */
    if (u.uswallow) {
        if (mtmp != u.ustuck)
            return 0;
        u.ustuck->mux = u.ux;
        u.ustuck->muy = u.uy;
        range2 = 0;
        foundyou = 1;
        if (u.uinvulnerable)
            return 0; /* stomachs can't hurt you! */

    } else if (u.usteed) {
        if (mtmp == u.usteed)
            /* Your steed won't attack you */
            return 0;
        /* Orcs like to steal and eat horses and the like */
        if (!rn2(is_orc(mtmp->data) ? 2 : 4)
            && distu(mtmp->mx, mtmp->my) <= 2) {
            /* Attack your steed instead */
            i = mattackm(mtmp, u.usteed);
            if ((i & MM_AGR_DIED))
                return 1;
            /* make sure steed is still alive and within range */
            if ((i & MM_DEF_DIED) || !u.usteed
                || distu(mtmp->mx, mtmp->my) > 2)
                return 0;
            /* Let your steed retaliate */
            return !!(mattackm(u.usteed, mtmp) & MM_DEF_DIED);
        }
    }

    if (u.uundetected && !range2 && foundyou && !u.uswallow) {
        if (!canspotmon(mtmp))
            map_invisible(mtmp->mx, mtmp->my);
        u.uundetected = 0;
        if (is_hider(youmonst.data) && u.umonnum != PM_TRAPPER) {
            /* ceiling hider */
            coord cc; /* maybe we need a unexto() function? */
            struct obj *obj;

            You("fall from the %s!", ceiling(u.ux, u.uy));
            /* take monster off map now so that its location
               is eligible for placing hero; we assume that a
               removed monster remembers its old spot <mx,my> */
            remove_monster(mtmp->mx, mtmp->my);
            if (!enexto(&cc, u.ux, u.uy, youmonst.data)
                /* a fish won't voluntarily swap positions
                   when it's in water and hero is over land */
                || (mtmp->data->mlet == S_EEL
                    && is_pool(mtmp->mx, mtmp->my)
                    && !is_pool(u.ux, u.uy))) {
                /* couldn't find any spot for hero; this used to
                   kill off attacker, but now we just give a "miss"
                   message and keep both mtmp and hero at their
                   original positions; hero has become unconcealed
                   so mtmp's next move will be a regular attack */
                place_monster(mtmp, mtmp->mx, mtmp->my); /* put back */
                newsym(u.ux, u.uy); /* u.uundetected was toggled */
                pline("%s draws back as you drop!", Monnam(mtmp));
                return 0;
            }

            /* put mtmp at hero's spot and move hero to <cc.x,.y> */
            newsym(mtmp->mx, mtmp->my); /* finish removal */
            place_monster(mtmp, u.ux, u.uy);
            if (mtmp->wormno) {
                worm_move(mtmp);
                /* tail hasn't grown, so if it now occupies <cc.x,.y>
                   then one of its original spots must be free */
                if (m_at(cc.x, cc.y))
                    (void) enexto(&cc, u.ux, u.uy, youmonst.data);
            }
            teleds(cc.x, cc.y, TRUE); /* move hero */
            set_apparxy(mtmp);
            newsym(u.ux, u.uy);

            if (youmonst.data->mlet != S_PIERCER)
                return 0; /* lurkers don't attack */

            obj = which_armor(mtmp, WORN_HELMET);
            if (obj && is_metallic(obj)) {
                Your("blow glances off %s %s.", s_suffix(mon_nam(mtmp)),
                     helm_simple_name(obj));
            } else {
                if (3 + find_mac(mtmp) <= rnd(20)) {
                    pline("%s is hit by a falling piercer (you)!",
                          Monnam(mtmp));
                    if ((mtmp->mhp -= d(3, 6)) < 1)
                        killed(mtmp);
                } else
                    pline("%s is almost hit by a falling piercer (you)!",
                          Monnam(mtmp));
            }

        } else {
            /* surface hider */
            if (!youseeit) {
                pline("It tries to move where you are hiding.");
            } else {
                /* Ugly kludge for eggs.  The message is phrased so as
                 * to be directed at the monster, not the player,
                 * which makes "laid by you" wrong.  For the
                 * parallelism to work, we can't rephrase it, so we
                 * zap the "laid by you" momentarily instead.
                 */
                struct obj *obj = level.objects[u.ux][u.uy];

                if (obj || u.umonnum == PM_TRAPPER
                    || (youmonst.data->mlet == S_EEL
                        && is_pool(u.ux, u.uy))) {
                    int save_spe = 0; /* suppress warning */

                    if (obj) {
                        save_spe = obj->spe;
                        if (obj->otyp == EGG)
                            obj->spe = 0;
                    }
                    /* note that m_monnam() overrides hallucination, which is
                       what we want when message is from mtmp's perspective */
                    if (youmonst.data->mlet == S_EEL
                        || u.umonnum == PM_TRAPPER)
                        pline(
                             "Wait, %s!  There's a hidden %s named %s there!",
                              m_monnam(mtmp), youmonst.data->mname, plname);
                    else
                        pline(
                          "Wait, %s!  There's a %s named %s hiding under %s!",
                              m_monnam(mtmp), youmonst.data->mname, plname,
                              doname(level.objects[u.ux][u.uy]));
                    if (obj)
                        obj->spe = save_spe;
                } else
                    impossible("hiding under nothing?");
            }
            newsym(u.ux, u.uy);
        }
        return 0;
    }

    /* hero might be a mimic, concealed via #monster */
    if (youmonst.data->mlet == S_MIMIC && U_AP_TYPE && !range2
        && foundyou && !u.uswallow) {
        boolean sticky = sticks(youmonst.data);

        if (!canspotmon(mtmp))
            map_invisible(mtmp->mx, mtmp->my);
        if (sticky && !youseeit)
            pline("It gets stuck on you.");
        else /* see note about m_monnam() above */
            pline("Wait, %s!  That's a %s named %s!", m_monnam(mtmp),
                  youmonst.data->mname, plname);
        if (sticky)
            u.ustuck = mtmp;
        youmonst.m_ap_type = M_AP_NOTHING;
        youmonst.mappearance = 0;
        newsym(u.ux, u.uy);
        return 0;
    }

    /* non-mimic hero might be mimicking an object after eating m corpse */
    if (U_AP_TYPE == M_AP_OBJECT && !range2 && foundyou && !u.uswallow) {
        if (!canspotmon(mtmp))
            map_invisible(mtmp->mx, mtmp->my);
        if (!youseeit)
            pline("%s %s!", Something, (likes_gold(mtmp->data)
                                        && youmonst.mappearance == GOLD_PIECE)
                                           ? "tries to pick you up"
                                           : "disturbs you");
        else /* see note about m_monnam() above */
            pline("Wait, %s!  That %s is really %s named %s!", m_monnam(mtmp),
                  mimic_obj_name(&youmonst), an(mons[u.umonnum].mname),
                  plname);
        if (multi < 0) { /* this should always be the case */
            char buf[BUFSZ];

            Sprintf(buf, "You appear to be %s again.",
                    Upolyd ? (const char *) an(youmonst.data->mname)
                           : (const char *) "yourself");
            unmul(buf); /* immediately stop mimicking */
        }
        return 0;
    }

    /*  Work out the armor class differential   */
    tmp = AC_VALUE(u.uac) + 10; /* tmp ~= 0 - 20 */
    tmp += mtmp->m_lev;	//Add level to hit chance
	tmp += m_strdex_to_hit_bonus(mtmp); //Add monster's STR and DEX bonus, thrown weapons are dealt separately
	tmp += mtmp->mhitinc;
	if (multi < 0)
        tmp += 4;
    if ((Invis && !has_see_invisible(mtmp)) || is_blinded(mtmp))
        tmp -= 2;
    if (mtmp->mtrapped)
        tmp -= 2;
    if (tmp <= 0)
        tmp = 1;

    /* make eels visible the moment they hit/miss us */
    if (mdat->mlet == S_EEL && mtmp->mundetected && cansee(mtmp->mx, mtmp->my))
	{
		mtmp->mundetected = 0;
        newsym(mtmp->mx, mtmp->my);
    }


    if (u.uinvulnerable) {
        /* monsters won't attack you */
        if (mtmp == u.ustuck) {
            pline("%s loosens its grip slightly.", Monnam(mtmp));
        } else if (!range2) {
            if (youseeit || sensemon(mtmp))
                pline("%s starts to attack you, but pulls back.",
                      Monnam(mtmp));
            else
                You_feel("%s move nearby.", something);
        }
        return 0;
    }

    /* Unlike defensive stuff, don't let them use item _and_ attack. */
    if (find_offensive(mtmp)) {
        int foo = use_offensive(mtmp);

        if (foo != 0)
            return (foo == 1);
    }

	int tmp2 = tmp;
	int weaponattackcount = 0;

    for (i = 0; i < NATTK; i++) 
	{
		tmp = tmp2; // Revert hit bonus to original value
        sum[i] = 0;
        mon_currwep = (struct obj *)0;
        mattk = getmattk(mtmp, &youmonst, i, sum, &alt_attk);
        if ((u.uswallow && mattk->aatyp != AT_ENGL)
            || (skipnonmagc && mattk->aatyp != AT_MAGC && mattk->aatyp != AT_SMMN))
            continue;

        switch (mattk->aatyp) {
        case AT_CLAW: /* "hand to hand" attacks */
        case AT_KICK:
        case AT_BITE:
        case AT_STNG:
        case AT_TUCH:
        case AT_BUTT:
        case AT_TENT:
            if (!range2 && (!MON_WEP(mtmp) || is_confused(mtmp) || Conflict || !touch_petrifies(youmonst.data))) 
			{
                if (foundyou) 
				{
                    if (tmp > (j = rnd(20 + i))) 
					{
                        if (mattk->aatyp != AT_KICK || !thick_skinned(youmonst.data))
                            sum[i] = hitmu(mtmp, mattk, (struct obj*) 0);
                    } else
                        missmu(mtmp, (tmp == j), mattk);
                } 
				else
				{
                    wildmiss(mtmp, mattk);
                    /* skip any remaining non-spell attacks */
                    skipnonmagc = TRUE;
                }
            }
            break;

        case AT_HUGS: /* automatic if prev two attacks succeed */
            /* Note: if displaced, prev attacks never succeeded */
            if ((!range2 && ((!hug_requires_two_previous_attacks(mtmp->data) && tmp > (j = rnd(20 + i))) || (hug_requires_two_previous_attacks(mtmp->data) && i >= 2 && sum[i - 1] && sum[i - 2])))
                || mtmp == u.ustuck)
                sum[i] = hitmu(mtmp, mattk, (struct obj*) 0);
            break;

        case AT_GAZE: /* can affect you either ranged or not */
            /* Medusa gaze already operated through m_respond in
               dochug(); don't gaze more than once per round. */
            if (mdat != &mons[PM_MEDUSA])
                sum[i] = gazemu(mtmp, mattk);
            break;

        case AT_EXPL: /* automatic hit if next to, and aimed at you */
            if (!range2)
                sum[i] = explmu(mtmp, mattk, foundyou);
            break;

        case AT_ENGL:
            if (!range2) {
                if (foundyou) {
                    if (u.uswallow
                        || (!mtmp->mspec_used && tmp > (j = rnd(20 + i)))) {
                        /* force swallowing monster to be displayed
                           even when hero is moving away */
                        flush_screen(1);
                        sum[i] = gulpmu(mtmp, mattk);
                    } else {
                        missmu(mtmp, (tmp == j), mattk);
                    }
                } else if (is_animal(mtmp->data)) {
                    pline("%s gulps some air!", Monnam(mtmp));
                } else {
                    if (youseeit)
                        pline("%s lunges forward and recoils!", Monnam(mtmp));
                    else
                        You_hear("a %s nearby.",
                                 is_whirly(mtmp->data) ? "rushing noise"
                                                       : "splat");
                }
            }
            break;
        case AT_BREA:
            if (range2)
                sum[i] = breamu(mtmp, mattk);
            /* Note: breamu takes care of displacement */
            break;
        case AT_SPIT:
            if (range2)
                sum[i] = spitmu(mtmp, mattk);
            /* Note: spitmu takes care of displacement */
            break;
        case AT_WEAP:
            if (range2) 
			{
                if (!Is_rogue_level(&u.uz))
                    thrwmu(mtmp);
            }
			else
			{
                int hittmp = 0;

                /* Rare but not impossible.  Normally the monster
                 * wields when 2 spaces away, but it can be
                 * teleported or whatever....
                 */
				boolean is_nonwelded_launcher = (MON_WEP(mtmp) && (is_launcher(MON_WEP(mtmp)) && !mwelded(MON_WEP(mtmp), mtmp)));

                if (mtmp->weapon_strategy == NEED_WEAPON || !MON_WEP(mtmp) || is_nonwelded_launcher)
				{
					if (is_nonwelded_launcher && !select_hwep(mtmp))
					{
						pline("%s unwields %s.", Monnam(mtmp), the(cxname(MON_WEP(mtmp))));
						setmnotwielded(mtmp, MON_WEP(mtmp));
					}
                    mtmp->weapon_strategy = NEED_HTH_WEAPON;
                    /* mon_wield_item resets weapon_strategy as appropriate */
                    if (mon_wield_item(mtmp, FALSE) != 0)
                        break;
                }
                if (foundyou)
				{
					weaponattackcount++;
					if(is_multiweaponmonster(mtmp->data))
						mon_currwep = select_multiweapon_nth_hwep(mtmp, weaponattackcount);
					else
	                    mon_currwep = MON_WEP(mtmp);

					int multistrike = 1;
					int multistrikernd = 0;

					if (mon_currwep) 
					{
                        hittmp = weapon_to_hit_value(mon_currwep, &youmonst, mtmp);
                        tmp += hittmp;

						get_multishot_stats(mtmp, mon_currwep, mon_currwep, FALSE, &multistrike, &multistrikernd);

                    }

					if (multistrikernd > 0)
						multistrike += rn2(multistrikernd + 1);

					for (int strikeindex = 0; strikeindex < multistrike; strikeindex++)
					{
						if (mon_currwep)
						{
							if (strikeindex == 0)
								mswings(mtmp, mon_currwep);
							else
								if (flags.verbose && !Blind && mon_visible(mtmp))
								{
									/* To be consistent with mswings */
									pline("%s %s %s!", s_suffix(Monnam(mtmp)), aobjnam(mon_currwep, "strike"), strikeindex == 1 ? "a second time" : strikeindex == 2 ? "a third time" : "once more");
								}
						}
							
						//TO-HIT IS DONE HERE
						if (tmp > (j = dieroll = rnd(20 + i)))
							sum[i] = hitmu(mtmp, mattk, mon_currwep);
						else
							missmu(mtmp, (tmp == j), mattk);
					}
                } 
				else
				{
                    wildmiss(mtmp, mattk);
                    /* skip any remaining non-spell attacks */
                    skipnonmagc = TRUE;
                }
            }
            break;
        case AT_MAGC:
			if(!has_cancelled(mtmp))
			{
				if (range2)
					sum[i] = buzzmu(mtmp, mattk);
				else
					sum[i] = castmu(mtmp, mattk, TRUE, foundyou);
			}
            break;

		case AT_SMMN:
			if (!has_summon_forbidden(mtmp))
			{
				if (mattk->adtyp == AD_DMNS)
				{
					/*  Special demon handling code */
					if ((mtmp->cham == NON_PM) && !range2) { //Chameleons do not summon, others only in close range
						int chance = mattk->mcadj;
						if (!has_cancelled(mtmp) && rn2(100) < chance && !item_prevents_summoning(mtmp->mnum))
						{
							pline("%s gates in some help.", Monnam(mtmp));
							(void)msummon(mtmp);
							sum[i] = 1;
						}
						else
						{
							if(mtmp->data == &mons[PM_DEMOGORGON])
								pline("%s hisses angrily.", Monnam(mtmp));
							else if(is_prince(mtmp->data) || is_lord(mtmp->data))
								pline("%s concentrates for a moment.", Monnam(mtmp));
							else
								pline("%s attempts to gate in some help, but nothing happens.", Monnam(mtmp));
						}
					}
				}
				else if (mattk->adtyp == AD_LYCA)
				{
					/*  Special lycanthrope handling code */
					if ((mtmp->cham == NON_PM) && is_were(mdat) && !range2) {
						if (is_human(mdat)) {
							if (!rn2(5 - (night() * 2)) && !has_cancelled(mtmp))
								new_were(mtmp);
						}
						else if (!rn2(30) && !has_cancelled(mtmp))
							new_were(mtmp);
						mdat = mtmp->data;

						if (!rn2(10) && !has_cancelled(mtmp) && !item_prevents_summoning(mtmp->mnum)) {
							int numseen, numhelp;
							char buf[BUFSZ], genericwere[BUFSZ];

							Strcpy(genericwere, "creature");
							numhelp = were_summon(mdat, FALSE, &numseen, genericwere);
							if (youseeit) {
								pline("%s summons help!", Monnam(mtmp));
								if (numhelp > 0) {
									if (numseen == 0)
										You_feel("hemmed in.");
								}
								else
									pline("But none comes.");
							}
							else {
								const char* from_nowhere;

								if (!Deaf) {
									pline("%s %s!", Something, makeplural(growl_sound(mtmp)));
									from_nowhere = "";
								}
								else
									from_nowhere = " from nowhere";
								if (numhelp > 0) {
									if (numseen < 1)
										You_feel("hemmed in.");
									else {
										if (numseen == 1)
											Sprintf(buf, "%s appears", an(genericwere));
										else
											Sprintf(buf, "%s appear",
												makeplural(genericwere));
										pline("%s%s!", upstart(buf), from_nowhere);
									}
								} /* else no help came; but you didn't know it tried */
							}
						}
					}
				}
				else if (mattk->adtyp == AD_GNOL)
				{
					/*  Special gnoll handling code */
					if ((mtmp->cham == NON_PM) && !range2) { //Chameleons do not summon, others only in close range
						int chance = mattk->mcadj;
						if (!has_cancelled(mtmp) && rn2(100) < chance && !item_prevents_summoning(mtmp->mnum))
						{
							pline("%s summons some gnolls!", Monnam(mtmp));
							(void)yeenoghu_gnoll_summon();
							sum[i] = 1;
						}
						else
						{
							if (!rn2(2))
								pline("%s swings his flail commandingly.", Monnam(mtmp));
							else
								pline("%s growls menacingly.", Monnam(mtmp));
						}
					}
				}
				else if (mattk->adtyp == AD_GHUL)
				{
					/*  Special gnoll handling code */
					if ((mtmp->cham == NON_PM) && !range2) { //Chameleons do not summon, others only in close range
						int chance = mattk->mcadj;
						if (!has_cancelled(mtmp) && rn2(100) < chance && !item_prevents_summoning(mtmp->mnum))
						{
							pline("%s summons some ghouls!", Monnam(mtmp));
							(void)yeenoghu_ghoul_summon();
							sum[i] = 1;
						}
						else
						{
							if (!rn2(2))
								pline("%s lets loose a blood-curdling howl!", Monnam(mtmp));
							else
								pline("%s howls!", Monnam(mtmp));
						}
					}
				}
				else if (mattk->adtyp == AD_UNDO)
				{
					/*  Special gnoll handling code */
					if ((mtmp->cham == NON_PM) && !range2) { //Chameleons do not summon, others only in close range
						int chance = mattk->mcadj;
						if (!has_cancelled(mtmp) && rn2(100) < chance && !item_prevents_summoning(mtmp->mnum))
						{
							pline("%s summons some undead!", Monnam(mtmp));
							(void)orcus_undead_summon();
							sum[i] = 1;
						}
						else
						{
							if (!rn2(2) || !canseemon(mtmp) || !m_carrying(mtmp, WAN_DEATH))
								pline("%s laughs at you!", Monnam(mtmp));
							else
								pline("%s swings his wand menacingly.", Monnam(mtmp));
						}
					}
				}
			}

			break;

		default: /* no attack */
            break;
        }
        if (context.botl)
            bot();
        /* give player a chance of waking up before dying -kaa */
        if (sum[i] == 1) { /* successful attack */
            if (u.usleep && u.usleep < monstermoves && !rn2(10)) {
                multi = -1;
                nomovemsg = "The combat suddenly awakens you.";
            }
        }
        if (sum[i] == 2)
            return 1; /* attacker dead */
        if (sum[i] == 3)
            break; /* attacker teleported, no more attacks */
        /* sum[i] == 0: unsuccessful attack */
    }
    return 0;
}

STATIC_OVL boolean
diseasemu(mdat)
struct permonst *mdat;
{
    if (Sick_resistance) {
        You_feel("a slight illness.");
        return FALSE;
    } else
	{
        make_sick(Sick ? Sick / 3L + 1L : (long) rn1(ACURR(A_CON), 20),
                  mdat->mname, TRUE, SICK_NONVOMITABLE);
        return TRUE;
    }
}

/* check whether slippery clothing protects from hug or wrap attack */
STATIC_OVL boolean
u_slip_free(mtmp, mattk)
struct monst *mtmp;
struct attack *mattk;
{
    struct obj *obj = (uarmc ? uarmc : (uarmo ? uarmo : uarm));

    if (!obj)
        obj = uarmu;
    if (mattk->adtyp == AD_DRIN)
        obj = uarmh;

    /* if your cloak/armor is greased, monster slips off; this
       protection might fail (33% chance) when the armor is cursed */
    if (obj && (obj->greased || obj->otyp == OILSKIN_CLOAK)
        && (!obj->cursed || rn2(3))) {
        pline("%s %s your %s %s!", Monnam(mtmp),
              (mattk->adtyp == AD_WRAP) ? "slips off of"
                                        : "grabs you, but cannot hold onto",
              obj->greased ? "greased" : "slippery",
              /* avoid "slippery slippery cloak"
                 for undiscovered oilskin cloak */
              (obj->greased || objects[obj->otyp].oc_name_known)
                  ? xname(obj)
                  : cloak_simple_name(obj));

        if (obj->greased && !rn2(2)) {
            pline_The("grease wears off.");
            obj->greased = 0;
            update_inventory();
        }
        return TRUE;
    }
    return FALSE;
}

boolean
check_ability_resistance_success(mtmp, ability, adjustment_to_roll)
struct monst* mtmp;
int ability;
int adjustment_to_roll;
{
	if (!mtmp)
		return FALSE;

	int ability_score = 0;
	int str = 0;
	int dex = 0;
	int con = 0;
	int intl = 0;
	int wis = 0;
	int cha = 0;

	str = m_acurrstr(mtmp);
	dex = m_acurr(mtmp, A_DEX);
	con = m_acurr(mtmp, A_CON);
	intl = m_acurr(mtmp, A_INT);
	wis = m_acurr(mtmp, A_WIS);
	cha = m_acurr(mtmp, A_CHA);

	switch (ability)
	{
	case A_STR:
		ability_score = str;
		break;
	case A_DEX:
		ability_score = dex;
		break;
	case A_CON:
		ability_score = con;
		break;
	case A_INT:
		ability_score = intl;
		break;
	case A_WIS:
		ability_score = wis;
		break;
	case A_CHA:
		ability_score = cha;
		break;
	case A_MAX_INT_WIS:
		ability_score = max(intl, wis);
		break;
	case A_MAX_INT_CHA:
		ability_score = max(intl, cha);
		break;
	case A_MAX_WIS_CHA:
		ability_score = max(wis, cha);
		break;
	case A_MAX_INT_WIS_CHA:
		ability_score = max(max(intl, wis), cha);
		break;
	case A_AVG_INT_WIS:
		ability_score = (intl + wis) / 2;
		break;
	case A_AVG_INT_CHA:
		ability_score = (intl + wis) / 2;
		break;
	case A_AVG_WIS_CHA:
		ability_score = (wis + cha) / 2;
		break;
	case A_AVG_INT_WIS_CHA:
		ability_score = (intl + wis + cha) / 3;
		break;
	default:
		break;
	}

	int adjscore = ability_score + adjustment_to_roll;
	int percentage = 0;

	if (adjscore < 1)
		percentage = 0;
	else if (adjscore > 25)
		percentage = 100;
	else
	{
		switch (adjscore)
		{
		case 1:
			percentage = 1;
			break;
		case 2:
			percentage = 2;
			break;
		case 3:
			percentage = 3;
			break;
		case 4:
			percentage = 4;
			break;
		case 5:
			percentage = 5;
			break;
		case 6:
			percentage = 7;
			break;
		case 7:
			percentage = 10;
			break;
		case 8:
			percentage = 15;
			break;
		case 9:
			percentage = 20;
			break;
		case 10:
			percentage = 30;
			break;
		case 11:
			percentage = 40;
			break;
		case 12:
			percentage = 50;
			break;
		case 13:
			percentage = 55;
			break;
		case 14:
			percentage = 60;
			break;
		case 15:
			percentage = 65;
			break;
		case 16:
			percentage = 70;
			break;
		case 17:
			percentage = 75;
			break;
		case 18:
			percentage = 80;
			break;
		case 19:
			percentage = 84;
			break;
		case 20:
			percentage = 87;
			break;
		case 21:
			percentage = 90;
			break;
		case 22:
			percentage = 93;
			break;
		case 23:
			percentage = 95;
			break;
		case 24:
			percentage = 97;
			break;
		case 25:
			percentage = 99;
			break;
		default:
			break;
		}
	}
	boolean success = (rn2(100) < percentage);
	return success;
}


boolean
check_magic_cancellation_success(mtmp, adjustment_to_roll)
struct monst* mtmp;
int adjustment_to_roll;
{
	boolean success = (rn2(100) < magic_negation_percentage(((mtmp == &youmonst) ? u.umc: magic_negation(mtmp)) + adjustment_to_roll));
	return success;
}

/* armor that sufficiently covers the body might be able to block magic */
int
magic_negation(mon)
struct monst *mon;
{
	if (!mon)
		return 0;

    struct obj *o;
    long wearmask;
    int mc = mons[mon->mnum].mc; /* base magic cancellation for current form */
	boolean is_you = (mon == &youmonst);

	int item_mc_bonus = 0;

	int suit_mc_bonus = 0;
	int robe_mc_bonus = 0;
	int combined_mc_bonus = 0;

	for (o = is_you ? invent : mon->minvent; o; o = o->nobj) {
        /* oc_magic_cancellation field is only applicable for armor (which must be worn), this should exclude spellbooks and wands, which use oc_oc2 for something else */
		/* omit W_SWAPWEP+W_QUIVER; W_ARTIFACT_CARRIED+W_ARTIFACT_INVOKED handled by protects() */
		wearmask = (W_ARMOR & ~W_ARMS) | W_ACCESSORY;

		if (is_shield(o) || is_weapon(o) || (objects[o->otyp].oc_flags & O1_IS_ARMOR_WHEN_WIELDED))
			wearmask |= (W_WEP | W_ARMS);

		item_mc_bonus = 0;
		int otyp = o->otyp;
		boolean inappr = inappropriate_monster_character_type(mon, o);
		boolean worn = !!(o->owornmask & wearmask);

		if (worn)
		{
			/* MC always from worn */
			if (objects[o->otyp].oc_flags & O1_EROSION_DOES_NOT_AFFECT_MC)
				item_mc_bonus += objects[o->otyp].oc_magic_cancellation;
			else
				item_mc_bonus += max(0, objects[o->otyp].oc_magic_cancellation - (int)greatest_erosion(o));

			if (objects[o->otyp].oc_flags & O1_SPE_AFFECTS_MC)
				item_mc_bonus += o->spe;
		}

		if ((worn || (!worn && (objects[otyp].oc_pflags & P1_ATTRIBUTE_BONUS_APPLIES_WHEN_CARRIED)))
			&& ((!inappr && !(objects[otyp].oc_pflags & (P1_ATTRIBUTE_BONUS_APPLIES_TO_INAPPROPRIATE_CHARACTERS_ONLY)))
				|| (objects[otyp].oc_pflags & P1_ATTRIBUTE_BONUS_APPLIES_TO_ALL_CHARACTERS)
				|| (inappr && (objects[otyp].oc_pflags & (P1_ATTRIBUTE_BONUS_APPLIES_TO_INAPPROPRIATE_CHARACTERS_ONLY | P1_ATTRIBUTE_BONUS_NEGATIVE_TO_INAPPROPRIATE_CHARACTERS)))
				)
			)
		{

			/* Note u.umcbonus is not being used at the moment, even though it contains appropriate bonuses for you */
			if (objects[o->otyp].oc_bonus_attributes & BONUS_TO_MC)
			{
				item_mc_bonus += objects[o->otyp].oc_attribute_bonus;
				if (!(objects[o->otyp].oc_bonus_attributes & IGNORE_SPE))
					item_mc_bonus += o->spe;
			}
		}

		if (o->owornmask == W_ARM)
			suit_mc_bonus = item_mc_bonus;
		else if (o->owornmask == W_ARMO)
			robe_mc_bonus = item_mc_bonus;
		else
			mc += item_mc_bonus;
	}

	/* Finally, add greated of suit and robe MC bonus */
	combined_mc_bonus = max(suit_mc_bonus, robe_mc_bonus);
	mc += combined_mc_bonus;
	if (is_you)
	{
		context.suit_yielding_mc_bonus = (suit_mc_bonus == combined_mc_bonus);
		context.robe_yielding_mc_bonus = (robe_mc_bonus == combined_mc_bonus);
	}

	if (is_you)
	{
		if (Magical_barkskin)
			mc += 7;
		else if (Protection)
			mc += 3;
	}
	else
	{
		if (mon->mprops[MAGICAL_BARKSKIN] != 0)
			mc += 7;
		else if (mon->mprops[PROTECTION] != 0)
			mc += 3;
	}

    return mc;
}


int
magic_negation_percentage(mclevel)
int mclevel;
{
	if (mclevel <= 0)
		return 0;
	if (mclevel > 20)
		return 100;

	int mcpercentage = 0;

	switch (mclevel)
	{
	case 1:
		mcpercentage = 10;
		break;
	case 2:
		mcpercentage = 20;
		break;
	case 3:
		mcpercentage = 30;
		break;
	case 4:
		mcpercentage = 40;
		break;
	case 5:
		mcpercentage = 50;
		break;
	case 6:
		mcpercentage = 60;
		break;
	case 7:
		mcpercentage = 65;
		break;
	case 8:
		mcpercentage = 70;
		break;
	case 9:
		mcpercentage = 75;
		break;
	case 10:
		mcpercentage = 80;
		break;
	case 11:
		mcpercentage = 84;
		break;
	case 12:
		mcpercentage = 87;
		break;
	case 13:
		mcpercentage = 90;
		break;
	case 14:
		mcpercentage = 93;
		break;
	case 15:
		mcpercentage = 95;
		break;
	case 16:
		mcpercentage = 96;
		break;
	case 17:
		mcpercentage = 97;
		break;
	case 18:
		mcpercentage = 98;
		break;
	case 19:
		mcpercentage = 99;
		break;
	case 20:
		mcpercentage = 100;
		break;
	default:
		mcpercentage = 0;
		break;
	}

	return mcpercentage;
}


/*
 * hitmu: monster hits you
 *        returns 2 if monster dies (e.g. "yellow light"), 1 otherwise
 *        3 if the monster lives but teleported/paralyzed, so it can't keep
 *             attacking you
 */
STATIC_OVL int
hitmu(mtmp, mattk, omonwep)
register struct monst *mtmp;
register struct attack *mattk;
register struct obj* omonwep;
{
    struct permonst *mdat = mtmp->data;
    int uncancelled, ptmp;
	int dmg = 0, extradmg = 0, permdmg, tmphp;
    char buf[BUFSZ];
    struct permonst *olduasmon = youmonst.data;
    int res;
	boolean objectshatters = FALSE;
	boolean isdisintegrated = FALSE;
	int critstrikeroll = rn2(100);

    if (!canspotmon(mtmp))
        map_invisible(mtmp->mx, mtmp->my);

    /*  If the monster is undetected & hits you, you should know where
     *  the attack came from.
     */
    if (mtmp->mundetected && (hides_under(mdat) || mdat->mlet == S_EEL)) {
        mtmp->mundetected = 0;
        if (!(Blind ? (Blind_telepat || Unblind_telepat) : Unblind_telepat)) {
            struct obj *obj;
            const char *what;

            if ((obj = level.objects[mtmp->mx][mtmp->my]) != 0) {
                if (Blind && !obj->dknown)
                    what = something;
                else if (is_pool(mtmp->mx, mtmp->my) && !Underwater)
                    what = "the water";
                else
                    what = doname(obj);

                pline("%s was hidden under %s!", Amonnam(mtmp), what);
            }
            newsym(mtmp->mx, mtmp->my);
        }
    }

	dmg = 0;
	dmg += mtmp->mdaminc;
    /*  First determine the base damage done */
	struct obj* mweapon = omonwep; // MON_WEP(mtmp);
	boolean uses_spell_flags = omonwep ? object_uses_spellbook_wand_flags_and_properties(omonwep) : FALSE;

	if (mweapon)
	{
		//Use weapon damage
		if (is_launcher(mweapon))
			dmg += d(1, 2);
		else
		{
			dmg += weapon_dmg_value(mweapon, &youmonst, mtmp);
			extradmg += weapon_extra_dmg_value(mweapon, &youmonst, mtmp, dmg);
			dmg += extradmg;
		}
	}
	else
	{
		//Use stats from ATTK
		if (mattk->damn > 0 && mattk->damd > 0)
			dmg += d((int)mattk->damn, (int)mattk->damd);
		dmg += (int)mattk->damp;
	}
	
	//Get damage bonus in both cases if physical
	if(mattk->adtyp == AD_PHYS || mattk->adtyp == AD_DRIN)
	{
		if(omonwep || mattk->aatyp == AT_WEAP || mattk->aatyp == AT_HUGS)
			dmg += m_str_dmg_bonus(mtmp);
		else
			dmg += m_str_dmg_bonus(mtmp) / 2;
	}

	//Let's add this even if a weapon is being used
    if ((is_undead(mdat) || is_vampshifter(mtmp)) && midnight())
	{
		if (mattk->damn > 0 && mattk->damd > 0)
			dmg += d((int) mattk->damn, (int) mattk->damd); /* extra damage */
		dmg += (int)mattk->damp;
	}

	//Make sure damage is at least 1
	if (dmg <= 0)
		dmg = 1;


    /*  Next a cancellation factor.
     *  Use uncancelled when cancellation factor takes into account certain
     *  armor's special magic protection.  Otherwise just use !has_cancelled(mtmp).
     */
	boolean mcsuccess = check_magic_cancellation_success(&youmonst, mattk->mcadj);
	uncancelled = !has_cancelled(mtmp) && !mcsuccess;

    permdmg = 0;

	//This is separately applied to hand-to-hand weapons
	//Random additions so that we can correctly calculate the final damage early
	if ((mattk->adtyp == AD_PHYS && mattk->aatyp == AT_HUGS && !sticks(youmonst.data)) || mattk->adtyp == AD_DRIN)
	{
		/* Strength bonus*/
		//dmg += m_str_dmg_bonus(mtmp);

	}
	else {
		/*This happens almost always*/
		/*  Negative armor class reduces damage done instead of fully protecting against hits. */
		if (dmg && u.uac < 0) {
			dmg -= rnd(-u.uac);
			if (dmg < 1)
				dmg = 1;
		}
	}
	if ((mattk->adtyp == AD_PHYS && mattk->aatyp == AT_HUGS && !sticks(youmonst.data)))
	{
		//Do nothing
	} else	{
		if (dmg) {
			if (Half_physical_damage
				|| (Half_physical_damage_against_undead_and_demons
					&& (is_undead(mtmp->data) || is_demon(mtmp->data)
						|| is_vampshifter(mtmp))))
				dmg = (dmg + 1) / 2;
		}
	}

	if (Invulnerable)
		dmg = 0;

	/*  Now, adjust damages via resistances or specific attacks */
    switch (mattk->adtyp) {
    case AD_PHYS:
        if (mattk->aatyp == AT_HUGS && !sticks(youmonst.data)) {
            if (!u.ustuck && rn2(2)) {
                if (u_slip_free(mtmp, mattk)) {
                    dmg = 0;
                } else {
                    u.ustuck = mtmp;
					if(dmg)
	                    pline("%s grabs you! You sustain %d damage.", Monnam(mtmp), dmg);
					else
						pline("%s grabs you!", Monnam(mtmp));

                }
            } else if (u.ustuck == mtmp) {
                exercise(A_STR, FALSE);
				if (dmg)
					You("are being %s. You sustain %d damage.", (mtmp->data == &mons[PM_ROPE_GOLEM])
                                         ? "choked"
                                         : "crushed", dmg);
				else
					You("are being %s, but sustain no damage.", (mtmp->data == &mons[PM_ROPE_GOLEM])
						? "choked"
						: "crushed");
			}
        } else { /* hand to hand weapon */
			struct obj* otmp = omonwep; //mon_currwep;

            if (mattk->aatyp == AT_WEAP && otmp)
			{
                if (otmp->otyp == CORPSE && touch_petrifies(&mons[otmp->corpsenm])) 
				{
                    dmg = 1;
                    pline("%s hits you with the %s corpse.", Monnam(mtmp),
                          mons[otmp->corpsenm].mname);
                    if (!Stoned)
                        goto do_stone;
                }

				/* Negative AC reduces damage */
				if (dmg && u.uac < 0) 
				{
					dmg -= rnd(-u.uac);
					if (dmg < 1)
						dmg = 1;
				}
				if (dmg) 
				{
					if (Half_physical_damage
						|| (Half_physical_damage_against_undead_and_demons
							&& (is_undead(mtmp->data) || is_demon(mtmp->data)|| is_vampshifter(mtmp)))
						)
						dmg = (dmg + 1) / 2;
				}


				if (Invulnerable)
					dmg = 0;

				boolean hittxt = FALSE;

				hittxt = (otmp->oartifact
					&& artifact_hit(mtmp, &youmonst, otmp, &dmg, dieroll));

				int special_hit_dmg = pseudo_artifact_hit(mtmp, &youmonst, otmp, extradmg, dieroll, critstrikeroll);
				if (special_hit_dmg < 0)
				{
					dmg += 2 * (Upolyd ? u.mh : u.uhp) + 200;
					isdisintegrated = TRUE;
					hittxt = TRUE; /* This means that hit text is already given */
				}
				else if (special_hit_dmg > 0)
				{
					dmg += special_hit_dmg;
				}

				boolean silvermsg = FALSE;
				if (objects[otmp->otyp].oc_material == MAT_SILVER && Hate_silver)
				{
					dmg += rnd(20);
					silvermsg = TRUE;
				}

				//Finally, display damage caused
				if(!hittxt)
					hitmsg(mtmp, mattk, dmg);

				/* Silver message immediately next */
				if (silvermsg) 
				{
					pline_The("silver sears your flesh!");
					exercise(A_CON, FALSE);
				}


				/* Check if the object should shatter */
				if (omonwep && objects[omonwep->otyp].oc_material == MAT_GLASS
					&& !(objects[omonwep->otyp].oc_flags & O1_INDESTRUCTIBLE)
					&& !is_quest_artifact(omonwep)
					&& !omonwep->oartifact
					)
				{
					/* Shattering is done below, here just the message */
					objectshatters = TRUE;
					if (omonwep->quan == 1)
						pline("%s %s shatters from the blow!", s_suffix(Monnam(mtmp)), xname(omonwep));
					else
						pline("One of %s %s shatters from the blow!", s_suffix(mon_nam(mtmp)), xname(omonwep));
				}


				if (!dmg)
                    break;


				/* this redundancy necessary because you have
                   to take the damage _before_ being cloned;
                   need to have at least 2 hp left to split */
                if (u.mh - dmg > 1 && (objects[otmp->otyp].oc_material == MAT_IRON
                        /* relevant 'metal' objects are scalpel and tsurugi */
                        || objects[otmp->otyp].oc_material == MAT_METAL)
                    && (u.umonnum >= 0 && does_split_upon_hit(&mons[u.umonnum])))
				{
                    if (dmg > 1)
                        exercise(A_STR, FALSE);
                    /* inflict damage now; we know it can't be fatal */
                    u.mh -= dmg;
                    context.botl = 1;
                    dmg = 0; /* don't inflict more damage below */
                    if (cloneu())
                        You("divide as %s hits you!", mon_nam(mtmp));
                }
                rustm(&youmonst, otmp);
            } else if (mattk->aatyp != AT_TUCH || dmg != 0
                       || mtmp != u.ustuck)
                hitmsg(mtmp, mattk, dmg);
        }
        break;
    case AD_DISE:
		if (Sick_resistance)
			dmg = 0;

		hitmsg(mtmp, mattk, dmg);

		//Must now bypass your MC -- Demogorgon has high penalties for saving throw
		if (!mcsuccess)
		{
			(void)diseasemu(mdat);
		}
		break;
    case AD_FIRE:
		hitmsg(mtmp, mattk, -1);
		if (uncancelled) {
            if (completelyburns(youmonst.data)) { /* paper or straw golem */
                You("go up in flames!");
                /* KMH -- this is okay with unchanging */
                rehumanize();
                break;
            } else if (Fire_resistance || !dmg) {
                pline("You are %s, but the fire doesn't feel hot!", on_fire(youmonst.data, mattk));
                dmg = 0;
            } else
				pline("You're %s! You sustain %d damage.", on_fire(youmonst.data, mattk), dmg);

            if ((int) mtmp->m_lev > rn2(20))
                destroy_item(SCROLL_CLASS, AD_FIRE);
            if ((int) mtmp->m_lev > rn2(20))
                destroy_item(POTION_CLASS, AD_FIRE);
            if ((int) mtmp->m_lev > rn2(25))
                destroy_item(SPBOOK_CLASS, AD_FIRE);
            burn_away_slime();
        } else
            dmg = 0;
		break;
    case AD_COLD:
		hitmsg(mtmp, mattk, -1);
		if (uncancelled) {
            if (Cold_resistance || !dmg) {
                pline("You're covered in frost, but the frost doesn't feel cold!");
                dmg = 0;
            } else
				pline("You're covered in frost! You sustain %d damage.", dmg);

            if ((int) mtmp->m_lev > rn2(20))
                destroy_item(POTION_CLASS, AD_COLD);
        } else
            dmg = 0;
		break;
    case AD_ELEC:
		hitmsg(mtmp, mattk, -1);
		if (uncancelled) {
            if (Shock_resistance || !dmg) {
				You("get zapped, but the zap doesn't shock you!");
				//pline_The("zap doesn't shock you!");
                dmg = 0;
            }
			else
				You("get zapped for %d damage!", dmg);

            if ((int) mtmp->m_lev > rn2(20))
                destroy_item(WAND_CLASS, AD_ELEC);
            if ((int) mtmp->m_lev > rn2(20))
                destroy_item(RING_CLASS, AD_ELEC);
        } else
            dmg = 0;
		break;
    case AD_SLEE:
		hitmsg(mtmp, mattk, dmg);
		if (uncancelled && multi >= 0 && !rn2(5)) {
            if (Sleep_resistance)
                break;
            fall_asleep(-rn1(3, 8), TRUE);
            if (Blind)
                You("are put to sleep!");
            else
                You("are put to sleep by %s!", mon_nam(mtmp));
        }
		break;
    case AD_BLND:
        if (can_blnd(mtmp, &youmonst, mattk->aatyp, (struct obj *) 0)) {
            if (!Blind)
                pline("%s blinds you!", Monnam(mtmp));
            make_blinded(Blinded + (long) dmg, FALSE);
            if (!Blind)
                Your1(vision_clears);
        }
        dmg = 0;
        break;
    case AD_DRST:
        ptmp = A_STR;
        goto dopois;
    case AD_DRDX:
        ptmp = A_DEX;
        goto dopois;
    case AD_DRCO:
        ptmp = A_CON;
 dopois:
        hitmsg(mtmp, mattk, dmg);
        if (uncancelled && !rn2(8)) {
            Sprintf(buf, "%s %s", s_suffix(Monnam(mtmp)),
                    mpoisons_subj(mtmp, mattk));
            poisoned(buf, ptmp, mdat->mname, 30, FALSE);
        }
        break;
    case AD_DRIN:
        hitmsg(mtmp, mattk, dmg);
        if (Brain_protection || !has_head(youmonst.data)) {
            You("don't seem harmed.");
            /* Not clear what to do for green slimes */
            break;
        }
        if (u_slip_free(mtmp, mattk))
            break;

        if (uarmh && rn2(8)) {
            /* not body_part(HEAD) */
            Your("%s blocks the attack to your head.",
                 helm_simple_name(uarmh));
            break;
        }
		//Cause the damage here rather than below
		mdamageu(mtmp, dmg, FALSE);
        dmg = 0; /* don't inflict a second dose below */

        if (!uarmh || uarmh->otyp != DUNCE_CAP) {
            /* eat_brains() will miss if target is mindless (won't
               happen here; hero is considered to retain his mind
               regardless of current shape) or is noncorporeal
               (can't happen here; no one can poly into a ghost
               or shade) so this check for missing is academic */
            if (eat_brains(mtmp, &youmonst, TRUE, (int *) 0) == MM_MISS)
                break;
        }
        /* adjattrib gives dunce cap message when appropriate */
        break;
    case AD_PLYS:
        hitmsg(mtmp, mattk, dmg);
        if (uncancelled && multi >= 0) 
		{
            if (Free_action) 
			{
				if(!HFree_action)
	                You("momentarily stiffen.");
            } 
			else 
			{
                if (Blind)
                    You("are frozen!");
                else
                    You("are frozen by %s!", mon_nam(mtmp));

				incr_itimeout(&HParalyzed, (rnd(8) + 2));
				context.botl = context.botlx = 1;

#if 0
                nomovemsg = You_can_move_again;
                nomul(-(rnd(8) + 2));
                multi_reason = "paralyzed by a monster";
                exercise(A_DEX, FALSE);
#endif
				/* No new paralysis for a while */
				set_itimeout(&HFree_action, 20);
            }
        }
        break;
    case AD_DRLI:
		hitmsg(mtmp, mattk, dmg);
		if (uncancelled && !Drain_resistance) //!rn2(3) && 
		{
            losexp("life drainage");
        }
		break;
    case AD_LEGS: {
        long side = rn2(2) ? RIGHT_SIDE : LEFT_SIDE;
        const char *sidestr = (side == RIGHT_SIDE) ? "right" : "left",
                   *Monst_name = Monnam(mtmp), *leg = body_part(LEG);

        /* This case is too obvious to ignore, but GnollHack is not in
         * general very good at considering height--most short monsters
         * still _can_ attack you when you're flying or mounted.
         * [FIXME: why can't a flying attacker overcome this?]
         */
        if (u.usteed || Levitation || Flying) {
            pline("%s tries to reach your %s %s!", Monst_name, sidestr, leg);
            dmg = 0;
        } else if (!uncancelled) {
            pline("%s nuzzles against your %s %s!", Monnam(mtmp),
                  sidestr, leg);
            dmg = 0;
        } else {
            if (uarmf) {
                if (rn2(2) && (uarmf->otyp == LOW_BOOTS
                               || uarmf->otyp == IRON_SHOES)) {
                    pline("%s pricks the exposed part of your %s %s for %d damage!",
                          Monst_name, sidestr, leg, dmg);
                } else if (!rn2(5)) {
                    pline("%s pricks through your %s boot for %d damage!", Monst_name,
                          sidestr, dmg);
                } else {
                    pline("%s scratches your %s boot!", Monst_name,
                          sidestr);
                    dmg = 0;
                    break;
                }
            } else
                pline("%s pricks your %s %s for %d damage!", Monst_name, sidestr, leg, dmg);

            set_wounded_legs(side, rnd(60 - ACURR(A_DEX)));
            exercise(A_STR, FALSE);
            exercise(A_DEX, FALSE);
        }
        break;
    }
    case AD_STON: /* cockatrice */
        hitmsg(mtmp, mattk, dmg);
        if (!rn2(3))
		{
            if (!uncancelled) //Needs to bypass MC
			{
                if (!Deaf)
                    You_hear("a cough from %s!", mon_nam(mtmp));
            } 
			else
			{
                if (!Deaf)
                    You_hear("%s hissing!", s_suffix(mon_nam(mtmp)));
 do_stone:
                if (!Stoned && !Stone_resistance
                    && !(poly_when_stoned(youmonst.data)
                            && polymon(PM_STONE_GOLEM))) {
                    int kformat = KILLED_BY_AN;
                    const char *kname = mtmp->data->mname;

                    if (mtmp->data->geno & G_UNIQ) {
                        if (!type_is_pname(mtmp->data))
                            kname = the(kname);
                        kformat = KILLED_BY;
                    }
                    make_stoned(5L, (char *) 0, kformat, kname);
                    return 1;
                    /* done_in_by(mtmp, STONING); */
                }
            }
        }
        break;
    case AD_STCK:
        hitmsg(mtmp, mattk, dmg);
        if (!has_cancelled(mtmp) && !u.ustuck && !sticks(youmonst.data))
            u.ustuck = mtmp;
        break;
    case AD_WRAP:
        if ((!has_cancelled(mtmp) || u.ustuck == mtmp) && !sticks(youmonst.data)) 
		{
            if (!u.ustuck && !rn2(2)) 
			{
                if (u_slip_free(mtmp, mattk)) 
				{
                    dmg = 0;
                } 
				else 
				{
					if(dmg)
						pline("%s swings itself around you. You sustain %d damage!", Monnam(mtmp), dmg);
					else
						pline("%s swings itself around you!", Monnam(mtmp));

					if (is_pool(mtmp->mx, mtmp->my) && !Swimming && !Amphibious)
						pline("%s is drowning you!", Monnam(mtmp));
					else if (is_constrictor(mtmp->data))
						pline("%s is constricting you to death!", Monnam(mtmp));

					u.ustuck = mtmp;
					context.botl = TRUE;
                }
            } 
			else if (u.ustuck == mtmp) 
			{
                if (is_pool(mtmp->mx, mtmp->my) && !Swimming && !Amphibious)
				{
					/* Drowning is now delayed death */
					pline("%s is drowning you!", Monnam(mtmp));

					/*
					boolean moat = (levl[mtmp->mx][mtmp->my].typ != POOL)
								   && (levl[mtmp->mx][mtmp->my].typ != WATER)
								   && !Is_medusa_level(&u.uz)
								   && !Is_waterlevel(&u.uz);


					killer.format = KILLED_BY_AN;
                    Sprintf(killer.name, "%s by %s",
                            moat ? "moat" : "pool of water",
                            an(mtmp->data->mname));
                    done(DROWNING);
					*/
                } 
				else if (is_constrictor(mtmp->data))
					pline("%s is constricting you to death!", Monnam(mtmp));
				else if (mattk->aatyp == AT_HUGS)
					if(dmg)
						You("are being crushed, sustaining %d damage.", dmg);
					else
						You("are being crushed, but sustain no damage.");
            } 
			else 
			{
                dmg = 0;
                if (flags.verbose)
                    pline("%s brushes against your %s.", Monnam(mtmp),
                          body_part(LEG));
            }
        }
		else
            dmg = 0;
        break;
    case AD_WERE:
        hitmsg(mtmp, mattk, dmg);
        if (uncancelled /* && !rn2(4)*/ && u.ulycn == NON_PM
            && !Protection_from_shape_changers && !Lycanthropy_resistance)
		{
            You_feel("feverish.");
            exercise(A_CON, FALSE);
            set_ulycn(monsndx(mdat));
            retouch_equipment(2);
        }
        break;
    case AD_SGLD:
        hitmsg(mtmp, mattk, dmg);
        if (youmonst.data->mlet == mdat->mlet)
            break;
        if (uncancelled)
            stealgold(mtmp);
        break;

    case AD_SSEX:
        if (SYSOPT_SEDUCE) {
            if (could_seduce(mtmp, &youmonst, mattk) == 1 && uncancelled && !resists_charm(&youmonst) && !Charm_resistance)
                if (doseduce(mtmp))
                    return 3;
            break;
        }
        /*FALLTHRU*/
    case AD_SITM: /* for now these are the same */
    case AD_SEDU:
	{
		boolean nymphcancelled = !uncancelled;

        if (is_animal(mtmp->data))
		{
            hitmsg(mtmp, mattk, -1);
            if (nymphcancelled)
                break;
            /* Continue below */
        } 
		else if (dmgtype(youmonst.data, AD_SEDU)
                   /* !SYSOPT_SEDUCE: when hero is attacking and AD_SSEX
                      is disabled, it would be changed to another damage
                      type, but when defending, it remains as-is */
                   || dmgtype(youmonst.data, AD_SSEX))
		{
            pline("%s %s.", Monnam(mtmp),
                  Deaf ? "says something but you can't hear it"
                       : mtmp->minvent
                      ? "brags about the goods some dungeon explorer provided"
                  : "makes some remarks about how difficult theft is lately");
            if (!tele_restrict(mtmp))
                (void) rloc(mtmp, TRUE);
            return 3;
        } 
		else if (nymphcancelled || resists_charm(&youmonst) || Charm_resistance) 
		{
            if (!Blind)
                pline("%s tries to %s you, but you seem %s.",
                      Adjmonnam(mtmp, "plain"),
                      flags.female ? "charm" : "seduce",
                      flags.female ? "unaffected" : "uninterested");
            if (rn2(3)) 
			{
                if (!tele_restrict(mtmp))
                    (void) rloc(mtmp, TRUE);
                return 3;
            }
            break;
        }
        buf[0] = '\0';
        switch (steal(mtmp, buf)) {
        case -1:
            return 2;
        case 0:
            break;
        default:
            if (!is_animal(mtmp->data) && !tele_restrict(mtmp))
                (void) rloc(mtmp, TRUE);
            if (is_animal(mtmp->data) && *buf) {
                if (canseemon(mtmp))
                    pline("%s tries to %s away with %s.", Monnam(mtmp),
                          locomotion(mtmp->data, "run"), buf);
            }
            monflee(mtmp, 0, FALSE, FALSE);
            return 3;
        }
        break;
	}
    case AD_SAMU:
        hitmsg(mtmp, mattk, dmg);
        /* when the Wizard or quest nemesis hits, there's a 1/20 chance
           to steal a quest artifact (any, not just the one for the hero's
           own role) or the Amulet or one of the invocation tools */
        if (!rn2(20))
            stealamulet(mtmp);
        break;

    case AD_TLPT:
        hitmsg(mtmp, mattk, dmg);
        if (uncancelled) {
            if (flags.verbose)
                Your("position suddenly seems %suncertain!",
                     (Teleport_control && !Stunned && !unconscious()) ? ""
                     : "very ");
            tele();
            /* 3.6.2:  make sure damage isn't fatal; previously, it
               was possible to be teleported and then drop dead at
               the destination when QM's 1d4 damage gets applied below;
               even though that wasn't "wrong", it seemed strange,
               particularly if the teleportation had been controlled
               [applying the damage first and not teleporting if fatal
               is another alternative but it has its own complications] */
            if ((Half_physical_damage ? (dmg - 1) / 2 : dmg)
                >= (tmphp = (Upolyd ? u.mh : u.uhp))) {
                dmg = tmphp - 1;
                if (Half_physical_damage)
                    dmg *= 2; /* doesn't actually increase damage; we only
                               * get here if half the original damage would
                               * would have been fatal, so double reduced
                               * damage will be less than original damage */
                if (dmg < 1) { /* implies (tmphp <= 1) */
                    dmg = 1;
                    /* this might increase current HP beyond maximum HP but
                       it will be immediately reduced below, so that should
                       be indistinguishable from zero damage; we don't drop
                       damage all the way to zero because that inhibits any
                       passive counterattack if poly'd hero has one */
                    if (Upolyd && u.mh == 1)
                        ++u.mh;
                    else if (!Upolyd && u.uhp == 1)
                        ++u.uhp;
                    /* [don't set context.botl here] */
                }
            }
        }
        break;
    case AD_RUST:
        hitmsg(mtmp, mattk, dmg);
        if (has_cancelled(mtmp))
            break;
        if (u.umonnum == PM_IRON_GOLEM) {
            You("rust!");
            /* KMH -- this is okay with unchanging */
            rehumanize();
            break;
        }
        erode_armor(&youmonst, ERODE_RUST);
        break;
    case AD_CORR:
        hitmsg(mtmp, mattk, dmg);
        if (has_cancelled(mtmp))
            break;
        erode_armor(&youmonst, ERODE_CORRODE);
        break;
    case AD_DCAY:
        hitmsg(mtmp, mattk, dmg);
        if (has_cancelled(mtmp))
            break;
        if (u.umonnum == PM_WOOD_GOLEM || u.umonnum == PM_LEATHER_GOLEM) {
            You("rot!");
            /* KMH -- this is okay with unchanging */
            rehumanize();
            break;
        }
        erode_armor(&youmonst, ERODE_ROT);
        break;
    case AD_HEAL:
        /* a cancelled nurse is just an ordinary monster,
         * nurses don't heal those that cause petrification */
        if (has_cancelled(mtmp) || (Upolyd && touch_petrifies(youmonst.data))) {
            hitmsg(mtmp, mattk, dmg);
            break;
        }
        if (!uwep && !uarmu && !uarm && !uarmc && !uarmo && !uarmb
            && !uarms && !uarmg && !uarmf && !uarmh) {
            boolean goaway = FALSE;

			int healamount = 0;
			if (Upolyd) {
				int starthp = u.mh;
				u.mh += rnd(7);
                if (!rn2(7)) {
                    /* no upper limit necessary; effect is temporary */
                    u.basemhmax++;
                    if (!rn2(13))
                        goaway = TRUE;
                }
				updatemaxhp();
				healamount = u.mh - starthp;
			} else {
				int starthp = u.uhp;
				u.uhp += rnd(7);
                if (!rn2(7)) {
                    /* hard upper limit via nurse care: 25 * ulevel */
                    if (u.uhpmax < 5 * u.ulevel + d(2 * u.ulevel, 10))
                        u.ubasehpmax++;
                    if (!rn2(13))
                        goaway = TRUE;
                }
				updatemaxhp();
				healamount = u.uhp - starthp;
			}
			if(healamount > 0)
				pline("%s hits you for %d healing!", Monnam(mtmp), healamount);
			else
				pline("%s hits you but nothing happens.", Monnam(mtmp));

			if (!rn2(3))
                exercise(A_STR, TRUE);
            if (!rn2(3))
                exercise(A_CON, TRUE);
            if (Sick)
                make_sick(0L, (char *) 0, FALSE, SICK_ALL);
            context.botl = 1;
            if (goaway) {
                mongone(mtmp);
                return 2;
            } else if (!rn2(33)) {
                if (!tele_restrict(mtmp))
                    (void) rloc(mtmp, TRUE);
                monflee(mtmp, d(3, 6), TRUE, FALSE);
                return 3;
            }
            dmg = 0;
        } else {
            if (Role_if(PM_HEALER)) {
                if (!Deaf && !(moves % 5))
                    verbalize("Doc, I can't help you unless you cooperate.");
                dmg = 0;
            } else
                hitmsg(mtmp, mattk, dmg);
        }
        break;
    case AD_CURS:
        hitmsg(mtmp, mattk, dmg);
        if (!night() && mdat == &mons[PM_GREMLIN])
            break;
        if (!has_cancelled(mtmp) && !rn2(10)) {
            if (!Deaf) {
                if (Blind)
                    You_hear("laughter.");
                else
                    pline("%s chuckles.", Monnam(mtmp));
            }
            if (u.umonnum == PM_CLAY_GOLEM) {
                pline("Some writing vanishes from your head!");
                /* KMH -- this is okay with unchanging */
                rehumanize();
                break;
            }
            attrcurse();
        }
        break;
    case AD_STUN:
		if (uncancelled && !rn2(4)) {
			hitmsg(mtmp, mattk, dmg / 2);
			make_stunned((HStun & TIMEOUT) + (long) dmg, TRUE);
            dmg /= 2;
        } else
			hitmsg(mtmp, mattk, dmg);

		break;
    case AD_ACID:
		if (!has_cancelled(mtmp) && !rn2(3))
            if (Acid_resistance) {
				hitmsg(mtmp, mattk, -1);
				pline("You're covered in %s, but it seems harmless.",
                      hliquid("acid"));
                dmg = 0;
            } else {
				hitmsg(mtmp, mattk, dmg);
				pline("You're covered in %s!  It burns!", hliquid("acid"));
                exercise(A_STR, FALSE);
            }
        else {
			hitmsg(mtmp, mattk, -1);
			dmg = 0;
		}
		break;
    case AD_SLOW:
        hitmsg(mtmp, mattk, dmg);
        if (uncancelled && HFast)
            u_slow_down();
        break;
    case AD_DREN:
        hitmsg(mtmp, mattk, -1);
        if (uncancelled)
            drain_en(dmg);
        dmg = 0;
        break;
    case AD_CONF:
        hitmsg(mtmp, mattk, -1);
        if (!has_cancelled(mtmp) && !mtmp->mspec_used) {
            mtmp->mspec_used = mtmp->mspec_used + (dmg + rn2(6));
            if (Confusion)
                You("are getting even more confused.");
            else
                You("are getting confused.");
            make_confused(itimeout_incr(HConfusion, dmg), FALSE);
        }
        dmg = 0;
        break;
    case AD_DETH:
	{
        pline("%s reaches out with its deadly touch.", Monnam(mtmp));
		boolean magic_resistance_success = check_magic_resistance_and_halve_damage(&youmonst, (struct obj*)0, mtmp->m_lev, 0, NOTELL);
        if (is_not_living(youmonst.data) || is_demon(youmonst.data) || Death_resistance || magic_resistance_success) 
		{
            /* Still does normal damage */
            pline("Was that the touch of death?");
            break;
        }
		int mcadj = 0;
        switch (rn2(20)) {
        case 19:
			mcadj -= 4;
        case 18:
			mcadj -= 4;
		case 17:
			mcadj -= 4;
			if (!Antimagic && !check_magic_cancellation_success(&youmonst, mcadj)) {
                killer.format = KILLED_BY_AN;
                Strcpy(killer.name, "touch of death");
                done(DIED);
                dmg = 0;
                break;
            }
            /*FALLTHRU*/
        default: /* case 16: ... case 5: */
            permdmg = 1; /* actual damage done below */
			You_feel("your life force draining away... You lose %d hit points, some of them permanently.", dmg);
			break;
        case 4:
        case 3:
        case 2:
        case 1:
        case 0:
            if (Antimagic)
                shieldeff(u.ux, u.uy);
            pline("Lucky for you, it didn't work!");
            dmg = 0;
            break;
        }
        break;
	}
    case AD_PEST:
        pline("%s reaches out, and you feel fever and chills. You lose %d hit points.", Monnam(mtmp), dmg);
		(void) diseasemu(mdat); /* plus the normal damage */
        break;
    case AD_FAMN:
        pline("%s reaches out, and your body shrivels. You lose %d hit points.", Monnam(mtmp), dmg);
        exercise(A_CON, FALSE);
        if (!is_fainted())
            morehungry(rn1(40, 40));
        /* plus the normal damage */
        break;
    case AD_SLIM:
        if (!uncancelled)
            break;
        if (flaming(youmonst.data)) {
			hitmsg(mtmp, mattk, -1);
			pline_The("slime burns away!");
            dmg = 0;
        } else if (Unchanging || noncorporeal(youmonst.data)
                   || youmonst.data == &mons[PM_GREEN_SLIME]) {
			hitmsg(mtmp, mattk, -1);
			You("are unaffected.");
            dmg = 0;
        } else if (!Slimed) {
			hitmsg(mtmp, mattk, dmg);
			You("don't feel very well.");
            make_slimed(10L, (char *) 0);
            delayed_killer(SLIMED, KILLED_BY_AN, mtmp->data->mname);
		}
		else {
			hitmsg(mtmp, mattk, dmg);
			pline("Yuck!");
		}
        break;
    case AD_ENCH: /* KMH -- remove enchantment (disenchanter) */
        hitmsg(mtmp, mattk, dmg);
        /* uncancelled is sufficient enough; please
           don't make this attack less frequent */
        if (uncancelled) {
            struct obj *obj = some_armor(&youmonst);

            if (!obj) {
                /* some rings are susceptible;
                   amulets and blindfolds aren't (at present) */
                switch (rn2(5)) {
                case 0:
                    break;
                case 1:
                    obj = uright;
                    break;
                case 2:
                    obj = uleft;
                    break;
                case 3:
                    obj = uamul;
                    break;
                case 4:
                    obj = ublindf;
                    break;
                }
            }
            if (drain_item(obj, FALSE)) {
                pline("%s less effective.", Yobjnam2(obj, "seem"));
            }
        }
        break;
    default:
        dmg = 0;
        break;
    }
    if ((Upolyd ? u.mh : u.uhp) < 1) {
        /* already dead? call rehumanize() or done_in_by() as appropriate */
        mdamageu(mtmp, 1, FALSE);
        dmg = 0;
    }

#if 0
	//Black blade damage adjustment
	if (mattk->aatyp == AT_WEAP && omonwep && omonwep->otyp == BLACK_BLADE_OF_DISINTEGRATION &&
		!(Disint_resistance || noncorporeal(youmonst.data) || Invulnerable)) {
		dmg  = 0; //Black blade does not do ordinary damage to disintegrateable monsters, to resistant monsters it inflicts normal damage
	}
#endif

	int permdmg2 = 0;

	/* Wounding */
	if (mattk->aatyp == AT_WEAP && omonwep && !uses_spell_flags && (objects[omonwep->otyp].oc_aflags & A1_WOUNDING) && eligible_for_extra_damage(omonwep, &youmonst, mtmp)
		&& (
		((objects[omonwep->otyp].oc_aflags & A1_USE_CRITICAL_STRIKE_PERCENTAGE_FOR_SPECIAL_ATTACK_TYPES)
			&& (
			((objects[omonwep->otyp].oc_aflags & A1_CRITICAL_STRIKE_PERCENTAGE_IS_A_DIE_ROLL)
				&& dieroll <= objects[omonwep->otyp].oc_critical_strike_percentage)
				||
				(!(objects[omonwep->otyp].oc_aflags & A1_CRITICAL_STRIKE_PERCENTAGE_IS_A_DIE_ROLL)
					&& critstrikeroll < objects[omonwep->otyp].oc_critical_strike_percentage))
			)
			||
			(!(objects[omonwep->otyp].oc_aflags & A1_USE_CRITICAL_STRIKE_PERCENTAGE_FOR_SPECIAL_ATTACK_TYPES)
				&& 1)
			)
		)
	{
		if (objects[omonwep->otyp].oc_aflags & A1_USE_FULL_DAMAGE_INSTEAD_OF_EXTRA)
			extradmg = dmg;

		permdmg2 = extradmg;

		if (extradmg > 0)
		{
			pline("%s's %s %s you deeply!", Monnam(mtmp), cxname(omonwep), otense (omonwep, "cut"));
		}
	}

	/* Life leech */
	if (mattk->aatyp == AT_WEAP && omonwep && !uses_spell_flags && (objects[omonwep->otyp].oc_aflags & A1_LIFE_LEECH) && eligible_for_extra_damage(omonwep, &youmonst, mtmp)
		&& (
		((objects[omonwep->otyp].oc_aflags & A1_USE_CRITICAL_STRIKE_PERCENTAGE_FOR_SPECIAL_ATTACK_TYPES)
			&& (
			((objects[omonwep->otyp].oc_aflags & A1_CRITICAL_STRIKE_PERCENTAGE_IS_A_DIE_ROLL)
				&& dieroll <= objects[omonwep->otyp].oc_critical_strike_percentage)
				||
				(!(objects[omonwep->otyp].oc_aflags & A1_CRITICAL_STRIKE_PERCENTAGE_IS_A_DIE_ROLL)
					&& critstrikeroll < objects[omonwep->otyp].oc_critical_strike_percentage))
			)
			||
			(!(objects[omonwep->otyp].oc_aflags & A1_USE_CRITICAL_STRIKE_PERCENTAGE_FOR_SPECIAL_ATTACK_TYPES)
				&& 1)
			)
		)
	{
		if (objects[omonwep->otyp].oc_aflags & A1_USE_FULL_DAMAGE_INSTEAD_OF_EXTRA)
			extradmg = dmg;

		mtmp->mhp += extradmg;
		if (mtmp->mhp > mtmp->mhpmax)
			mtmp->mhp = mtmp->mhpmax;

		if (extradmg > 0)
		{
			pline("%s's %s %s your life energy!", Monnam(mtmp), cxname(omonwep), otense(omonwep, "leech"));
		}
	}

	int oldumort = u.umortality;

	if (Invulnerable)
	{
		dmg = 0;
		permdmg = 0;
		permdmg2 = 0;
	}

	if (dmg) {
		if (permdmg > 0) { /* Death's life force drain */
			int lowerlimit, * hpmax_p;
			/*
			 * Apply some of the damage to permanent hit points:
			 *  polymorphed         100% against poly'd hpmax
			 *  hpmax > 25*lvl      100% against normal hpmax
			 *  hpmax > 10*lvl  50..100%
			 *  hpmax >  5*lvl  25..75%
			 *  otherwise        0..50%
			 * Never reduces hpmax below 1 hit point per level.
			 */
			permdmg = rn2(dmg / 2 + 1);
			if (Upolyd || u.uhpmax > 25 * u.ulevel)
				permdmg = dmg;
			else if (u.uhpmax > 10 * u.ulevel)
				permdmg += dmg / 2;
			else if (u.uhpmax > 5 * u.ulevel)
				permdmg += dmg / 4;

			if (Upolyd) {
				hpmax_p = &u.basemhmax;
				/* [can't use youmonst.m_lev] */
				lowerlimit = min((int)youmonst.data->mlevel, u.ulevel);
			}
			else {
				hpmax_p = &u.ubasehpmax;
				lowerlimit = u.ulevel;
			}
			if (*hpmax_p - permdmg > lowerlimit)
				* hpmax_p -= permdmg;
			else if (*hpmax_p > lowerlimit)
				* hpmax_p = lowerlimit;

			updatemaxhp();
			if (Upolyd)
			{
				if (u.mh > u.mhmax)
					u.mh = u.mhmax;
			}
			else
			{
				if (u.uhp > u.uhpmax)
					u.uhp = u.uhpmax;
			}
			/* else unlikely...
			 * already at or below minimum threshold; do nothing */
			context.botl = 1;
		}

		mdamageu(mtmp, dmg, FALSE);

		if (permdmg2 > 0)
		{
			if (Upolyd)
			{
				u.basemhmax -= permdmg2;
				updatemaxhp();
				if (u.mh > u.mhmax)
					u.mh = u.mhmax;
			}
			else
			{
				u.ubasehpmax -= permdmg2;
				updatemaxhp();
				if (u.uhp > u.uhpmax)
					u.uhp = u.uhpmax;
			}
			context.botl = 1;
		}

    }

	//Add special enchantments
	if (mattk->aatyp == AT_WEAP && omonwep) {
#if 0
		if (omonwep->otyp == BLACK_BLADE_OF_DISINTEGRATION)
		{
			if (Disint_resistance || noncorporeal(youmonst.data) || Invulnerable) {					// if (abstyp == ZT_BREATH(ZT_DISINTEGRATION)) {
				You("are not disintegrated.");
			}
			else if (uarms) {
				/* destroy shield; other possessions are safe */
				(void)destroy_arm(uarms);
			}
			else if (uarm) {
				/* destroy suit; if present, cloak and robe go too */
				if (uarmc)
					(void)destroy_arm(uarmc);
				if (uarmo)
					(void)destroy_arm(uarmo);
				(void)destroy_arm(uarm);
			}
			else
			{
				/* no shield or suit, you're dead; wipe out cloak
					and/or shirt in case of life-saving or bones */
				if (uarmc)
					(void)destroy_arm(uarmc);
				if (uarmo)
					(void)destroy_arm(uarmo);
				if (uarmu)
					(void)destroy_arm(uarmu);
				killer.format = KILLED_BY_AN;
				Strcpy(killer.name, killer_xname(omonwep));
				/* when killed by disintegration breath, don't leave corpse */
				u.ugrave_arise = -3;
				done(DIED);
			}
		}
#endif
		if (omonwep->elemental_enchantment > 0)
		{
			char onmbuf[BUFSZ], knmbuf[BUFSZ];

			Strcpy(onmbuf, xname(mweapon));
			Strcpy(knmbuf, killer_xname(mweapon));

			extra_enchantment_damage(onmbuf, omonwep->elemental_enchantment, knmbuf, (u.umortality > oldumort));

			switch (omonwep->elemental_enchantment)
			{
			case COLD_ENCHANTMENT:
				if (is_ammo(omonwep) || throwing_weapon(omonwep) || objects[omonwep->otyp].oc_merge ? !rn2(10) : !rn2(50))
					omonwep->elemental_enchantment = 0;
				break;
			case FIRE_ENCHANTMENT:
				if (is_ammo(omonwep) || throwing_weapon(omonwep) || objects[omonwep->otyp].oc_merge ? !rn2(3) : !rn2(15))
					omonwep->elemental_enchantment = 0;
				break;
			case LIGHTNING_ENCHANTMENT:
				if(is_ammo(omonwep) || throwing_weapon(omonwep) || objects[omonwep->otyp].oc_merge ? 1 : !rn2(5))
					omonwep->elemental_enchantment = 0;
				break;
			case DEATH_ENCHANTMENT:
				omonwep->elemental_enchantment = 0;
				break;
			}
		}
	}

	if (omonwep && (objectshatters || (!uses_spell_flags 
		&& ((objects[omonwep->otyp].oc_aflags & A1_ITEM_VANISHES_ON_HIT)
			&& (
				!(objects[omonwep->otyp].oc_aflags & A1_ITEM_VANISHES_ONLY_IF_PERMITTED_TARGET)
				|| ((objects[omonwep->otyp].oc_aflags & A1_ITEM_VANISHES_ONLY_IF_PERMITTED_TARGET) && eligible_for_extra_damage(omonwep, &youmonst, mtmp))
				)
			)
		)
	))
	{
		if(omonwep->where == OBJ_MINVENT)
			m_useup(mtmp, omonwep);
		else if (omonwep->where == OBJ_FLOOR)
		{
			int x = omonwep->ox, y = omonwep->oy;
			delobj(omonwep);
			newsym(x, y);
		}
		else if (omonwep->where == OBJ_FREE)
			obfree(omonwep, (struct obj*)0);
	}

    if (dmg)
        res = passiveum(olduasmon, mtmp, mattk);
    else
        res = 1;
    stop_occupation();
    return res;
}

/* An interface for use when taking a blindfold off, for example,
 * to see if an engulfing attack should immediately take affect, like
 * a passive attack. TRUE if engulfing blindness occurred */
boolean
gulp_blnd_check()
{
    struct attack *mattk;

    if (!Blinded && u.uswallow
        && (mattk = attacktype_fordmg(u.ustuck->data, AT_ENGL, AD_BLND))
        && can_blnd(u.ustuck, &youmonst, mattk->aatyp, (struct obj *) 0)) {
        ++u.uswldtim; /* compensate for gulpmu change */
        (void) gulpmu(u.ustuck, mattk);
        return TRUE;
    }
    return FALSE;
}

/* monster swallows you, or damage if u.uswallow */
STATIC_OVL int
gulpmu(mtmp, mattk)
struct monst *mtmp;
struct attack *mattk;
{
    struct trap *t = t_at(u.ux, u.uy);
	int tmp = 0;
	if(mattk->damn > 0 && mattk->damd > 0)
		tmp += d((int)mattk->damn, (int)mattk->damd);
	tmp += mattk->damp;
	int tim_tmp;
    struct obj *otmp2;
    int i;
    boolean physical_damage = FALSE;

    if (!u.uswallow) { /* swallows you */
        int omx = mtmp->mx, omy = mtmp->my;

        if (!engulf_target(mtmp, &youmonst))
            return 0;
        if ((t && is_pit(t->ttyp))
            && sobj_at(BOULDER, u.ux, u.uy))
            return 0;

        if (Punished)
            unplacebc(); /* ball&chain go away */
        remove_monster(omx, omy);
        mtmp->mtrapped = 0; /* no longer on old trap */
        place_monster(mtmp, u.ux, u.uy);
        u.ustuck = mtmp;
        newsym(mtmp->mx, mtmp->my);
        if (is_animal(mtmp->data) && u.usteed) {
            char buf[BUFSZ];

            /* Too many quirks presently if hero and steed
             * are swallowed. Pretend purple worms don't
             * like horses for now :-)
             */
            Strcpy(buf, mon_nam(u.usteed));
            pline("%s lunges forward and plucks you off %s!", Monnam(mtmp),
                  buf);
            dismount_steed(DISMOUNT_ENGULFED);
        } else
            pline("%s engulfs you!", Monnam(mtmp));
        stop_occupation();
        reset_occupations(); /* behave as if you had moved */

        if (u.utrap) {
            You("are released from the %s!",
                u.utraptype == TT_WEB ? "web" : "trap");
            reset_utrap(FALSE);
        }

        i = number_leashed();
        if (i > 0) {
            const char *s = (i > 1) ? "leashes" : "leash";

            pline_The("%s %s loose.", s, vtense(s, "snap"));
            unleash_all();
        }

        if (touch_petrifies(youmonst.data) && !resists_ston(mtmp)) {
            /* put the attacker back where it started;
               the resulting statue will end up there */
            remove_monster(mtmp->mx, mtmp->my); /* u.ux,u.uy */
            place_monster(mtmp, omx, omy);
			int existing_stoning = get_mon_temporary_property(mtmp, STONED);
			set_mon_property_verbosely(mtmp, STONED, max(1, min(existing_stoning - 1, 5)));
			//minstapetrify(mtmp, TRUE);
            /* normally unstuck() would do this, but we're not
               fully swallowed yet so that won't work here */
            //if (Punished)
            //    placebc();
            //u.ustuck = 0;
            //return (!DEADMONSTER(mtmp)) ? 0 : 2;
        }

        display_nhwindow(WIN_MESSAGE, FALSE);
        vision_recalc(2); /* hero can't see anything */
        u.uswallow = 1;
        /* for digestion, shorter time is more dangerous;
           for other swallowings, longer time means more
           chances for the swallower to attack */
        if (mattk->adtyp == AD_DGST) {
            tim_tmp = 25 - (int) mtmp->m_lev;
            if (tim_tmp > 0)
                tim_tmp = rnd(tim_tmp) / 2;
            else if (tim_tmp < 0)
                tim_tmp = -(rnd(-tim_tmp) / 2);
            /* having good armor & high constitution makes
               it take longer for you to be digested, but
               you'll end up trapped inside for longer too */
            tim_tmp += -u.uac + 10 + (ACURR(A_CON) / 3 - 1);
        } else {
            /* higher level attacker takes longer to eject hero */
            tim_tmp = rnd((int) mtmp->m_lev + 10 / 2);
        }
        /* u.uswldtim always set > 1 */
        u.uswldtim = (unsigned) ((tim_tmp < 2) ? 2 : tim_tmp);
        swallowed(1);
        for (otmp2 = invent; otmp2; otmp2 = otmp2->nobj)
            (void) snuff_lit(otmp2);
    }

    if (mtmp != u.ustuck)
        return 0;
    if (u.uswldtim > 0)
        u.uswldtim -= 1;

    switch (mattk->adtyp) {
    case AD_DGST:
        physical_damage = TRUE;
        if (Slow_digestion) {
            /* Messages are handled below */
            u.uswldtim = 0;
            tmp = 0;
        } else if (u.uswldtim == 0) {
            pline("%s totally digests you!", Monnam(mtmp));
            tmp = u.uhp;
            if (Half_physical_damage)
                tmp *= 2; /* sorry */
        } else {
            pline("%s%s digests you!", Monnam(mtmp),
                  (u.uswldtim == 2) ? " thoroughly"
                                    : (u.uswldtim == 1) ? " utterly" : "");
            exercise(A_STR, FALSE);
        }
        break;
    case AD_PHYS:
        physical_damage = TRUE;
        if (mtmp->data == &mons[PM_FOG_CLOUD]) {
            You("are laden with moisture and %s",
                flaming(youmonst.data)
                    ? "are smoldering out!"
                    : Breathless ? "find it mildly uncomfortable."
                                 : amphibious(youmonst.data)
                                       ? "feel comforted."
                                       : "can barely breathe!");
            /* NB: Amphibious includes Breathless */
            if (Amphibious && !flaming(youmonst.data))
                tmp = 0;
        } else {
            You("are pummeled with debris!");
            exercise(A_STR, FALSE);
        }
        break;
    case AD_ACID:
        if (Acid_resistance || Invulnerable) {
            You("are covered with a seemingly harmless goo.");
            tmp = 0;
        } else {
            if (Hallucination)
                pline("Ouch!  You've been slimed!");
            else
                You("are covered in slime!  It burns!");
            exercise(A_STR, FALSE);
        }
        break;
    case AD_BLND:
        if (can_blnd(mtmp, &youmonst, mattk->aatyp, (struct obj *) 0)) {
            if (!Blind) {
                long was_blinded = Blinded;

                if (!Blinded)
                    You_cant("see in here!");
                make_blinded((long) tmp, FALSE);
                if (!was_blinded && !Blind)
                    Your1(vision_clears);
            } else
                /* keep him blind until disgorged */
                make_blinded(Blinded + 1, FALSE);
        }
        tmp = 0;
        break;
    case AD_ELEC:
        if (!has_cancelled(mtmp) && rn2(2)) {
            pline_The("air around you crackles with electricity.");
            if (Shock_resistance || Invulnerable) {
                shieldeff(u.ux, u.uy);
                You("seem unhurt.");
                ugolemeffects(AD_ELEC, tmp);
                tmp = 0;
            }
        } else
            tmp = 0;
        break;
    case AD_COLD:
        if (!has_cancelled(mtmp) && rn2(2)) {
            if (Cold_resistance || Invulnerable) {
                shieldeff(u.ux, u.uy);
                You_feel("mildly chilly.");
                ugolemeffects(AD_COLD, tmp);
                tmp = 0;
            } else
                You("are freezing to death!");
        } else
            tmp = 0;
        break;
    case AD_FIRE:
        if (!has_cancelled(mtmp) && rn2(2)) {
            if (Fire_resistance || Invulnerable) {
                shieldeff(u.ux, u.uy);
                You_feel("mildly hot.");
                ugolemeffects(AD_FIRE, tmp);
                tmp = 0;
            } else
                You("are burning to a crisp!");
            burn_away_slime();
        } else
            tmp = 0;
        break;
    case AD_DISE:
        if (!diseasemu(mtmp->data))
            tmp = 0;
        break;
    case AD_DREN:
        /* AC magic cancellation doesn't help when engulfed */
        if (!has_cancelled(mtmp) && rn2(4)) /* 75% chance */
            drain_en(tmp);
        tmp = 0;
        break;
    default:
        physical_damage = TRUE;
        tmp = 0;
        break;
    }

    if (physical_damage)
        tmp = Maybe_Half_Phys(tmp);

	if (Invulnerable)
		tmp = 0;

    mdamageu(mtmp, tmp, TRUE);
    if (tmp)
        stop_occupation();

    if (!u.uswallow) {
        ; /* life-saving has already expelled swallowed hero */
    } else if (touch_petrifies(youmonst.data) && !resists_ston(mtmp)) {
        pline("%s very hurriedly %s you!", Monnam(mtmp),
              is_animal(mtmp->data) ? "regurgitates" : "expels");
        expels(mtmp, mtmp->data, FALSE);
    } else if (!u.uswldtim || youmonst.data->msize >= MZ_HUGE) {
        /* 3.6.2: u.uswldtim used to be set to 0 by life-saving but it
           expels now so the !u.uswldtim case is no longer possible;
           however, polymorphing into a huge form while already
           swallowed is still possible */
        You("get %s!", is_animal(mtmp->data) ? "regurgitated" : "expelled");
        if (flags.verbose
            && (is_animal(mtmp->data)
                || (dmgtype(mtmp->data, AD_DGST) && Slow_digestion)))
            pline("Obviously %s doesn't like your taste.", mon_nam(mtmp));
        expels(mtmp, mtmp->data, FALSE);
    }
    return 1;
}

/* monster explodes in your face */
STATIC_OVL int
explmu(mtmp, mattk, ufound)
struct monst *mtmp;
struct attack *mattk;
boolean ufound;
{
    boolean physical_damage = TRUE, kill_agr = TRUE;

    if (has_cancelled(mtmp))
        return 0;

    if (!ufound) {
        pline("%s explodes at a spot in %s!",
              canseemon(mtmp) ? Monnam(mtmp) : "It",
              levl[mtmp->mux][mtmp->muy].typ == WATER ? "empty water"
                                                      : "thin air");
    } else {
		int tmp = 0;
		if(mattk->damn > 0 && mattk->damd > 0)
			tmp += d((int)mattk->damn, (int)mattk->damd);
		tmp += (int)mattk->damp;
		boolean not_affected = 0;

        hitmsg(mtmp, mattk, -1);

        switch (mattk->adtyp) {
        case AD_COLD:
            physical_damage = FALSE;
            not_affected |= Cold_resistance;
            goto common;
        case AD_FIRE:
            physical_damage = FALSE;
            not_affected |= Fire_resistance;
            goto common;
        case AD_ELEC:
            physical_damage = FALSE;
            not_affected |= Shock_resistance;
            goto common;
        case AD_PHYS:
            /* there aren't any exploding creatures with AT_EXPL attack
               for AD_PHYS damage but there might be someday; without this,
               static analysis complains that 'physical_damage' is always
               False when tested below; it's right, but having that in
               place means one less thing to update if AD_PHYS gets added */
 common:

            if (!not_affected) {
                if (ACURR(A_DEX) > rnd(20)) {
                    You("duck some of the blast.");
                    tmp = (tmp + 1) / 2;
                } else {
                    if (flags.verbose)
                        You("get blasted!");
                }
                if (mattk->adtyp == AD_FIRE)
                    burn_away_slime();
                if (physical_damage)
                    tmp = Maybe_Half_Phys(tmp);
				if (Invulnerable)
					tmp = 0;
				mdamageu(mtmp, tmp, TRUE);
            }
            break;

        case AD_BLND:
            not_affected = resists_blnd(&youmonst) || Flash_resistance;
            if (!not_affected) {
                /* sometimes you're affected even if it's invisible */
                if (mon_visible(mtmp) || (rnd(tmp /= 2) > u.ulevel)) {
                    You("are blinded by a blast of light!");
                    make_blinded((long) tmp, FALSE);
                    if (!Blind)
                        Your1(vision_clears);
                } else if (flags.verbose)
                    You("get the impression it was not terribly bright.");
            }
            break;

        case AD_HALU:
            not_affected = Blind || Halluc_resistance || (u.umonnum == PM_BLACK_LIGHT
                                      || u.umonnum == PM_VIOLET_FUNGUS
                                      || dmgtype(youmonst.data, AD_STUN));
            if (!not_affected) {
                boolean chg;
                if (!Hallucination)
                    You("are caught in a blast of kaleidoscopic light!");
                /* avoid hallucinating the black light as it dies */
                mondead(mtmp);    /* remove it from map now */
                kill_agr = FALSE; /* already killed (maybe lifesaved) */
                chg =
                    make_hallucinated(HHallucination + (long) tmp, FALSE, 0L);
                You("%s.", chg ? "are freaked out" : "seem unaffected");
            }
            break;

        default:
            break;
        }
        if (not_affected) {
            You("seem unaffected by it.");
            ugolemeffects((int) mattk->adtyp, tmp);
        }
    }
    if (kill_agr)
        mondead(mtmp);
    wake_nearto(mtmp->mx, mtmp->my, 7 * 7);
    return (!DEADMONSTER(mtmp)) ? 0 : 2;
}

/* monster gazes at you */
int
gazemu(mtmp, mattk)
struct monst *mtmp;
struct attack *mattk;
{
    static const char *const reactions[] = {
        "confused",              /* [0] */
        "stunned",               /* [1] */
        "puzzled",   "dazzled",  /* [2,3] */
        "irritated", "inflamed", /* [4,5] */
        "tired",                 /* [6] */
        "dulled",                /* [7] */
    };
    int react = -1;
    boolean cancelled = has_cancelled(mtmp), already = FALSE;

    /* assumes that hero has to see monster's gaze in order to be
       affected, rather than monster just having to look at hero;
       when hallucinating, hero's brain doesn't register what
       it's seeing correctly so the gaze is usually ineffective
       [this could be taken a lot farther and select a gaze effect
       appropriate to what's currently being displayed, giving
       ordinary monsters a gaze attack when hero thinks he or she
       is facing a gazing creature, but let's not go that far...] */
    if (Hallucination && rn2(4))
        cancelled = TRUE;

    switch (mattk->adtyp) {
    case AD_STON:
        if (cancelled || is_blinded(mtmp)) {
            if (!canseemon(mtmp))
                break; /* silently */
            pline("%s %s.", Monnam(mtmp),
                  (mtmp->data == &mons[PM_MEDUSA] && has_cancelled(mtmp))
                      ? "doesn't look all that ugly"
                      : "gazes ineffectually");
            break;
        }
        if (Reflecting && couldsee(mtmp->mx, mtmp->my)
            && mtmp->data == &mons[PM_MEDUSA]) {
            /* hero has line of sight to Medusa and she's not blind */
            boolean useeit = canseemon(mtmp);

            if (useeit)
                (void) ureflects("%s gaze is reflected by your %s.",
                                 s_suffix(Monnam(mtmp)));
            if (mon_reflects(
                    mtmp, !useeit ? (char *) 0
                                  : "The gaze is reflected away by %s %s!"))
                break;
            if (!m_canseeu(mtmp)) { /* probably you're invisible */
                if (useeit)
                    pline(
                      "%s doesn't seem to notice that %s gaze was reflected.",
                          Monnam(mtmp), mhis(mtmp));
                break;
            }
            if (useeit)
                pline("%s is turned to stone!", Monnam(mtmp));
            stoned = TRUE;
            killed(mtmp);

            if (!DEADMONSTER(mtmp))
                break;
            return 2;
        }
        if (canseemon(mtmp) && couldsee(mtmp->mx, mtmp->my)
            && !Stone_resistance) {
            You("meet %s gaze.", s_suffix(mon_nam(mtmp)));
            stop_occupation();
            if (poly_when_stoned(youmonst.data) && polymon(PM_STONE_GOLEM))
                break;
            You("turn to stone...");
            killer.format = KILLED_BY;
            Strcpy(killer.name, mtmp->data->mname);
            done(STONING);
        }
        break;
    case AD_CONF:
        if (canseemon(mtmp) && couldsee(mtmp->mx, mtmp->my) && !is_blinded(mtmp)
            && !mtmp->mspec_used && rn2(5)) {
            if (cancelled) {
                react = 0; /* "confused" */
                already = (is_confused(mtmp) != 0);
            } else {
                int conf = d(3, 4);

                mtmp->mspec_used = mtmp->mspec_used + (conf + rn2(6));
                if (!Confusion)
                    pline("%s gaze confuses you!", s_suffix(Monnam(mtmp)));
                else
                    You("are getting more and more confused.");
                make_confused(itimeout_incr(HConfusion, conf), FALSE);
                stop_occupation();
            }
        }
        break;
    case AD_STUN:
        if (canseemon(mtmp) && couldsee(mtmp->mx, mtmp->my) && !is_blinded(mtmp)
            && !mtmp->mspec_used && rn2(5)) {
            if (cancelled) {
                react = 1; /* "stunned" */
                already = (is_stunned(mtmp) != 0);
            } else {
                int stun = d(2, 6);

                mtmp->mspec_used = mtmp->mspec_used + (stun + rn2(6));
                pline("%s stares piercingly at you!", Monnam(mtmp));
                make_stunned((HStun & TIMEOUT) + (long) stun, TRUE);
                stop_occupation();
            }
        }
        break;
    case AD_BLND:
        if (canseemon(mtmp) && !resists_blnd(&youmonst) && !Flash_resistance
            && distu(mtmp->mx, mtmp->my) <= BOLT_LIM * BOLT_LIM) {
            if (cancelled) {
                react = rn1(2, 2); /* "puzzled" || "dazzled" */
                already = (mtmp->mprops[BLINDED] != 0);
                /* Archons gaze every round; we don't want cancelled ones
                   giving the "seems puzzled/dazzled" message that often */
                if (has_cancelled(mtmp) && mtmp->data == &mons[PM_ARCHON] && rn2(5))
                    react = -1;
            } else {
				int blnd = 0;
				if(mattk->damn > 0 && mattk->damd > 0)
					blnd += d((int)mattk->damn, (int)mattk->damd);
				blnd += mattk->damp;

                You("are blinded by %s radiance!", s_suffix(mon_nam(mtmp)));
                make_blinded((long) blnd, FALSE);
                stop_occupation();
                /* not blind at this point implies you're wearing
                   the Eyes of the Overworld; make them block this
                   particular stun attack too */
                if (!Blind) {
					pline("But you can still see!");
                    //Your1(vision_clears);
                } else {
                    long oldstun = (HStun & TIMEOUT), newstun = (long) rnd(3);

                    /* we don't want to increment stun duration every time
                       or sighted hero will become incapacitated */
                    make_stunned(max(oldstun, newstun), TRUE);
                }
            }
        }
        break;
    case AD_FIRE:
        if (canseemon(mtmp) && couldsee(mtmp->mx, mtmp->my) && !is_blinded(mtmp)
            && !mtmp->mspec_used && rn2(5)) {
            if (cancelled) {
                react = rn1(2, 4); /* "irritated" || "inflamed" */
            } else {
                int dmg = d(2, 6), lev = (int) mtmp->m_lev;

                pline("%s attacks you with a fiery gaze!", Monnam(mtmp));
                stop_occupation();
                if (Fire_resistance || Invulnerable) {
                    pline_The("fire doesn't feel hot!");
                    dmg = 0;
                }
                burn_away_slime();
                if (lev > rn2(20))
                    destroy_item(SCROLL_CLASS, AD_FIRE);
                if (lev > rn2(20))
                    destroy_item(POTION_CLASS, AD_FIRE);
                if (lev > rn2(25))
                    destroy_item(SPBOOK_CLASS, AD_FIRE);
                if (dmg)
                    mdamageu(mtmp, dmg, TRUE);
            }
        }
        break;
#ifdef PM_BEHOLDER /* work in progress */
    case AD_SLEE:
        if (canseemon(mtmp) && couldsee(mtmp->mx, mtmp->my) && !is_blinded(mtmp)
            && multi >= 0 && !rn2(5) && !Sleep_resistance) {
            if (cancelled) {
                react = 6;                      /* "tired" */
                already = (mtmp->mfrozen != 0); /* can't happen... */
            } else {
                fall_asleep(-rnd(10), TRUE);
                pline("%s gaze makes you very sleepy...",
                      s_suffix(Monnam(mtmp)));
            }
        }
        break;
    case AD_SLOW:
        if (canseemon(mtmp) && couldsee(mtmp->mx, mtmp->my) && !is_blinded(mtmp)
            && (HFast & (INTRINSIC | TIMEOUT)) && !defends(AD_SLOW, uwep)
            && !rn2(4)) {
            if (cancelled) {
                react = 7; /* "dulled" */
                already = (mtmp->mspeed == MSLOW);
            } else {
                u_slow_down();
                stop_occupation();
            }
        }
        break;
#endif /* BEHOLDER */
    default:
        impossible("Gaze attack %d?", mattk->adtyp);
        break;
    }
    if (react >= 0) {
        if (Hallucination && rn2(3))
            react = rn2(SIZE(reactions));
        /* cancelled/hallucinatory feedback; monster might look "confused",
           "stunned",&c but we don't actually set corresponding attribute */
        pline("%s looks %s%s.", Monnam(mtmp),
              !rn2(3) ? "" : already ? "quite "
                                     : (!rn2(2) ? "a bit " : "somewhat "),
              reactions[react]);
    }
    return 0;
}

/* mtmp hits you for n points damage */
void
mdamageu(mtmp, n, verbose)
struct monst *mtmp;
int n;
boolean verbose;
{
    context.botl = 1;
	if (verbose && n > 0)
		You("sustain %d damage!", n);

	if (Upolyd) {
        u.mh -= n;
        if (u.mh < 1)
            rehumanize();
    } else {
        u.uhp -= n;
        if (u.uhp < 1)
            done_in_by(mtmp, DIED);
    }

}

/* returns 0 if seduction impossible,
 *         1 if fine,
 *         2 if wrong gender for nymph
 */
int
could_seduce(magr, mdef, mattk)
struct monst *magr, *mdef;
struct attack *mattk; /* non-Null: current attack; Null: general capability */
{
    struct permonst *pagr;
    boolean agrinvis, defperc;
    xchar genagr, gendef;
    int adtyp;

    if (is_animal(magr->data))
        return 0;
    if (magr == &youmonst) {
        pagr = youmonst.data;
        agrinvis = (Invis != 0);
        genagr = poly_gender();
    } else {
        pagr = magr->data;
        agrinvis = is_invisible(magr);
        genagr = gender(magr);
    }
    if (mdef == &youmonst) {
        defperc = (See_invisible != 0);
        gendef = poly_gender();
    } else {
        defperc = has_see_invisible(mdef);
        gendef = gender(mdef);
    }

    adtyp = mattk ? mattk->adtyp
            : dmgtype(pagr, AD_SSEX) ? AD_SSEX
              : dmgtype(pagr, AD_SEDU) ? AD_SEDU
                : AD_PHYS;
    if (adtyp == AD_SSEX && !SYSOPT_SEDUCE)
        adtyp = AD_SEDU;

    if (agrinvis && !defperc && adtyp == AD_SEDU)
        return 0;

    if ((pagr->mlet != S_NYMPH
         && pagr != &mons[PM_INCUBUS] && pagr != &mons[PM_SUCCUBUS])
        || (adtyp != AD_SEDU && adtyp != AD_SSEX))
        return 0;

    return (genagr == 1 - gendef) ? 1 : (pagr->mlet == S_NYMPH) ? 2 : 0;
}

/* returns 1 if monster teleported (or hero leaves monster's vicinity) */
int
doseduce(mon)
struct monst *mon;
{
    struct obj *ring, *nring;
    boolean fem = (mon->data == &mons[PM_SUCCUBUS]); /* otherwise incubus */
    boolean seewho, naked; /* True iff no armor */
    int attr_tot, tried_gloves = 0;
    char qbuf[QBUFSZ], Who[QBUFSZ];

    if (has_cancelled(mon) || mon->mspec_used) {
        pline("%s acts as though %s has got a %sheadache.", Monnam(mon),
              mhe(mon), has_cancelled(mon) ? "severe " : "");
        return 0;
    }
    if (unconscious()) {
        pline("%s seems dismayed at your lack of response.", Monnam(mon));
        return 0;
    }
    seewho = canseemon(mon);
    if (!seewho)
        pline("Someone caresses you...");
    else
        You_feel("very attracted to %s.", mon_nam(mon));
    /* cache the seducer's name in a local buffer */
    Strcpy(Who, (!seewho ? (fem ? "She" : "He") : Monnam(mon)));

    /* if in the process of putting armor on or taking armor off,
       interrupt that activity now */
    (void) stop_donning((struct obj *) 0);
    /* don't try to take off gloves if cursed weapon blocks them */
    if (welded(uwep, &youmonst))
        tried_gloves = 1;

    for (ring = invent; ring; ring = nring) {
        nring = ring->nobj;
        if (ring->otyp != RIN_ADORNMENT)
            continue;
        if (fem) {
            if (ring->owornmask && uarmg) {
                /* don't take off worn ring if gloves are in the way */
                if (!tried_gloves++)
                    mayberem(mon, Who, uarmg, "gloves");
                if (uarmg)
                    continue; /* next ring might not be worn */
            }
            /* confirmation prompt when charisma is high bypassed if deaf */
            if (!Deaf && rn2(20) < ACURR(A_CHA)) {
                (void) safe_qbuf(qbuf, "\"That ",
                                 " looks pretty.  May I have it?\"", ring,
                                 xname, simpleonames, "ring");
                makeknown(RIN_ADORNMENT);
                if (yn(qbuf) == 'n')
                    continue;
            } else
                pline("%s decides she'd like %s, and takes it.",
                      Who, yname(ring));
            makeknown(RIN_ADORNMENT);
            /* might be in left or right ring slot or weapon/alt-wep/quiver */
            if (ring->owornmask)
                remove_worn_item(ring, FALSE);
            freeinv(ring);
            (void) mpickobj(mon, ring);
        } else {
            if (uleft && uright && uleft->otyp == RIN_ADORNMENT
                && uright->otyp == RIN_ADORNMENT)
                break;
            if (ring == uleft || ring == uright)
                continue;
            if (uarmg) {
                /* don't put on ring if gloves are in the way */
                if (!tried_gloves++)
                    mayberem(mon, Who, uarmg, "gloves");
                if (uarmg)
                    break; /* no point trying further rings */
            }
            /* confirmation prompt when charisma is high bypassed if deaf */
            if (!Deaf && rn2(20) < ACURR(A_CHA)) {
                (void) safe_qbuf(qbuf, "\"That ",
                                 " looks pretty.  Would you wear it for me?\"",
                                 ring, xname, simpleonames, "ring");
                makeknown(RIN_ADORNMENT);
                if (yn(qbuf) == 'n')
                    continue;
            } else {
                pline("%s decides you'd look prettier wearing %s,",
                      Who, yname(ring));
                pline("and puts it on your finger.");
            }
            makeknown(RIN_ADORNMENT);
            if (!uright) {
                pline("%s puts %s on your right %s.",
                      Who, the(xname(ring)), body_part(HAND));
                setworn(ring, RIGHT_RING);
            } else if (!uleft) {
                pline("%s puts %s on your left %s.",
                      Who, the(xname(ring)), body_part(HAND));
                setworn(ring, LEFT_RING);
            } else if (uright && uright->otyp != RIN_ADORNMENT) {
                /* note: the "replaces" message might be inaccurate if
                   hero's location changes and the process gets interrupted,
                   but trying to figure that out in advance in order to use
                   alternate wording is not worth the effort */
                pline("%s replaces %s with %s.",
                      Who, yname(uright), yname(ring));
                Ring_gone(uright);
                /* ring removal might cause loss of levitation which could
                   drop hero onto trap that transports hero somewhere else */
                if (u.utotype || distu(mon->mx, mon->my) > 2)
                    return 1;
                setworn(ring, RIGHT_RING);
            } else if (uleft && uleft->otyp != RIN_ADORNMENT) {
                /* see "replaces" note above */
                pline("%s replaces %s with %s.",
                      Who, yname(uleft), yname(ring));
                Ring_gone(uleft);
                if (u.utotype || distu(mon->mx, mon->my) > 2)
                    return 1;
                setworn(ring, LEFT_RING);
            } else
                impossible("ring replacement");
            Ring_on(ring);
            prinv((char *) 0, ring, 0L);
        }
    }

    naked = (!uarmc && !uarmf && !uarmg && !uarms && !uarmh && !uarmu && !uarmo && !uarmb);
    pline("%s %s%s.", Who,
          Deaf ? "seems to murmur into your ear"
               : naked ? "murmurs sweet nothings into your ear"
                       : "murmurs in your ear",
          naked ? "" : ", while helping you undress");
	if (uarmc)
		mayberem(mon, Who, uarmc, cloak_simple_name(uarmc));

    if (!uarmc && uarmo)
        mayberem(mon, Who, uarmo, robe_simple_name(uarmo));
	if (!uarmc && !uarmo && uarm)
		mayberem(mon, Who, uarm, suit_simple_name(uarm));
	if (uarmf)
		mayberem(mon, Who, uarmf, "boots");
    if (!tried_gloves && uarmg)
        mayberem(mon, Who, uarmg, "gloves");
	if (uarmb)
		mayberem(mon, Who, uarmb, "bracers");
	if (uarms)
		mayberem(mon, Who, uarms, "shield");
	if (uarmh)
		mayberem(mon, Who, uarmh, helm_simple_name(uarmh));
	if (!uarmc && !uarmo && !uarm && uarmu)
        mayberem(mon, Who, uarmu, "shirt");

    /* removing armor (levitation boots, or levitation ring to make
       room for adornment ring with incubus case) might result in the
       hero falling through a trap door or landing on a teleport trap
       and changing location, so hero might not be adjacent to seducer
       any more (mayberem() has its own adjacency test so we don't need
       to check after each potential removal) */
    if (u.utotype || distu(mon->mx, mon->my) > 2)
        return 1;

    if (uarm || uarmc) {
        if (!Deaf)
            verbalize("You're such a %s; I wish...",
                      flags.female ? "sweet lady" : "nice guy");
        else if (seewho)
            pline("%s appears to sigh.", Monnam(mon));
        /* else no regret message if can't see or hear seducer */

        if (!tele_restrict(mon))
            (void) rloc(mon, TRUE);
        return 1;
    }
    if (u.ualign.type == A_CHAOTIC)
        adjalign(1);

    /* by this point you have discovered mon's identity, blind or not... */
    pline("Time stands still while you and %s lie in each other's arms...",
          noit_mon_nam(mon));
    /* 3.6.1: a combined total for charisma plus intelligence of 35-1
       used to guarantee successful outcome; now total maxes out at 32
       as far as deciding what will happen; chance for bad outcome when
       Cha+Int is 32 or more is 2/35, a bit over 5.7% */
    attr_tot = ACURR(A_CHA) + ACURR(A_INT);
    if (rn2(35) > min(attr_tot, 32)) {
        /* Don't bother with mspec_used here... it didn't get tired! */
        pline("%s seems to have enjoyed it more than you...",
              noit_Monnam(mon));
        switch (rn2(5)) {
        case 0:
            You_feel("drained of energy.");
            u.uen = 0;
            u.ubaseenmax -= rnd(Half_physical_damage ? 5 : 10);
			updatemaxen();
            exercise(A_CON, FALSE);
            if (u.uenmax < 0)
                u.uenmax = 0;
            break;
        case 1:
            You("are down in the dumps.");
            (void) adjattrib(A_CON, -1, TRUE);
            exercise(A_CON, FALSE);
            context.botl = 1;
            break;
        case 2:
            Your("senses are dulled.");
            (void) adjattrib(A_WIS, -1, TRUE);
            exercise(A_WIS, FALSE);
            context.botl = 1;
            break;
        case 3:
            if (!resists_drli(&youmonst)) {
                You_feel("out of shape.");
                losexp("overexertion");
            } else {
                You("have a curious feeling...");
            }
            exercise(A_CON, FALSE);
            exercise(A_DEX, FALSE);
            exercise(A_WIS, FALSE);
            break;
        case 4: {
            int tmp;

            You_feel("exhausted.");
            exercise(A_STR, FALSE);
            tmp = rn1(10, 6);
            losehp(Maybe_Half_Phys(tmp), "exhaustion", KILLED_BY);
            break;
        } /* case 4 */
        } /* switch */
    } else {
        mon->mspec_used = rnd(100); /* monster is worn out */
        You("seem to have enjoyed it more than %s...", noit_mon_nam(mon));
        switch (rn2(5)) {
        case 0:
            You_feel("raised to your full potential.");
            exercise(A_CON, TRUE);
            u.uen = (u.ubaseenmax += rnd(5));
			updatemaxen();
            break;
        case 1:
            You_feel("good enough to do it again.");
            (void) adjattrib(A_CON, 1, TRUE);
            exercise(A_CON, TRUE);
            context.botl = 1;
            break;
        case 2:
            You("will always remember %s...", noit_mon_nam(mon));
            (void) adjattrib(A_WIS, 1, TRUE);
            exercise(A_WIS, TRUE);
            context.botl = 1;
            break;
        case 3:
            pline("That was a very educational experience.");
            pluslvl(FALSE);
            exercise(A_WIS, TRUE);
            break;
        case 4:
            You_feel("restored to health!");
            u.uhp = u.uhpmax;
            if (Upolyd)
                u.mh = u.mhmax;
            exercise(A_STR, TRUE);
            context.botl = 1;
            break;
        }
    }

    if (is_tame(mon)) { /* don't charge */
        ;
    } else if (rn2(20) < ACURR(A_CHA)) {
        pline("%s demands that you pay %s, but you refuse...",
              noit_Monnam(mon), noit_mhim(mon));
    } else if (u.umonnum == PM_LEPRECHAUN) {
        pline("%s tries to take your money, but fails...", noit_Monnam(mon));
    } else {
        long cost;
        long umoney = money_cnt(invent);

        if (umoney > (long) LARGEST_INT - 10L)
            cost = (long) rnd(LARGEST_INT) + 500L;
        else
            cost = (long) rnd((int) umoney + 10) + 500L;
        if (is_peaceful(mon))
		{
            cost /= 5L;
            if (!cost)
                cost = 1L;
        }
        if (cost > umoney)
            cost = umoney;
        if (!cost) {
            verbalize("It's on the house!");
        } else {
            pline("%s takes %ld %s for services rendered!", noit_Monnam(mon),
                  cost, currency(cost));
            money2mon(mon, cost);
            context.botl = 1;
        }
    }
    if (!rn2(25))
        increase_mon_temporary_property(mon, CANCELLED, 10 + rnd(40)); /* monster is worn out */
    if (!tele_restrict(mon))
        (void) rloc(mon, TRUE);
    return 1;
}

STATIC_OVL void
mayberem(mon, seducer, obj, str)
struct monst *mon;
const char *seducer; /* only used for alternate message */
struct obj *obj;
const char *str;
{
    char qbuf[QBUFSZ];

    if (!obj || !obj->owornmask)
        return;
    /* removal of a previous item might have sent the hero elsewhere
       (loss of levitation that leads to landing on a transport trap) */
    if (u.utotype || distu(mon->mx, mon->my) > 2)
        return;

    /* being deaf overrides confirmation prompt for high charisma */
    if (Deaf) {
        pline("%s takes off your %s.", seducer, str);
    } else if (rn2(20) < ACURR(A_CHA)) {
        Sprintf(qbuf, "\"Shall I remove your %s, %s?\"", str,
                (!rn2(2) ? "lover" : !rn2(2) ? "dear" : "sweetheart"));
        if (yn(qbuf) == 'n')
            return;
    } else {
        char hairbuf[BUFSZ];

        Sprintf(hairbuf, "let me run my fingers through your %s",
                body_part(HAIR));
        verbalize("Take off your %s; %s.", str,
                  (obj == uarm)
                     ? "let's get a little closer"
                     : (obj == uarmc || obj == uarms || obj == uarmo || obj == uarmb)
                        ? "it's in the way"
                        : (obj == uarmf)
                           ? "let me rub your feet"
                           : (obj == uarmg)
                              ? "they're too clumsy"
                              : (obj == uarmu)
                                 ? "let me massage you"
                                 /* obj == uarmh */
                                 : hairbuf);
    }
    remove_worn_item(obj, TRUE);
}

/* FIXME:
 *  sequencing issue:  a monster's attack might cause poly'd hero
 *  to revert to normal form.  The messages for passive counterattack
 *  would look better if they came before reverting form, but we need
 *  to know whether hero reverted in order to decide whether passive
 *  damage applies.
 */
STATIC_OVL int
passiveum(olduasmon, mtmp, mattk)
struct permonst *olduasmon;
struct monst *mtmp;
struct attack *mattk;
{
    int i, tmp;
    struct attack *oldu_mattk = 0;

    /*
     * mattk      == mtmp's attack that hit you;
     * oldu_mattk == your passive counterattack (even if mtmp's attack
     *               has already caused you to revert to normal form).
     */
    for (i = 0; !oldu_mattk; i++) {
        if (i >= NATTK)
            return 1;
        if (olduasmon->mattk[i].aatyp == AT_NONE
            || olduasmon->mattk[i].aatyp == AT_BOOM)
            oldu_mattk = &olduasmon->mattk[i];
    }
    if (oldu_mattk->damn > 0 && oldu_mattk->damd > 0)
        tmp = d((int) oldu_mattk->damn, (int) oldu_mattk->damd);
    else if (oldu_mattk->damd > 0)
        tmp = d((int) olduasmon->mlevel + 1, (int) oldu_mattk->damd);
    else
        tmp = 0;
	tmp += oldu_mattk->damp;

    /* These affect the enemy even if you were "killed" (rehumanized) */
    switch (oldu_mattk->adtyp) {
    case AD_ACID:
        if (!rn2(2)) {
            pline("%s is splashed by %s%s!", Monnam(mtmp),
                  /* temporary? hack for sequencing issue:  "your acid"
                     looks strange coming immediately after player has
                     been told that hero has reverted to normal form */
                  !Upolyd ? "" : "your ", hliquid("acid"));
            if (resists_acid(mtmp)) {
                pline("%s is not affected.", Monnam(mtmp));
                tmp = 0;
            }
        } else
            tmp = 0;
        if (!rn2(30))
            erode_armor(mtmp, ERODE_CORRODE);
        if (!rn2(6))
            acid_damage(MON_WEP(mtmp));
        goto assess_dmg;
    case AD_STON: /* cockatrice */
    {
        long protector = attk_protection((int) mattk->aatyp),
             wornitems = mtmp->worn_item_flags;

        /* wielded weapon gives same protection as gloves here */
        if (MON_WEP(mtmp) != 0)
            wornitems |= W_ARMG;

        if (!resists_ston(mtmp)
            && (protector == 0L
                || (protector != ~0L
                    && (wornitems & protector) != protector))) {
            if (poly_when_stoned(mtmp->data)) {
                mon_to_stone(mtmp);
                return 1;
            }
            pline("%s turns to stone!", Monnam(mtmp));
            stoned = 1;
            xkilled(mtmp, XKILL_NOMSG);
            if (!DEADMONSTER(mtmp))
                return 1;
            return 2;
        }
        return 1;
    }
    case AD_ENCH: /* KMH -- remove enchantment (disenchanter) */
        if (mon_currwep) {
            /* by_you==True: passive counterattack to hero's action
               is hero's fault */
            (void) drain_item(mon_currwep, TRUE);
            /* No message */
        }
        return 1;
    default:
        break;
    }
    if (!Upolyd)
        return 1;

    /* These affect the enemy only if you are still a monster */
    if (rn2(3))
        switch (oldu_mattk->adtyp) {
        case AD_PHYS:
            if (oldu_mattk->aatyp == AT_BOOM) {
                You("explode!");
                /* KMH, balance patch -- this is okay with unchanging */
                rehumanize();
                goto assess_dmg;
            }
            break;
        case AD_PLYS: /* Floating eye */
            if (tmp > 127)
                tmp = 127;
            if (u.umonnum == PM_FLOATING_EYE) {
                if (!rn2(20))
                    tmp = 24;
                if (!is_blinded(mtmp) && haseyes(mtmp->data) && rn2(3)
                    && (has_see_invisible(mtmp) || !Invis)) {
                    if (Blind)
                        pline("As a blind %s, you cannot defend yourself.",
                              youmonst.data->mname);
                    else {
                        if (mon_reflects(mtmp,
                                         "Your gaze is reflected by %s %s."))
                            return 1;
                        pline("%s is frozen by your gaze!", Monnam(mtmp));
                        paralyze_monst(mtmp, tmp, FALSE);
                        return 3;
                    }
                }
            } else { /* gelatinous cube */
                pline("%s is frozen by you.", Monnam(mtmp));
                paralyze_monst(mtmp, tmp, FALSE);
                return 3;
            }
            return 1;
        case AD_COLD: /* Brown mold or blue jelly */
            if (resists_cold(mtmp)) {
                shieldeff(mtmp->mx, mtmp->my);
                pline("%s is mildly chilly.", Monnam(mtmp));
                golemeffects(mtmp, AD_COLD, tmp);
                tmp = 0;
                break;
            }
            pline("%s is suddenly very cold!", Monnam(mtmp));
            u.mh += tmp / 2;
            if (u.mh - u.mhmax > 0)
                u.basemhmax += u.mh - u.mhmax;
			updatemaxhp();
			if (u.mhmax > ((youmonst.data->mlevel + 1) * 8))
                (void) split_mon(&youmonst, mtmp);
            break;
        case AD_STUN: /* Yellow mold */
            if (!is_stunned(mtmp)) {
				nonadditive_increase_mon_temporary_property(mtmp, STUNNED, 5 + rnd(5));
				pline("%s %s.", Monnam(mtmp),
                      makeplural(stagger(mtmp->data, "stagger")));
            }
            tmp = 0;
            break;
        case AD_FIRE: /* Red mold */
            if (resists_fire(mtmp)) 
			{
                shieldeff(mtmp->mx, mtmp->my);
				if (alternative_passive_defense_text(youmonst.data))
					pline("%s is engulfed in your flames, but they do not burn %s.", Monnam(mtmp), mon_nam(mtmp));
				else
					pline("%s is mildly warm.", Monnam(mtmp));
                golemeffects(mtmp, AD_FIRE, tmp);
                tmp = 0;
                break;
            }
			if (alternative_passive_defense_text(youmonst.data))
				pline("%s is engulfed in your flames!", Monnam(mtmp));
			else
				pline("%s is suddenly very hot!", Monnam(mtmp));
            break;
        case AD_ELEC:
            if (resists_elec(mtmp)) {
                shieldeff(mtmp->mx, mtmp->my);
                pline("%s is slightly tingled.", Monnam(mtmp));
                golemeffects(mtmp, AD_ELEC, tmp);
                tmp = 0;
                break;
            }
            pline("%s is jolted with your electricity!", Monnam(mtmp));
            break;
        default:
            tmp = 0;
            break;
        }
    else
        tmp = 0;

assess_dmg:
	if (canseemon(mtmp) && tmp > 0)
		pline("%s sustains %d damage!", Monnam(mtmp), tmp);

    if ((mtmp->mhp -= tmp) <= 0)
	{
        pline("%s dies!", Monnam(mtmp));
        xkilled(mtmp, XKILL_NOMSG);
        if (!DEADMONSTER(mtmp))
            return 1;
        return 2;
    }
    return 1;
}

struct monst *
cloneu()
{
    struct monst *mon;
    int mndx = monsndx(youmonst.data);

    if (u.mh <= 1)
        return (struct monst *) 0;
    if (mvitals[mndx].mvflags & G_EXTINCT)
        return (struct monst *) 0;
    mon = makemon(youmonst.data, u.ux, u.uy, MM_NO_MONSTER_INVENTORY | MM_EDOG);
    if (!mon)
        return NULL;
    mon->mcloned = 1;
    mon = christen_monst(mon, plname);
    initedog(mon, TRUE);
    mon->m_lev = youmonst.data->mlevel;
	//mon might need mbasehpmax stat
    mon->mbasehpmax = u.basemhmax;
	update_mon_maxhp(mon);
	mon->mhp = u.mh / 2;
    u.mh -= mon->mhp;
    context.botl = 1;
    return mon;
}

/*mhitu.c*/
