/*!\file sched-builtin.h
 * \brief Definitions for built-in scheduling classes.
 * \author Philippe Gerum
 *
 * Copyright (C) 2008 Philippe Gerum <rpm@xenomai.org>.
 *
 * Xenomai is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * Xenomai is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Xenomai; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifndef _XENO_NUCLEUS_SCHED_BUILTIN_H
#define _XENO_NUCLEUS_SCHED_BUILTIN_H

#ifndef _XENO_NUCLEUS_SCHED_H
#error "please don't include nucleus/sched-builtin.h directly"
#endif

/* Priority scale for the RT scheduling class. */
#define XNSCHED_RT_MIN_PRIO	0
#define XNSCHED_RT_MAX_PRIO	257
#define XNSCHED_RT_NR_PRIO	(XNSCHED_RT_MAX_PRIO - XNSCHED_RT_MIN_PRIO + 1)

/* Idle priority level - actually never used for indexing. */
#define XNSCHED_IDLE_PRIO	-1

/*
 * Builtin priorities shared by all core APIs.  Those APIs, namely
 * POSIX, native and RTDM, only use a sub-range of the available
 * priority levels from the RT scheduling class, in order to exhibit a
 * 1:1 mapping with Linux's SCHED_FIFO ascending priority scale
 * [1..99]. Non-core APIs with inverted priority scales (e.g. VxWorks,
 * VRTX), normalize the priority values internally when calling the
 * priority-sensitive services of the nucleus, so that they fit into
 * the RT priority scale.
 */
#define XNSCHED_LOW_PRIO	0
#define XNSCHED_HIGH_PRIO	99
#define XNSCHED_IRQ_PRIO	XNSCHED_RT_MAX_PRIO /* For IRQ servers. */

#if defined(__KERNEL__) || defined(__XENO_SIM__)

#if defined(CONFIG_XENO_OPT_SCALABLE_SCHED) && \
  XNSCHED_RT_NR_PRIO > XNSCHED_MLQ_LEVELS
#error "RT class cannot use multi-level queue (too many priority levels)"
#endif

extern struct xnsched_class xnsched_class_rt;

extern struct xnsched_class xnsched_class_idle;

#define xnsched_class_default xnsched_class_rt

static inline void __xnsched_rt_requeue(struct xnthread *thread)
{
	sched_insertpql(&thread->sched->rt.runnable,
			&thread->rlink, thread->cprio);
}

static inline void __xnsched_rt_enqueue(struct xnthread *thread)
{
	sched_insertpqf(&thread->sched->rt.runnable,
			&thread->rlink, thread->cprio);
}

static inline void __xnsched_rt_dequeue(struct xnthread *thread)
{
	sched_removepq(&thread->sched->rt.runnable, &thread->rlink);
}

static inline struct xnthread *__xnsched_rt_pick(struct xnsched *sched)
{
	struct xnpholder *h = sched_getpq(&sched->rt.runnable);
	return h ? link2thread(h, rlink) : NULL;
}

#ifdef CONFIG_XENO_OPT_PRIOCPL

static inline struct xnthread *__xnsched_rt_push_rpi(struct xnsched *sched,
						     struct xnthread *thread)
{
	sched_insertpqf(&sched->rt.relaxed, &thread->xlink, thread->cprio);
	return link2thread(sched_getheadpq(&sched->rt.relaxed), xlink);
}

static inline void __xnsched_rt_pop_rpi(struct xnthread *thread)
{
	struct xnsched *sched = thread->rpi;
	sched_removepq(&sched->rt.relaxed, &thread->xlink);
}

static inline struct xnthread *__xnsched_rt_peek_rpi(struct xnsched *sched)
{
	struct xnpholder *h = sched_getheadpq(&sched->rt.relaxed);
	return h ? link2thread(h, xlink) : NULL;
}

#endif /* CONFIG_XENO_OPT_PRIOCPL */

#endif /* __KERNEL__ || __XENO_SIM__ */

#endif /* !_XENO_NUCLEUS_SCHED_BUILTIN_H */
