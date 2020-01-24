/* GnollHack 4.0	minion.c	$NHDT-Date: 1544998886 2018/12/16 22:21:26 $  $NHDT-Branch: GnollHack-3.6.2-beta01 $:$NHDT-Revision: 1.40 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2008. */
/* GnollHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "artifact.h"

void
newemin(mtmp)
struct monst *mtmp;
{
    if (!mtmp->mextra)
        mtmp->mextra = newmextra();
    if (!EMIN(mtmp)) {
        EMIN(mtmp) = (struct emin *) alloc(sizeof(struct emin));
        (void) memset((genericptr_t) EMIN(mtmp), 0, sizeof(struct emin));
    }
}

void
free_emin(mtmp)
struct monst *mtmp;
{
    if (mtmp->mextra && EMIN(mtmp)) {
        free((genericptr_t) EMIN(mtmp));
        EMIN(mtmp) = (struct emin *) 0;
    }
    mtmp->isminion = 0;
}

/* count the number of monsters on the level */
int
monster_census(spotted)
boolean spotted; /* seen|sensed vs all */
{
    struct monst *mtmp;
    int count = 0;

    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
        if (DEADMONSTER(mtmp))
            continue;
        if (spotted && !canspotmon(mtmp))
            continue;
        ++count;
    }
    return count;
}

/* mon summons a monster */
int
msummon(mon)
struct monst *mon;
{
    struct permonst *ptr;
    int dtype = NON_PM, cnt = 0, result = 0, census;
    aligntyp atyp;
    struct monst *mtmp;

    if (mon) 
	{
        ptr = mon->data;

        if (item_prevents_summoning(mon->mnum)) 
		{
            if (canseemon(mon))
                pline("%s looks puzzled for a moment.", Monnam(mon));
            return 0;
        }

        atyp = mon->ispriest ? EPRI(mon)->shralign
                             : mon->isminion ? EMIN(mon)->min_align
                                             : (ptr->maligntyp == A_NONE)
                                                   ? A_NONE
                                                   : sgn(ptr->maligntyp);
    } else {
        ptr = &mons[PM_WIZARD_OF_YENDOR];
        atyp = (ptr->maligntyp == A_NONE) ? A_NONE : sgn(ptr->maligntyp);
    }

	if (ptr == &mons[PM_FLIND_LORD]) {
		dtype = (!rn2(80)) ? PM_YEENOGHU : (!rn2(40)) ? monsndx(ptr) : (!rn2(4)) ? ndemon(atyp) : PM_FLIND;
		if (dtype == PM_YEENOGHU && (mvitals[PM_YEENOGHU].mvflags & G_GONE))
			dtype = monsndx(ptr);
		if (dtype == PM_FLIND && (mvitals[PM_FLIND].mvflags & G_GONE))
			dtype = ndemon(atyp);
		cnt = 1;
	} else if (is_dprince(ptr) || (ptr == &mons[PM_WIZARD_OF_YENDOR])) {
        dtype = (!rn2(20)) ? dlord(atyp) : ndemon(atyp); //(!rn2(50)) ? dprince(atyp) : 
        cnt = (!rn2(3) && is_ndemon(&mons[dtype])) ? 2 : 1;
    } else if (is_dlord(ptr)) {
        dtype = (!rn2(80)) ? dprince(atyp) : (!rn2(40)) ? dlord(atyp)
                                                        : ndemon(atyp);
        cnt = (!rn2(4) && is_ndemon(&mons[dtype])) ? 2 : 1;
    } else if (is_ndemon(ptr)) {
        dtype = (!rn2(80)) ? dlord(atyp) : (!rn2(6)) ? ndemon(atyp)
                                                     : monsndx(ptr);
        cnt = 1;
    } else if (is_lminion(mon)) {
        dtype = (is_lord(ptr) && !rn2(40))
                    ? llord()
                    : (is_lord(ptr) || !rn2(6)) ? lminion() : monsndx(ptr);
        cnt = (!rn2(4) && !is_lord(&mons[dtype])) ? 2 : 1;
    } else if (ptr == &mons[PM_ANGEL]) {
        /* non-lawful angels can also summon */
        if (!rn2(6)) {
            switch (atyp) { /* see summon_minion */
            case A_NEUTRAL:
                dtype = PM_AIR_ELEMENTAL + rn2(4);
                break;
            case A_CHAOTIC:
            case A_NONE:
                dtype = ndemon(atyp);
                break;
            }
        } else {
            dtype = PM_ANGEL;
        }
        cnt = (!rn2(4) && !is_lord(&mons[dtype])) ? 2 : 1;
    }

