
#include "dps8.h"

#define N_CLK_UNITS 1
static t_stat clk_svc(UNIT *up);
UNIT TR_clk_unit [N_CLK_UNITS] = {{ UDATA(&clk_svc, UNIT_IDLE, 0) }};

static t_stat clk_svc(UNIT *up)
{
    // only valid for TR
    (void) sim_rtcn_calb (CLK_TR_HZ, TR_CLK);   // calibrate clock
    uint32 t = sim_is_active(&TR_clk_unit[0]);
    log_msg(INFO_MSG, "SYS::clock::service", "TR has %d time units left\n", t);
    return 0;
}

#define reg_TR rTR

#ifndef QUIET_UNUSED
static int activate_timer (void)
{
    uint32 t;
    log_msg(DEBUG_MSG, "SYS::clock", "TR is %lld %#llo.\n", reg_TR, reg_TR);
    if (bit_is_neg(reg_TR, 27)) {
        if ((t = sim_is_active(&TR_clk_unit[0])) != 0)
            log_msg(DEBUG_MSG, "SYS::clock", "TR cancelled with %d time units left.\n", t);
        else
            log_msg(DEBUG_MSG, "SYS::clock", "TR loaded with negative value, but it was alread stopped.\n", t);
        sim_cancel(&TR_clk_unit[0]);
        return 0;
    }
    if ((t = sim_is_active(&TR_clk_unit[0])) != 0) {
        log_msg(DEBUG_MSG, "SYS::clock", "TR was still running with %d time units left.\n", t);
        sim_cancel(&TR_clk_unit[0]);   // BUG: do we need to cancel?
    }
    
    (void) sim_rtcn_init(CLK_TR_HZ, TR_CLK);
    sim_activate(&TR_clk_unit[0], reg_TR);
    if ((t = sim_is_active(&TR_clk_unit[0])) == 0)
        log_msg(DEBUG_MSG, "SYS::clock", "TR is not running\n", t);
    else
        log_msg(DEBUG_MSG, "SYS::clock", "TR is now running with %d time units left.\n", t);
    return 0;
}
#endif

#ifdef QUIET_UNUSED
t_stat XX_clk_svc(UNIT *up)
{
    // only valid for TR
#if 0
    tmr_poll = sim_rtcn_calb (clk_tps, TMR_CLK);            /* calibrate clock */
    sim_activate (&clk_unit, tmr_poll);                     /* reactivate unit */
    tmxr_poll = tmr_poll * TMXR_MULT;                       /* set mux poll */
    todr_reg = todr_reg + 1;                                /* incr TODR */
    if ((tmr_iccs & TMR_CSR_RUN) && tmr_use_100hz)          /* timer on, std intvl? */
        tmr_incr (TMR_INC);                                 /* do timer service */
    return 0;
#else
    return 2;
#endif
}
#endif

