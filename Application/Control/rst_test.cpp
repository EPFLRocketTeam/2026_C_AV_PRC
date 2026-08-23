
// This file is entirely meant to feed back data into the rst controller to test its behaviour.

#ifdef TEST_RST_CONTROLLER

#include <bits/stdc++.h>
#include "Application/Control/rst_controller.hpp"

using namespace std;

float r[100], s[100], t[100];

int main (void) {
    prc::RstController controller(r, s, t);
    prc::UpdateRstPolynomials(r, s, t, 7); // pressurize to 7 bars

    controller.reset(0);
    for (int i = 0; i < 1400; i ++) {
        float target = ((float) i) / 1400. * 60.;
        float value  = ((float) i) / 1400.;
        float ctrl = controller.update(target, value);

        cout << "=== Tick " << i << " ===\n";
        cout << value << " -> " << target << endl;
        cout << "Control : " << ctrl << "\n";
        cout << "Fixed : " << prc::FlowToAngleDeg(ctrl) << "\n";
        cout << endl;
    }

    controller.reset(1);
    prc::UpdateRstPolynomials(r, s, t, 5); // pressurize to 7 bars

    for (int i = 0; i < 1400; i ++) {
        float target = 1.;
        float value =  ((float) i) / 1400. * 2.7 + 1.;
        float ctrl = controller.update(target, value);
        
        cout << "=== Tick " << i << " ===\n";
        cout << value << " -> " << target << endl;
        cout << "Control : " << ctrl << "\n";
        cout << "Fixed : " << prc::FlowToAngleDeg(ctrl) << "\n";
        cout << endl;
    }
}

#endif