    if (dtype == NON_PM)
        return 0;

    /* sanity checks */
    if (cnt > 1 && (mons[dtype].geno & G_UNIQ))
        cnt = 1;
    /*
     * If this daemon is unique and being re-summoned (the only way we
     * could get this far with an extinct dtype), try another.
     */
    if (mvitals[dtype].mvflags & G_GONE) {
        dtype = ndemon(atyp);
        if (dtype == NON_PM)
            return 0;
    }

    /* some candidates can generate a group of monsters, so simple
       count of non-null makemon() result is not sufficient */
    census = monster_census(FALSE);

    while (cnt > 0) {
        mtmp = makemon(&mons[dtype], u.ux, u.uy, MM_EMIN);
        if (mtmp) {
            result++;
            /* an angel's alignment should match the summoner */
            if (dtype == PM_ANGEL) {
                mtmp->isminion = 1;
                EMIN(mtmp)->min_align = atyp;
                /* renegade if same alignment but not peaceful
                   or peaceful but different alignment */
                EMIN(mtmp)->renegade =
                    (atyp != u.ualign.type) ^ !is_peaceful(mtmp);
            }
			if(canseemon(mtmp))
			{ 
				if (is_demon(ptr))
					pline("%s appears in a cloud of smoke!", Amonnam(mtmp));
				else
					pline("%s appears!", Amonnam(mtmp));
			}
		}
        cnt--;
    }

    /* how many monsters exist now compared to before? */
    if (result)
        result = monster_census(FALSE) - census;

    return result;
}


/* Yeenoghu summons gnolls */
int
yeenoghu_gnoll_summon()
{
	int dtype = NON_PM, cnt = 0, result = 0, census;
	struct monst* mtmp = (struct monst*) 0;

	cnt = d(1, 3);

	/* some candidates can generate a group of monsters, so simple
	   count of non-null makemon() result is not sufficient */
	census = monster_census(FALSE);

	int canseemonnumber = 0;
	struct monst* mtmp2 = (struct monst *) 0;

	while (cnt > 0) {
		int roll = rn2(15);
		dtype = NON_PM;

		switch (roll)
		{
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
			dtype = PM_GNOLL;
			break;
		case 5:
		case 6:
		case 7:
		case 8:
			dtype = PM_GNOLL_LORD;
			break;
		case 9:
		case 10:
		case 11:
			dtype = PM_GNOLL_KING;
			break;
		case 12:
		case 13:
			dtype = PM_FLIND;
			break;
		case 14:
			dtype = PM_FLIND_LORD;
			break;
		default:
			break;

		}

		if (dtype == NON_PM || mvitals[dtype].mvflags & G_GONE)
		{
			cnt--;
			continue;
		}

		mtmp = makemon(&mons[dtype], u.ux, u.uy, MM_EMIN);
		if (mtmp) {
			result++;
			if (canseemon(mtmp))
			{
				canseemonnumber++;
				mtmp2 = mtmp;
			}
		}

		cnt--;
	}

	char numberword[BUFSZ] = "Some";
	if(canseemonnumber >= 3)
		strcpy(numberword, "Several");

	if (result > 0 && canseemonnumber > 0) {
		if (result == 1 && mtmp)
			pline("%s appears out of nowhere!", Amonnam(mtmp));
		else
		{
			if (canseemonnumber == 1 && mtmp2)
				pline("%s appears out of nowhere!", Amonnam(mtmp2));
			else
				pline("%s gnolls appear out of nowhere!", numberword);
		}
	}

	/* how many monsters exist now compared to before? */
	if (result)
		result = monster_census(FALSE) - census;

	return result;
}

