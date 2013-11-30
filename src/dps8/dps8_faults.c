//
//  dps8_faults.c
//  dps8
//
//  Created by Harry Reed on 6/11/13.
//  Copyright (c) 2013 Harry Reed. All rights reserved.
//

#include <stdio.h>

#include "dps8.h"

static t_uint64 FR;

/*
 FAULT RECOGNITION
 For the discussion following, the term "function" is defined as a major processor functional cycle. Examples are: APPEND CYCLE, CA CYCLE, INSTRUCTION FETCH CYCLE, OPERAND STORE CYCLE, DIVIDE EXECUTION CYCLE. Some of these cycles are discussed in various sections of this manual.
 Faults in groups 1 and 2 cause the processor to abort all functions immediately by entering a FAULT CYCLE.
 Faults in group 3 cause the processor to "close out" current functions without taking any irrevocable action (such as setting PTW.U in an APPEND CYCLE or modifying an indirect word in a CA CYCLE), then to discard any pending functions (such as an APPEND CYCLE needed during a CA CYCLE), and to enter a FAULT CYCLE.
 Faults in group 4 cause the processor to suspend overlapped operation, to complete current and pending functions for the current instruction, and then to enter a FAULT CYCLE.
 Faults in groups 5 or 6 are normally detected during virtual address formation and instruction decode. These faults cause the processor to suspend overlapped operation, to complete the current and pending instructions, and to enter a FAULT CYCLE. If a fault in a higher priority group is generated by the execution of the current or pending instructions, that higher priority fault will take precedence and the group 5 or 6 fault will be lost. If a group 5 or 6 fault is detected during execution of the current instruction (e.g., an access violation, out of segment bounds, fault
 ￼￼
 during certain interruptible EIS instructions), the instruction is considered "complete" upon detection of the fault.
 Faults in group 7 are held and processed (with interrupts) at the completion of the current instruction pair. Group 7 faults are inhibitable by setting bit 28 of the instruction word.
 Faults in groups 3 through 6 must wait for the system controller to acknowledge the last access request before entering the FAULT CYCLE.
 */

/*
 
                                Table 7-1. List of Faults
 
 Decimal fault     Octal (1)      Fault   Fault name            Priority    Group
     number      fault address   mnemonic
        0      ;         0     ;      sdf  ;   Shutdown             ;   27     ;     7
        1      ;         2     ;      str  ;   Store                ;   10     ;     4
        2      ;         4     ;      mme  ;   Master mode entry 1  ;   11     ;     5
        3      ;         6     ;      f1   ;   Fault tag 1          ;   17     ;     5
        4      ;        10     ;      tro  ;   Timer runout         ;   26     ;     7
        5      ;        12     ;      cmd  ;   Command              ;   9      ;     4
        6      ;        14     ;      drl  ;   Derail               ;   15     ;     5
        7      ;        16     ;      luf  ;   Lockup               ;   5      ;     4
        8      ;        20     ;      con  ;   Connect              ;   25     ;     7
        9      ;        22     ;      par  ;   Parity               ;   8      ;     4
        10     ;        24     ;      ipr  ;   Illegal procedure    ;   16     ;     5
        11     ;        26     ;      onc  ;   Operation not complete ; 4      ;     2
        12     ;        30     ;      suf  ;   Startup              ;   1      ;     1
        13     ;        32     ;      ofl  ;   Overflow             ;   7      ;     3
        14     ;        34     ;      div  ;   Divide check         ;   6      ;     3
        15     ;        36     ;      exf  ;   Execute              ;   2      ;     1
        16     ;        40     ;      df0  ;   Directed fault 0     ;   20     ;     6
        17     ;        42     ;      df1  ;   Directed fault 1     ;   21     ;     6
        18     ;        44     ;      df2  ;   Directed fault 2     ;   22     ;     6
        19     ;        46     ;      df3  ;   Directed fault 3     ;   23     ;     6
        20     ;        50     ;      acv  ;   Access violation     ;   24     ;     6
        21     ;        52     ;      mme2 ;   Master mode entry 2  ;   12     ;     5
        22     ;        54     ;      mme3 ;   Master mode entry 3  ;   13     ;     5
        23     ;        56     ;      mme4 ;   Master mode entry 4  ;   14     ;     5
        24     ;        60     ;      f2   ;   Fault tag 2          ;   18     ;     5
        25     ;        62     ;      f3   ;   Fault tag 3          ;   19     ;     5
        26     ;        64     ;           ;   Unassigned           ;          ;
        27     ;        66     ;           ;   Unassigned           ;          ;
 
*/
struct dps8faults
{
    int         fault_number;
    int         fault_address;
    const char *fault_mnemonic;
    const char *fault_name;
    int         fault_priority;
    int         fault_group;
    bool        fault_pending;        // when true fault is pending and waiting to be processed
};

