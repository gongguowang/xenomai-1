/*!\file pod.h
 * \brief Real-time pod interface header.
 * \author Philippe Gerum
 *
 * Copyright (C) 2001-2007 Philippe Gerum <rpm@xenomai.org>.
 * Copyright (C) 2004 The RTAI project <http://www.rtai.org>
 * Copyright (C) 2004 The HYADES project <http://www.hyades-itea.org>
 * Copyright (C) 2004 The Xenomai project <http://www.xenomai.org>
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
 *
 * \ingroup pod
 */

#ifndef _XENO_NUCLEUS_POD_H
#define _XENO_NUCLEUS_POD_H

/*! \addtogroup pod
 *@{*/

#include <nucleus/sched.h>

/* Pod status flags */
#define XNFATAL  0x00000001	/* Fatal error in progress */
#define XNPEXEC  0x00000002	/* Pod is active (a skin is attached) */

/* Sched status flags */
#define XNKCOUT  0x80000000	/* Sched callout context */
#define XNHTICK  0x40000000	/* Host tick pending  */
#define XNRPICK  0x20000000	/* Check RPI state */
#define XNINTCK  0x10000000	/* In master tick handler context */

/* These flags are available to the real-time interfaces */
#define XNPOD_SPARE0  0x01000000
#define XNPOD_SPARE1  0x02000000
#define XNPOD_SPARE2  0x04000000
#define XNPOD_SPARE3  0x08000000
#define XNPOD_SPARE4  0x10000000
#define XNPOD_SPARE5  0x20000000
#define XNPOD_SPARE6  0x40000000
#define XNPOD_SPARE7  0x80000000

#define XNPOD_NORMAL_EXIT  0x0
#define XNPOD_FATAL_EXIT   0x1

#define XNPOD_ALL_CPUS  XNARCH_CPU_MASK_ALL

#define XNPOD_FATAL_BUFSZ  16384

#define nkpod (&nkpod_struct)

struct xnsynch;

/*! 
 * \brief Real-time pod descriptor.
 *
 * The source of all Xenomai magic.
 */

struct xnpod {

	xnflags_t status;	/*!< Status bitmask. */

	xnsched_t sched[XNARCH_NR_CPUS];	/*!< Per-cpu scheduler slots. */

	xnqueue_t threadq;	/*!< All existing threads. */
	int threadq_rev;	/*!< Modification counter of threadq. */

	xnqueue_t tstartq,	/*!< Thread start hook queue. */
	 tswitchq,		/*!< Thread switch hook queue. */
	 tdeleteq;		/*!< Thread delete hook queue. */

	atomic_counter_t timerlck;	/*!< Timer lock depth.  */

	int refcnt;		/*!< Reference count.  */

#ifdef __XENO_SIM__
	void (*schedhook) (xnthread_t *thread, xnflags_t mask);	/*!< Internal scheduling hook. */
#endif	/* __XENO_SIM__ */
};

typedef struct xnpod xnpod_t;

DECLARE_EXTERN_XNLOCK(nklock);

extern u_long nklatency;

extern u_long nktimerlat;

extern char *nkmsgbuf;

extern xnarch_cpumask_t nkaffinity;

extern xnpod_t nkpod_struct;