/* Yeenoghu summons ghouls */
int
yeenoghu_ghoul_summon()
{
	int dtype = NON_PM, cnt = 0, result = 0, census;
	struct monst* mtmp = (struct monst*) 0;

	cnt = d(1, 3);

	/* some candidates can generate a group of monsters, so simple
	   count of non-null makemon() result is not sufficient */
	census = monster_census(FALSE);

	int canseemonnumber = 0;
	struct monst* mtmp2 = (struct monst*) 0;

	while (cnt > 0) {
		dtype = PM_GHOUL;

		if (mvitals[dtype].mvflags & G_GONE)
		{
			cnt--;
			break;
		}

		mtmp = makemon(&mons[dtype], u.ux, u.uy, MM_EMIN);
		if (mtmp) {
			result++;
			if (canseemon(mtmp))
			{
				canseemonnumber++;
				mtmp2 = mtmp;
			}
		}
		cnt--;
	}

	char numberword[BUFSZ] = "Some";
	if (canseemonnumber >= 3)
		strcpy(numberword, "Several");

	if (result > 0 && canseemonnumber > 0) {
		if (result == 1 && mtmp)
			pline("%s crawls out of nowhere!", Amonnam(mtmp));
		else
		{
			if (canseemonnumber == 1 && mtmp2)
				pline("%s  crawls out of nowhere!", Amonnam(mtmp2));
			else
				pline("%s ghouls crawl out of nowhere!", numberword);
		}
	}

	/* how many monsters exist now compared to before? */
	if (result)
		result = monster_census(FALSE) - census;

	return result;
}

/* Orcus summons undead */
int
orcus_undead_summon()
{
	int dtype = NON_PM, cnt = 0, result = 0, census;
	struct monst* mtmp = (struct monst*) 0;

	cnt = d(2, 3);

	/* some candidates can generate a group of monsters, so simple
	   count of non-null makemon() result is not sufficient */
	census = monster_census(FALSE);

	int canseemonnumber = 0;
	struct monst* mtmp2 = (struct monst*) 0;

	while (cnt > 0) {
		int roll = rn2(22);
		dtype = NON_PM;

		switch (roll)
		{
		case 0:
		case 1:
			dtype = PM_GNOME_ZOMBIE;
			break;
		case 2:
			dtype = PM_ELF_ZOMBIE;
			break;
		case 3:
			dtype = PM_ORC_ZOMBIE;
			break;
		case 4:
			dtype = PM_HUMAN_ZOMBIE;
			break;
		case 5:
			dtype = PM_GNOME_MUMMY;
			break;
		case 6:
			dtype = PM_ELF_MUMMY;
			break;
		case 7:
			dtype = PM_ORC_MUMMY;
			break;
		case 8:
			dtype = PM_HUMAN_MUMMY;
			break;
		case 9:
			dtype = PM_GIANT_MUMMY;
			break;
		case 10:
			dtype = PM_ETTIN_MUMMY;
			break;
		case 11:
			dtype = PM_VAMPIRE;
			break;
		case 12:
			dtype = PM_VAMPIRE_MAGE;
			break;
		case 13:
			dtype = PM_VAMPIRE_LORD;
			break;
		case 14:
			dtype = PM_WRAITH;
			break;
		case 15:
			dtype = PM_LICH;
			break;
		case 16:
			dtype = PM_BARROW_WIGHT;
			break;
		case 17:
			dtype = PM_GIANT_ZOMBIE;
			break;
		case 18:
			dtype = PM_DWARF_ZOMBIE;
			break;
		case 19:
			dtype = PM_DWARF_MUMMY;
			break;
		case 20:
			dtype = PM_SPECTRE;
			break;
		case 21:
			dtype = PM_KING_WRAITH;
			break;
		default:
			break;
		}

		if (dtype == NON_PM || mvitals[dtype].mvflags & G_GONE)
		{
			cnt--;
			continue;
		}

		mtmp = makemon(&mons[dtype], u.ux, u.uy, MM_EMIN);
		if (mtmp) {
			result++;
			if (canseemon(mtmp))
			{
				canseemonnumber++;
				mtmp2 = mtmp;
			}
		}

		cnt--;
	}

	char numberword[BUFSZ] = "Some";
	if (canseemonnumber >= 3)
		strcpy(numberword, "Several");

	if (result > 0 && canseemonnumber > 0) {
		if (result == 1 && mtmp)
			pline("%s appears out of nowhere!", Amonnam(mtmp));
		else
		{
			if (canseemonnumber == 1 && mtmp2)
				pline("%s appears out of nowhere!", Amonnam(mtmp2));
			else
				pline("%s undead appear out of nowhere!", numberword);
		}
	}

	/* how many monsters exist now compared to before? */
	if (result)
		result = monster_census(FALSE) - census;

	return result;
}