typedef struct dps8faults dps8faults;

#ifndef QUIET_UNUSED
static dps8faults _faultsP[] = { // sorted by priority
//  number  address  mnemonic   name                 Priority    Group
    {   12,     030,    "suf",  "Startup",                  1,	     1,     false },
    {   15,     036,    "exf",  "Execute",                  2,	     1,     false },
    {   31,     076,    "trb",  "Trouble",                  3,       2,     false },
    {   11,     026,    "onc",  "Operation not complete", 	4,	     2,     false },
    {   7,      016,    "luf",  "Lockup",               	5,	     4,     false },
    {   14,     034,    "div",  "Divide check",         	6,	     3,     false },
    {   13,     032,    "ofl",  "Overflow",             	7,	     3,     false },
    {   9,      022,    "par",  "Parity",               	8,	     4,     false },
    {   5,      012,    "cmd",  "Command",              	9,	     4,     false },
    {   1,       2 ,    "str",  "Store",                	10,	     4,     false },
    {   2,       4 ,    "mme",  "Master mode entry 1",  	11,	     5,     false },
    {   21,     052,    "mme2", "Master mode entry 2",  	12,	     5,     false },
    {   22,     054,    "mme3", "Master mode entry 3",  	13,	     5,     false },
    {   23,     056,    "mme4", "Master mode entry 4",  	14,	     5,     false },
    {   6,      014,    "drl",  "Derail",               	15,	     5,     false },
    {   10,     024,    "ipr",  "Illegal procedure",    	16,	     5,     false },
    {   3,       06,    "f1",   "Fault tag 1",          	17,	     5,     false },
    {   24,     060,    "f2",   "Fault tag 2",          	18,	     5,     false },
    {   25,     062,    "f3",   "Fault tag 3",          	19,	     5,     false },
    {   16,     040,    "df0",  "Directed fault 0",     	20,	     6,     false },
    {   17,     042,    "df1",  "Directed fault 1",     	21,	     6,     false },
    {   18,     044,    "df2",  "Directed fault 2",     	22,	     6,     false },
    {   19,     046,    "df3",  "Directed fault 3",     	23,	     6,     false },
    {   20,     050,    "acv",  "Access violation",     	24,	     6,     false },
    {   8,      020,    "con",  "Connect",              	25,	     7,     false },
    {   4,      010,    "tro",  "Timer runout",         	26,	     7,     false },
    {   0,       0 ,    "sdf",  "Shutdown",             	27,	     7,     false },
    {   26,     064,    "???",  "Unassigned",               -1,     -1,     false },
    {   27,     066,    "???",  "Unassigned",               -1,     -1,     false },
    {   -1,     -1,     NULL,   NULL,                       -1,     -1,     false }
};
#endif
dps8faults _faults[] = {    // sorted by number
    //  number  address  mnemonic   name                 Priority    Group
    {   0,       0 ,    "sdf",  "Shutdown",             	27,	     7,     false },
    {   1,       2 ,    "str",  "Store",                	10,	     4,     false },
    {   2,       4 ,    "mme",  "Master mode entry 1",  	11,	     5,     false },
    {   3,       06,    "f1",   "Fault tag 1",          	17,	     5,     false },
    {   4,      010,    "tro",  "Timer runout",         	26,	     7,     false },
    {   5,      012,    "cmd",  "Command",              	9,	     4,     false },
    {   6,      014,    "drl",  "Derail",               	15,	     5,     false },
    {   7,      016,    "luf",  "Lockup",               	5,	     4,     false },
    {   8,      020,    "con",  "Connect",              	25,	     7,     false },
    {   9,      022,    "par",  "Parity",               	8,	     4,     false },
    {   10,     024,    "ipr",  "Illegal procedure",    	16,	     5,     false },
    {   11,     026,    "onc",  "Operation not complete", 	4,	     2,     false },
    {   12,     030,    "suf",  "Startup",                  1,	     1,     false },
    {   13,     032,    "ofl",  "Overflow",             	7,	     3,     false },
    {   14,     034,    "div",  "Divide check",         	6,	     3,     false },
    {   15,     036,    "exf",  "Execute",                  2,	     1,     false },
    {   16,     040,    "df0",  "Directed fault 0",     	20,	     6,     false },
    {   17,     042,    "df1",  "Directed fault 1",     	21,	     6,     false },
    {   18,     044,    "df2",  "Directed fault 2",     	22,	     6,     false },
    {   19,     046,    "df3",  "Directed fault 3",     	23,	     6,     false },
    {   20,     050,    "acv",  "Access violation",     	24,	     6,     false },
    {   21,     052,    "mme2", "Master mode entry 2",  	12,	     5,     false },
    {   22,     054,    "mme3", "Master mode entry 3",  	13,	     5,     false },
    {   23,     056,    "mme4", "Master mode entry 4",  	14,	     5,     false },
    {   24,     060,    "f2",   "Fault tag 2",          	18,	     5,     false },
    {   25,     062,    "f3",   "Fault tag 3",          	19,	     5,     false },
    {   26,     064,    "???",  "Unassigned",               -1,     -1,     false },
    {   27,     066,    "???",  "Unassigned",               -1,     -1,     false },
    {   28,     070,    "???",  "Unassigned",               -1,     -1,     false },
    {   29,     072,    "???",  "Unassigned",               -1,     -1,     false },
    {   30,     074,    "???",  "Unassigned",               -1,     -1,     false },
    {   31,     076,    "trb",  "Trouble",                  3,       2,     false },

    {   -1,     -1,     NULL,   NULL,                       -1,     -1,     false }
};

