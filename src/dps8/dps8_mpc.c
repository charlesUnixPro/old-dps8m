/**
 * \file dps8_mpc.c
 * \project dps8
 * \date 27Nov13
 * \copyright Copyright (c) 2013 Charles A. Clinton. All rights reserved.
*/

#if 0
#include <stdio.h>

#include "dps8.h"

static REG mpc_reg [] =
  {
    { 0 }
  };

#define N_MPC_UNITS_MAX 16 // XXX Verify

#define N_MPC_UNITS 1 // default

UNIT mpc_unit [N_MPC_UNITS_MAX] = {
{ UDATA(NULL, 0, 0) },
{ UDATA(NULL, 0, 0) },
{ UDATA(NULL, 0, 0) },
{ UDATA(NULL, 0, 0) },
{ UDATA(NULL, 0, 0) },
{ UDATA(NULL, 0, 0) },
{ UDATA(NULL, 0, 0) },
{ UDATA(NULL, 0, 0) },
{ UDATA(NULL, 0, 0) },
{ UDATA(NULL, 0, 0) },
{ UDATA(NULL, 0, 0) },
{ UDATA(NULL, 0, 0) },
{ UDATA(NULL, 0, 0) },
{ UDATA(NULL, 0, 0) },
{ UDATA(NULL, 0, 0) },
{ UDATA(NULL, 0, 0) }
};
DEVICE mpc_dev =
  {
    "MPC",      /* name */
     mpc_unit,   /* units */
     mpc_reg,    /* registers */
     NULL,       /* modifiers */
     N_MPC_UNITS,          /* #units */
     10,         /* address radix */
     8,          /* address width */
     1,          /* address increment */
     8,          /* data radix */
     8,          /* data width */
     NULL,       /* examine routine */
     NULL,       /* deposit routine */
     NULL, /* reset routine */
     NULL,  /* boot routine */
     NULL,       /* attach routine */
     NULL,       /* detach routine */
     NULL,       /* context */
     DEV_DEBUG,  /* flags */
     0,          /* debug control flags */
     0,          /* debug flag names */
     NULL,       /* memory size change */
     NULL        /* logical name */
  };

// Devices hooked to an MPC are identified by a six bit dev_code
#define NUM_DEVS 64

static struct
  {
    int mpc_dev_type;
    int dev_unit_num;
  } cables_from_units [N_MPC_UNITS_MAX] [NUM_DEVS];

static struct
  {
    int iom_unit_num;
    int chan_num;
  } cables_from_ioms [N_MPC_UNITS_MAX];

t_stat cable_to_mpc (int mpc_unit_num, int dev_code, enum dev_type mpc_dev_type, int dev_unit_num)
  {
    if (mpc_unit_num < 0 || mpc_unit_num >= mpc_dev . numunits)
      {
        sim_debug (DBG_ERR, & mpc_dev, "cable_to_mpc: mpc_unit_num out of range <%d>\n", mpc_unit_num);
        out_msg ("cable_to_mpc: mpc_unit_num out of range <%d>\n", mpc_unit_num);
        return SCPE_ARG;
      }

    if (dev_code < 0 || dev_code >= NUM_DEVS)
      {
        sim_debug (DBG_ERR, & mpc_dev, "cable_to_mpc: dev_code out of range <%d>\n", dev_code);
        out_msg ("cable_to_mpc: dev_code out of range <%d>\n", dev_code);
        return SCPE_ARG;
      }

    if (cables_from_units [mpc_unit_num] [dev_code] . mpc_dev_type != DEVT_NONE)
      {
        sim_debug (DBG_ERR, & mpc_dev, "cable_to_mpc: socket in use\n");
        out_msg ("cable_to_mpc: socket in use\n");
        return SCPE_ARG;
      }

    cables_from_units [mpc_unit_num] [dev_code] . mpc_dev_type = mpc_dev_type;
    cables_from_units [mpc_unit_num] [dev_code] . dev_unit_num = dev_unit_num;

    return SCPE_OK;
  }

//
// String a cable from a mpc to an IOM
//
// This end: mpc_unit_num
// That end: iom_unit_num, chan_num
// 

t_stat cable_mpc (int mpc_unit_num, int iom_unit_num, int chan_num)
  {
    if (mpc_unit_num < 0 || mpc_unit_num >= mpc_dev . numunits)
      {
        sim_debug (DBG_ERR, & mpc_dev, "cable_mpc: mpc_unit_num out of range <%d>\n", mpc_unit_num);
        out_msg ("cable_mpc: mpc_unit_num out of range <%d>\n", mpc_unit_num);
        return SCPE_ARG;
      }

    // Plug the other end of the cable in
    t_stat rc = cable_to_iom (iom_unit_num, chan_num, DEVT_MPC, mpc_unit_num);

    if (rc)
      return rc;

    if (cables_from_ioms [mpc_unit_num] . iom_unit_num != -1)
      {
        sim_debug (DBG_ERR, & mpc_dev, "cable_mpc: socket in use\n");
        out_msg ("cable_mpc: socket in use\n");
        return SCPE_ARG;
      }

    cables_from_ioms [mpc_unit_num] . iom_unit_num = iom_unit_num;
    cables_from_ioms [mpc_unit_num] . chan_num = chan_num;

    return SCPE_OK;
  }

void mpc_init (void) // one time initialization
  {
// init cables_from_units to MPC_NO_DEV
    memset (cables_from_units, 0, sizeof (cables_from_units));
    for (int i = 0; i < N_MPC_UNITS_MAX; i ++)
      cables_from_ioms [i] . iom_unit_num = -1;
  }
#endif