void
summon_minion(alignment, talk)
aligntyp alignment;
boolean talk;
{
    register struct monst *mon;
    int mnum;

    switch ((int) alignment) {
    case A_LAWFUL:
        mnum = lminion();
        break;
    case A_NEUTRAL:
        mnum = PM_AIR_ELEMENTAL + rn2(4);
        break;
    case A_CHAOTIC:
    case A_NONE:
        mnum = ndemon(alignment);
        break;
    default:
        impossible("unaligned player?");
        mnum = ndemon(A_NONE);
        break;
    }
    if (mnum == NON_PM) {
        mon = 0;
    } else if (mnum == PM_ANGEL) {
        mon = makemon(&mons[mnum], u.ux, u.uy, MM_EMIN);
        if (mon) {
            mon->isminion = 1;
            EMIN(mon)->min_align = alignment;
            EMIN(mon)->renegade = FALSE;
        }
    } else if (mnum != PM_SHOPKEEPER && mnum != PM_GUARD
               && mnum != PM_ALIGNED_PRIEST && mnum != PM_HIGH_PRIEST) {
        /* This was mons[mnum].pxlth == 0 but is this restriction
           appropriate or necessary now that the structures are separate? */
        mon = makemon(&mons[mnum], u.ux, u.uy, MM_EMIN);
        if (mon) {
            mon->isminion = 1;
            EMIN(mon)->min_align = alignment;
            EMIN(mon)->renegade = FALSE;
        }
    } else {
        mon = makemon(&mons[mnum], u.ux, u.uy, NO_MM_FLAGS);
    }
    if (mon) {
        if (talk) {
            pline_The("voice of %s booms:", align_gname(alignment));
            verbalize("Thou shalt pay for thine indiscretion!");
            if (!Blind)
                pline("%s appears before you.", Amonnam(mon));
            mon->mstrategy &= ~STRAT_APPEARMSG;
        }
        mon->mpeaceful = FALSE;
        /* don't call set_malign(); player was naughty */
    }
}

#define Athome (Inhell && (mtmp->cham == NON_PM))