#ifdef __cplusplus
extern "C" {
#endif

void xnpod_renice_thread_inner(xnthread_t *thread, int prio, int propagate);

#ifdef CONFIG_XENO_HW_FPU
void xnpod_switch_fpu(xnsched_t *sched);
#endif /* CONFIG_XENO_HW_FPU */

void __xnpod_finalize_zombie(xnsched_t *sched);

static inline void xnpod_finalize_zombie(xnsched_t *sched)
{
	if (sched->zombie)
		__xnpod_finalize_zombie(sched);
}

	/* -- Beginning of the exported interface */

#define xnpod_sched_slot(cpu) \
    (&nkpod->sched[cpu])

#define xnpod_current_sched() \
    xnpod_sched_slot(xnarch_current_cpu())

#define xnpod_active_p() \
    (!!testbits(nkpod->status, XNPEXEC))

#define xnpod_fatal_p() \
    (!!testbits(nkpod->status, XNFATAL))

#define xnpod_interrupt_p() \
    (xnpod_current_sched()->inesting > 0)

#define xnpod_callout_p() \
    (!!testbits(xnpod_current_sched()->status,XNKCOUT))

#define xnpod_asynch_p() \
    (xnpod_interrupt_p() || xnpod_callout_p())

#define xnpod_current_thread() \
    (xnpod_current_sched()->curr)

#define xnpod_current_root() \
    (&xnpod_current_sched()->rootcb)

#ifdef CONFIG_XENO_OPT_PERVASIVE
#define xnpod_current_p(thread)					\
    ({ int __shadow_p = xnthread_test_state(thread, XNSHADOW);		\
       int __curr_p = __shadow_p ? xnshadow_thread(current) == thread	\
	   : thread == xnpod_current_thread();				\
       __curr_p;})
#else
#define xnpod_current_p(thread) \
    (xnpod_current_thread() == (thread))
#endif

#define xnpod_locked_p() \
    (!!xnthread_test_state(xnpod_current_thread(),XNLOCK))

#define xnpod_unblockable_p() \
    (xnpod_asynch_p() || xnthread_test_state(xnpod_current_thread(),XNROOT))

#define xnpod_root_p() \
    (!!xnthread_test_state(xnpod_current_thread(),XNROOT))

#define xnpod_shadow_p() \
    (!!xnthread_test_state(xnpod_current_thread(),XNSHADOW))

#define xnpod_userspace_p() \
    (!!xnthread_test_state(xnpod_current_thread(),XNROOT|XNSHADOW))

#define xnpod_primary_p() \
    (!(xnpod_asynch_p() || xnpod_root_p()))

#define xnpod_secondary_p()	xnpod_root_p()

#define xnpod_idle_p()		xnpod_root_p()

int xnpod_init(void);

int xnpod_enable_timesource(void);

void xnpod_disable_timesource(void);

void xnpod_shutdown(int xtype);

int xnpod_init_thread(xnthread_t *thread,
		      xntbase_t *tbase,
		      const char *name,
		      int prio,
		      xnflags_t flags,
		      unsigned stacksize,
		      xnthrops_t *ops);

int xnpod_start_thread(xnthread_t *thread,
		       xnflags_t mode,
		       int imask,
		       xnarch_cpumask_t affinity,
		       void (*entry) (void *cookie),
		       void *cookie);

void xnpod_restart_thread(xnthread_t *thread);

void xnpod_delete_thread(xnthread_t *thread);

void xnpod_abort_thread(xnthread_t *thread);

xnflags_t xnpod_set_thread_mode(xnthread_t *thread,
				xnflags_t clrmask,
				xnflags_t setmask);

void xnpod_suspend_thread(xnthread_t *thread,
			  xnflags_t mask,
			  xnticks_t timeout,
			  xntmode_t timeout_mode,
			  struct xnsynch *wchan);

void xnpod_resume_thread(xnthread_t *thread,
			 xnflags_t mask);

int xnpod_unblock_thread(xnthread_t *thread);

void xnpod_renice_thread(xnthread_t *thread,
			 int prio);

int xnpod_migrate_thread(int cpu);

void xnpod_schedule(void);

void xnpod_dispatch_signals(void);

static inline void xnpod_lock_sched(void)
{
	xnthread_t *curr;
	spl_t s;

	xnlock_get_irqsave(&nklock, s);

	curr = xnpod_current_sched()->curr;

	if (xnthread_lock_count(curr)++ == 0)
		xnthread_set_state(curr, XNLOCK);

	xnlock_put_irqrestore(&nklock, s);
}

static inline void xnpod_unlock_sched(void)
{
	xnthread_t *curr;
	spl_t s;

	xnlock_get_irqsave(&nklock, s);

	curr = xnpod_current_sched()->curr;

	if (--xnthread_lock_count(curr) == 0) {
		xnthread_clear_state(curr, XNLOCK);
		xnpod_schedule();
	}

	xnlock_put_irqrestore(&nklock, s);
}

void xnpod_fire_callouts(xnqueue_t *hookq,
			 xnthread_t *thread);

void xnpod_activate_rr(xnticks_t quantum);

void xnpod_deactivate_rr(void);

int xnpod_set_thread_periodic(xnthread_t *thread,
			      xnticks_t idate,
			      xnticks_t period);

int xnpod_wait_thread_period(unsigned long *overruns_r);

static inline xntime_t xnpod_get_cpu_time(void)
{
	return xnarch_get_cpu_time();
}

int xnpod_add_hook(int type, void (*routine) (xnthread_t *));

int xnpod_remove_hook(int type, void (*routine) (xnthread_t *));

static inline void xnpod_yield(void)
{
	xnpod_resume_thread(xnpod_current_thread(), 0);
	xnpod_schedule();
}

static inline void xnpod_delay(xnticks_t timeout)
{
	xnpod_suspend_thread(xnpod_current_thread(), XNDELAY, timeout, XN_RELATIVE, NULL);
}

static inline void xnpod_suspend_self(void)
{
	xnpod_suspend_thread(xnpod_current_thread(), XNSUSP, XN_INFINITE, XN_RELATIVE, NULL);
}

static inline void xnpod_delete_self(void)
{
	xnpod_delete_thread(xnpod_current_thread());
}

#ifdef __cplusplus
}
#endif

/*@}*/

#endif /* !_XENO_NUCLEUS_POD_H */