//bool pending_fault = false;     // true when a fault has been signalled, but not processed


#ifndef QUIET_UNUSED
static bool port_interrupts[8] = {false, false, false, false, false, false, false, false };
#endif

//-----------------------------------------------------------------------------
// ***  Constants, unchanging lookup tables, etc

static int fault2group[32] = {
    // from AL39, page 7-3
    7, 4, 5, 5, 7, 4, 5, 4,
    7, 4, 5, 2, 1, 3, 3, 1,
    6, 6, 6, 6, 6, 5, 5, 5,
    5, 5, 0, 0, 0, 0, 0, 2
};
#ifndef QUIET_UNUSED
static int fault2prio[32] = {
    // from AL39, page 7-3
    27, 10, 11, 17, 26,  9, 15,  5,
    25,  8, 16,  4,  1,  7,  6,  2,
    20, 21, 22, 23, 24, 12, 13, 14,
    18, 19,  0,  0,  0,  0,  0,  3
};
#endif

// Fault conditions as stored in the "FR" Fault Register
// C99 and C++ would allow 64bit enums, but bits past 32 are related to (unimplemented) parity faults.
typedef enum {
    // Values are bit masks
    fr_ill_op = 1, // illegal opcode
    fr_ill_mod = 1 << 1, // illegal address modifier
    // fr_ill_slv = 1 << 2, // illegal BAR mode procedure
    fr_ill_proc = 1 << 3 // illegal procedure other than the above three
    // fr_ill_dig = 1 << 6 // illegal decimal digit
} fault_cond_t;

// "MR" Mode Register, L68
typedef struct {
    // See member "word" for the raw bits, other member values are derivations
    flag_t mr_enable; // bit 35 "n"
    flag_t strobe; // bit 30 "l"
    flag_t fault_reset; // bit 31 "m"
    t_uint64 word;
} mode_reg_t;

static mode_reg_t MR;

/*
 *  check_events()
 *
 *  Called after executing an instruction pair for xed.   The instruction pair
 *  may have included a rpt, rpd, or transfer.   The instruction pair may even
 *  have faulted, but if so, it was saved and restarted.
 */

void check_events (void)
{
    events.any = events.int_pending || events.low_group || events.group7;
    if (events.any)
        log_msg(NOTIFY_MSG, "CU", "check_events: event(s) found (%d,%d,%d).\n", events.int_pending, events.low_group, events.group7);
    
    return;
}

/*
 *  fault_gen()
 *
 *  Called by instructions or the addressing code to record the
 *  existance of a fault condition.
 */