/* returns 1 if it won't attack. */
int demon_talk(struct monst *mtmp)
{
    long cash, demand, offer;

	if ((uwep && uwep->oartifact && artifact_has_flag(uwep, AF_ANGERS_DEMONS))
		|| (uarms && uarms->oartifact && artifact_has_flag(uarms, AF_ANGERS_DEMONS))
		) 
	{
        pline("%s looks very angry.", Amonnam(mtmp));
        mtmp->mpeaceful = mtmp->mtame = 0;
		if (!mtmp->mtame)
			mtmp->ispartymember = FALSE;

		set_malign(mtmp);
        newsym(mtmp->mx, mtmp->my);
        return 0;
    }

    if (is_fainted()) {
        reset_faint(); /* if fainted - wake up */
    } else {
        stop_occupation();
        if (multi > 0) {
            nomul(0);
            unmul((char *) 0);
        }
    }

    /* Slight advantage given. */
    if (is_dprince(mtmp->data) && is_invisible(mtmp))
	{
        boolean wasunseen = !canspotmon(mtmp);

		set_mon_temporary_property(mtmp, INVISIBILITY, 0);

		if (wasunseen && canspotmon(mtmp)) {
            pline("%s appears before you.", Amonnam(mtmp));
            mtmp->mstrategy &= ~STRAT_APPEARMSG;
        }
        newsym(mtmp->mx, mtmp->my);
    }
    if (youmonst.data->mlet == S_DEMON) { /* Won't blackmail their own. */
        pline("%s says, \"Good hunting, %s.\"", Amonnam(mtmp),
              flags.female ? "Sister" : "Brother");
        if (!tele_restrict(mtmp))
            (void) rloc(mtmp, TRUE);
        return 1;
	}
	else if (mtmp->data == &mons[PM_YEENOGHU] && maybe_polyd(is_gnoll(youmonst.data), Race_if(PM_GNOLL)))
	{
		if (canspotmon(mtmp))
			pline("%s, the Demon Lord of Gnolls, stands towering before you.", Amonnam(mtmp));
		else
			pline("You hear the thundering voice of %s, the Demon Lord of Gnolls.", Amonnam(mtmp));

		pline("%s speaks.", Amonnam(mtmp));

		if ((u.ualign.type == A_CHAOTIC && (u.ualign.record >= 14 || !(2-rnl(3)))) || (u.ualign.type == A_NEUTRAL && !rnl(3))
			|| ((uwep && uwep->oartifact == ART_HOWLING_FLAIL) || (uarms && uarms->oartifact == ART_HOWLING_FLAIL)))
		{
			verbalize("You have pleased me, my minion. I will grant one wish!");
			/* give a wish and discard the monster (mtmp set to null) */
			mongrantswish(&mtmp);
			//mongrantswish removes the monster (calls mongone)
			pline("The demon lord laughs, then vanishes.");
			return 1;
		}
		if (u.ualign.type != A_LAWFUL)
		{
			verbalize("You have not pleased me! Prove your allegiance to the Abyss, and you shall be rewarded.");
			pline("%s scowls at you, then vanishes.", Amonnam(mtmp));
		}
		else
		{
			verbalize("Your ways displease me. Follow the path of the Abyss, and you shall be rewarded.");
			pline("%s scowls at you menacingly, then vanishes.", Amonnam(mtmp));
		}
	}
	else if (mtmp->data == &mons[PM_DEMOGORGON] || mtmp->data == &mons[PM_YEENOGHU] || mtmp->data == &mons[PM_JUIBLEX] || mtmp->data == &mons[PM_ORCUS])
	{
		//Demon lords and princes get angry for all but devout (and some random less devout) chaotic characters
		if ((u.ualign.type == A_CHAOTIC && (u.ualign.record >= 14 || !rnl(4))))
		{
			pline("%s laughs menacingly, then vanishes.",
				Amonnam(mtmp));
		} else 
		{
			pline("Seeing you, %s gets angry...", Amonnam(mtmp));
			mtmp->mpeaceful = 0;
			set_malign(mtmp);
			return 0;
		}
	}
	else //An Arch-Devil demands payment
	{
		cash = money_cnt(invent);
		demand =
			(cash * (rnd(80) + 20 * Athome))
			/ (100 * (1 + (sgn(u.ualign.type) == sgn(mtmp->data->maligntyp))));

		if (!demand || multi < 0) { /* you have no gold or can't move */
			mtmp->mpeaceful = 0;
			set_malign(mtmp);
			return 0;
		} else {
			/* make sure that the demand is unmeetable if the monster
			   has the Amulet, preventing monster from being satisfied
			   and removed from the game (along with said Amulet...) */
			if (mon_has_amulet(mtmp))
				demand = cash + (long) rn1(1000, 40);

			if (canspotmon(mtmp))
				pline("%s stands towering before you.", Amonnam(mtmp));
			else
				pline("You hear the thundering voice of %s.", Amonnam(mtmp));

			verbalize("For your life, we, %s, demand a gift of %ld %s.", Amonnam(mtmp), demand,
				  currency(demand));

			if ((offer = bribe(mtmp)) >= demand) {
				pline("%s vanishes, laughing about cowardly mortals.",
					  Amonnam(mtmp));
			} else if (offer > 0L
					   && (long) rnd(5 * ACURR(A_CHA)) > (demand - offer)) {
				pline("%s scowls at you menacingly, then vanishes.",
					  Amonnam(mtmp));
			} else {
				pline("%s gets angry...", Amonnam(mtmp));
				mtmp->mpeaceful = 0;
				set_malign(mtmp);
				return 0;
			}
		}
	}
    mongone(mtmp);
    return 1;
}

