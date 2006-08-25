#ifndef GAUGE_H_
#define GAUGE_H_

#include "defs.h"

class IIR;

/*
 * Implements the dynamic behaviour of the gauge with a digital filter.
 */
class Gauge {
	
	private:
		IIR*	filter;
		FLT		position;
	
	public:
		Gauge(FLT initial_position);
		virtual ~Gauge();
		
		
		void compute(FLT sample);
		FLT getPosition();
};

#endif /*GAUGE_H_*/