void fault_gen(int f)
{
    int group;
    
#if 0
    if (f == oob_fault) {
        log_msg(ERR_MSG, "CU::fault", "Faulting for internal bug\n");
        f = trouble_fault;
        (void) cancel_run(STOP_BUG);
    }
#endif
    
    if (f < 1 || f > 32) {
        //log_msg(ERR_MSG, "CU::fault", "Bad fault # %d\n", f);
        cancel_run(STOP_BUG);
        return;
    }
    group = fault2group[f];
    if (group < 1 || group > 7) {
        //log_msg(ERR_MSG, "CU::fault", "Internal error.\n");
        cancel_run(STOP_BUG);
        return;
    }
    
    if (fault_gen_no_fault) {
        //log_msg(DEBUG_MSG, "CU::fault", "Ignoring fault # %d in group %d\n", f, group);
        return;
    }
    
    if (f == FAULT_IPR)
        FR |= fr_ill_proc;
    
    events.any = 1;
    //log_msg(DEBUG_MSG, "CU::fault", "Recording fault # %d in group %d\n", f, group);
    
    // Note that we never simulate a (hardware) op_not_complete_fault
    if (MR.mr_enable && (f == FAULT_ONC || MR.fault_reset)) {
        if (MR.strobe) {
            log_msg(INFO_MSG, "CU::fault", "Clearing MR.strobe.\n");
            MR.strobe = 0;
        } else
            log_msg(INFO_MSG, "CU::fault", "MR.strobe was already unset.\n");
    }
    
    if (group == 7) {
        // Recognition of group 7 faults is delayed and we can have
        // multiple group 7 faults pending.
        events.group7 |= (1 << f);
    } else {
        // Groups 1-6 are handled more immediately and there can only be
        // one fault pending within each group
        //if (cpu.cycle == FAULT_cycle)
        if (cpu.cycle == FAULT_cycle || cpu.cycle == FAULT_EXEC_cycle) {
            // FIXME: || events.xed AND/OR || cpu.cycle == FAULT_EXEC_cycle
            f = FAULT_TRB;
            group = fault2group[f];
            log_msg(WARN_MSG, "CU::fault", "Double fault:  Recording current fault as a trouble fault (fault # %d in group %d).\n", f, group);
            cpu.cycle = FAULT_cycle;
            //cancel_run(STOP_DIS); // BUG: not really
        } else {
            if (events.fault[group]) {
                // todo: error, unhandled fault
                log_msg(WARN_MSG, "CU::fault", "Found unhandled prior fault #%d in group %d.\n", events.fault[group], group);
            }
            if (cpu.cycle == EXEC_cycle) {
                // don't execute any pending odd half of an instruction pair
                cpu.cycle = FAULT_cycle;
            }
        }
        events.fault[group] = f;
    }
    if (events.low_group == 0 || group < events.low_group)
        events.low_group = group;   // new highest priority fault group
}

/*
 * fault_check_group
 *
 * Returns true if faults exist for the specifed group or for a higher
 * priority group.
 *
 */

#ifndef QUIET_UNUSED
static int fault_check_group(int group)
{
    
    if (group < 1 || group > 7) {
        log_msg(ERR_MSG, "CU::fault-check-group", "Bad group # %d\n", group);
        cancel_run(STOP_BUG);
        return 1;
    }
    
    if (! events.any)
        return 0;
    return events.low_group <= group;
}
#endif

/*
 * fault handler(s).
 */

#ifdef NOT_USED
t_stat doFaultInstructionPair(DCDstruct *i, word24 fltAddress)
{
    // XXX stolen from xed instruction
    
    DCDstruct _xip;   // our decoded instruction struct
    EISstruct _eis;

    word36 insPair[2];
    Read2(i, fltAddress, &insPair[0], &insPair[1], InstructionFetch, 0);
    
    _xip.IWB = insPair[0];
    _xip.e = &_eis;
    
    DCDstruct *xec = decodeInstruction(insPair[0], &_xip);    // fetch instruction into current instruction
    
    t_stat ret = executeInstruction(xec);
    
    if (ret)
        return (ret);
    
    _xip.IWB = insPair[1];
    _xip.e = &_eis;
    
    xec = decodeInstruction(insPair[1], &_xip);               // fetch instruction into current instruction
    
    ret = executeInstruction(xec);
    
    //if (ret)
    //    return (ret);
    //
    //return SCPE_OK;
    return ret;
}
#endif