long
bribe(mtmp)
struct monst *mtmp;
{
    char buf[BUFSZ] = DUMMY;
    long offer;
    long umoney = money_cnt(invent);

    getlin("How much will you offer?", buf);
    if (sscanf(buf, "%ld", &offer) != 1)
        offer = 0L;

    /*Michael Paddon -- fix for negative offer to monster*/
    /*JAR880815 - */
    if (offer < 0L) {
        You("try to shortchange %s, but fumble.", mon_nam(mtmp));
        return 0L;
    } else if (offer == 0L) {
        You("refuse.");
        return 0L;
    } else if (offer >= umoney) {
        You("give %s all your gold.", mon_nam(mtmp));
        offer = umoney;
    } else {
        You("give %s %ld %s.", mon_nam(mtmp), offer, currency(offer));
    }
    (void) money2mon(mtmp, offer);
    context.botl = 1;
    return offer;
}

int
dprince(atyp)
aligntyp atyp;
{
    int tryct, pm;

    for (tryct = !In_endgame(&u.uz) ? 20 : 0; tryct > 0; --tryct) {
        pm = rn1(PM_DEMOGORGON + 1 - PM_ORCUS, PM_ORCUS);
        if (!(mvitals[pm].mvflags & G_GONE)
            && (atyp == A_NONE || sgn(mons[pm].maligntyp) == sgn(atyp)))
            return pm;
    }
    return dlord(atyp); /* approximate */
}

int
dlord(atyp)
aligntyp atyp;
{
    int tryct, pm;

    for (tryct = !In_endgame(&u.uz) ? 20 : 0; tryct > 0; --tryct) {
        pm = rn1(PM_YEENOGHU + 1 - PM_JUIBLEX, PM_JUIBLEX);
        if (!(mvitals[pm].mvflags & G_GONE)
            && (atyp == A_NONE || sgn(mons[pm].maligntyp) == sgn(atyp)))
            return pm;
    }
    return ndemon(atyp); /* approximate */
}

/* create lawful (good) lord */
int
llord()
{
    if (!(mvitals[PM_ARCHON].mvflags & G_GONE))
        return PM_ARCHON;

    return lminion(); /* approximate */
}

int
lminion()
{
    int tryct;
    struct permonst *ptr;

    for (tryct = 0; tryct < 20; tryct++) {
        ptr = mkclass(S_ANGEL, 0);
        if (ptr && !is_lord(ptr))
            return monsndx(ptr);
    }

    return NON_PM;
}

