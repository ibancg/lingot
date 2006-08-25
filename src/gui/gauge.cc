
#include "gauge.h"
#include "iir.h"

Gauge::Gauge(FLT initial_position) {
	
  //
  // ----- ERROR GAUGE FILTER CONFIGURATION -----
  //
  // dynamic model of the gauge:
  //
  //               2
  //              d                              d      
  //              --- s(t) = k (e(t) - s(t)) - q -- s(t)
  //                2                            dt     
  //              dt
  //
  // acceleration of gauge position 's(t)' linealy depends on the difference
  // respect to the input stimulus 'e(t)' (destination position). Inserting
  // a friction coefficient 'q', acceleration proportionaly diminish with
  // velocity (typical friction in mechanics). 'k' is the adaptation constant,
  // and depends on the gauge mass.
  //
  // with the following derivatives approximation (valid for high sample rate):
  //
  //                 d
  //                 -- s(t) ~= (s[n] - s[n - 1])*fs
  //                 dt
  //
  //            2
  //           d                                            2
  //           --- s(t) ~= (s[n] - 2*s[n - 1] + s[n - 2])*fs
  //             2
  //           dt
  //
  // we can obtain a difference equation, and implement it with an IIR digital
  // filter.
  //

  FLT k = 60; // adaptation constant.
  FLT q = 6;  // friction coefficient.

  FLT a[] = {
    k + GAUGE_RATE*(q + GAUGE_RATE),
    -GAUGE_RATE*(q + 2.0*GAUGE_RATE),  
      GAUGE_RATE*GAUGE_RATE };
  FLT b[] = { k };

  filter = new IIR( 2, 0, a, b );
	
	compute(initial_position);
}

Gauge::~Gauge() {
	delete filter;
}

void Gauge::compute(FLT sample) {
	position = filter->filter(sample);
}

FLT Gauge::getPosition() {
	return position;
}