static bool bFaultCycle = false;       // when true then in FAULT CYCLE

void doFault(DCDstruct *i, _fault faultNumber, _fault_subtype subFault, char *faultMsg)
{
    //if (faultNumber < 0 || faultNumber > 31)
    if (faultNumber & ~037)  // quicker?
    {
        printf("fault(out-of-range): %d %d '%s'\r\n", faultNumber, subFault, faultMsg ? faultMsg : "?");
        return;
    }

    dps8faults *f = &_faults[faultNumber];
    
    if (bFaultCycle)    // if already in a FAULT CYCLE then signal trouble faule
        f = &_faults[FAULT_TRB];
    else
    {
        // TODO: safe-store the Control Unit Data (see Section 3) into program-invisible holding registers in preparation for a Store Control Unit (scu) instruction,
    }
    
    int fltAddress = rFAULTBASE & 07740;            // (12-bits of which the top-most 7-bits are used)
    
    word24 addr = fltAddress + f->fault_address;    // absolute address of fault YPair
  
    bFaultCycle = true;                 // enter FAULT CYCLE
    
    word36 faultPair[2];
    core_read2(addr, faultPair, faultPair+1);
    
    // In the FAULT CYCLE, the processor safe-stores the Control Unit Data (see Section 3) into program-invisible holding registers in preparation for a Store Control Unit (scu) instruction, then enters temporary absolute mode, forces the current ring of execution C(PPR.PRR) to 0, and generates a computed address for the fault trap pair by concatenating the setting of the FAULT BASE switches on the processor configuration panel with twice the fault number (see Table 7-1). This computed address and the operation code for the Execute Double (xed) instruction are forced into the instruction register and executed as an instruction. Note that the execution of the instruction is not done in a normal EXECUTE CYCLE but in the FAULT CYCLE with the processor in temporary absolute mode.
    
    addr_modes_t am = get_addr_mode();  // save address mode
    
    PPR.PRR = 0;
    
    set_addr_mode(ABSOLUTE_mode);
    
    t_stat xrv = doXED(faultPair);
    
    bFaultCycle = false;                // exit FAULT CYCLE
    
    if (xrv == CONT_TRA)
        longjmp(jmpMain, JMP_TRA);      // execute transfer instruction
    
    set_addr_mode(am);      // If no transfer of control takes place, the processor returns to the mode in effect at the time of the fault and resumes normal sequential execution with the instruction following the faulting instruction (C(PPR.IC) + 1).
    
    if (xrv == 0)
        longjmp(jmpMain, JMP_NEXT);     // execute next instruction
    else if (0)                         // TODO: need to put test in to retry instruction
        longjmp(jmpMain, JMP_RETRY);    // retry instruction
    
    
//    printf("fault(): %d %d %s (%s) '%s'\r\n", f->fault_number, f->fault_group,  f->fault_name, f->fault_mnemonic, faultMsg ? faultMsg : "?");
//
//    if (f->fault_group == 7 && i && i->a)
//        return;
//
//    return;
//    longjmp(jmpMain, JMP_NEXT); // causes cpuCycles to not count the current instruction
//
//    pending_fault = true;
//    bool retry = false;
//    
//    int fltAddress = rFAULTBASE & 07740; // (12-bits of which the top-most 7-bits are used)
//    fltAddress += 2 * f->fault_number;
//    
//    f->fault_pending = true;        // this particular fault is pending, waiting for processing
//    
//    _processor_addressing_mode modeTemp = processorAddressingMode;
//    
//    processorAddressingMode = ABSOLUTE_MODE;
//    word24 rIC_temp = rIC;
//    
//    t_stat ret = doFaultInstructionPair(i, fltAddress);
//    
//    f->fault_pending = false;        
//    pending_fault = false;
//    
//    processorAddressingMode = modeTemp;
//    
//    // XXX we really only want to do this in extreme conditions since faults can be returned from *more-or-less*
//    // XXX do it properly - later..
//    
//    if (retry)
//        longjmp(jmpMain, JMP_RETRY);    // this will retry the faulted instruction
//    
//    if (ret == CONT_TRA)
//        longjmp(jmpMain, JMP_TRA);
}