int
ndemon(atyp)
aligntyp atyp; /* A_NONE is used for 'any alignment' */
{
    struct permonst *ptr;

    /*
     * 3.6.2:  [fix #H2204, 22-Dec-2010, eight years later...]
     * pick a correctly aligned demon in one try.  This used to
     * use mkclass() to choose a random demon type and keep trying
     * (up to 20 times) until it got one with the desired alignment.
     * mkclass_aligned() skips wrongly aligned potential candidates.
     * [The only neutral demons are djinni and mail daemon and
     * mkclass() won't pick them, but call it anyway in case either
     * aspect of that changes someday.]
     */
#if 0
    if (atyp == A_NEUTRAL)
        return NON_PM;
#endif
    ptr = mkclass_aligned(S_DEMON, 0, atyp);
    return (ptr && is_ndemon(ptr)) ? monsndx(ptr) : NON_PM;
}

/* guardian angel has been affected by conflict so is abandoning hero */
void
lose_guardian_angel(mon)
struct monst *mon; /* if null, angel hasn't been created yet */
{
    coord mm;
    int i;

    if (mon) {
        if (canspotmon(mon)) {
            if (!Deaf) {
                pline("%s rebukes you, saying:", Monnam(mon));
                verbalize("Since you desire conflict, have some more!");
            } else {
                pline("%s vanishes!", Monnam(mon));
            }
        }
        mongone(mon);
    }
    /* create 2 to 4 hostile angels to replace the lost guardian */
    for (i = rn1(3, 2); i > 0; --i) {
        mm.x = u.ux;
        mm.y = u.uy;
        if (enexto(&mm, mm.x, mm.y, &mons[PM_ANGEL]))
            (void) mk_roamer(&mons[PM_ANGEL], u.ualign.type, mm.x, mm.y,
                             FALSE);
    }
}

/* just entered the Astral Plane; receive tame guardian angel if worthy */
void
gain_guardian_angel(fromspell)
boolean fromspell;
{
    struct monst *mtmp;
    struct obj *otmp;
    coord mm;

    Hear_again(); /* attempt to cure any deafness now (divine
                     message will be heard even if that fails) */
    if (!fromspell && Conflict) {
        pline("A voice booms:");
        verbalize("Thy desire for conflict shall be fulfilled!");
        /* send in some hostile angels instead */
        lose_guardian_angel((struct monst *) 0);
    } else if (fromspell || u.ualign.record > 8) { /* fervent */
        pline("A voice whispers:");
        verbalize("Thou hast been worthy of me!");
        mm.x = u.ux;
        mm.y = u.uy;
        if (enexto(&mm, mm.x, mm.y, &mons[PM_ANGEL])
            && (mtmp = mk_roamer(&mons[PM_ANGEL], u.ualign.type, mm.x, mm.y,
                                 TRUE)) != 0) {
            mtmp->mstrategy &= ~STRAT_APPEARMSG;
            if (!Blind)
                pline("An angel appears near you.");
            else
                You_feel("the presence of a friendly angel near you.");
            /* guardian angel -- the one case mtame doesn't
             * imply an edog structure, so we don't want to
             * call tamedog().
             */
            mtmp->mtame = 10;
			newsym(mtmp->mx, mtmp->my);
            /* make him strong enough vs. endgame foes */
            mtmp->m_lev = rn1(8, 15);
            mtmp->mbasehpmax =
                d((int) mtmp->m_lev, 10) + 30 + rnd(30);
			update_mon_maxhp(mtmp);
			mtmp->mhp = mtmp->mhpmax;
            if ((otmp = select_hwep(mtmp)) == 0) {
                otmp = mksobj(SILVER_SABER, FALSE, FALSE, FALSE);
                if (mpickobj(mtmp, otmp))
                    panic("merged weapon?");
            }
            bless(otmp);
            if (otmp->spe < 4)
                otmp->spe += rnd(4);
            if ((otmp = which_armor(mtmp, W_ARMS)) == 0
                || otmp->otyp != SHIELD_OF_REFLECTION) {
                (void) mongets(mtmp, AMULET_OF_REFLECTION);
                m_dowear(mtmp, TRUE);
            }
        }
    }
}

/*minion.c*/
