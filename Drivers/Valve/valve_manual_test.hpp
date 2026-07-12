//    FILE: valve_manual_test.hpp
// PURPOSE: C-callable smoke test for the valve drivers, for use from main.c.
//          Cycles every registered valve open/close (and sweeps position on
//          the two servo ball valves) so wiring/polarity can be verified on
//          the bench before it's part of a real sequence.

#ifndef VALVE_MANUAL_TEST_H
#define VALVE_MANUAL_TEST_H

#ifdef __cplusplus
extern "C" {
#endif

void Valve_ManualTest(void);

#ifdef __cplusplus
}
#endif

#endif // VALVE_MANUAL_TEST_H
