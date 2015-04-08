/* WaoN - a Wave-to-Notes transcriber : main
 * Copyright (C) 1998-2007 Kengo Ichiki <kichiki@users.sourceforge.net>
 * $Id: main.c,v 1.10 2007/11/04 23:51:36 kichiki Exp $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <math.h>
#include <stdio.h> /* printf(), fprintf(), strerror()  */
#include <sys/errno.h> /* errno  */
#include <stdlib.h> /* exit()  */
#include <string.h> /* strcat(), strcpy()  */
#include "memory-check.h" // CHECK_MALLOC() macro

/* FFTW library */
#ifdef FFTW2
#include <rfftw.h>
#else // FFTW3
#include <fftw3.h>
#endif // FFTW2

#include "fft.h" // FFT utility functions
#include "hc.h" // HC array manipulation routines

void main (void){
  int i;
  long len 2048; // number of samples
  int flag_window = 3;
 
  double *x = NULL; /* wave data for FFT  */
  double *y = NULL; /* spectrum data for FFT */ 

#ifdef FFTW2
  x = (double *)malloc (sizeof (double) * len);
  y = (double *)malloc (sizeof (double) * len);
#else // FFTW3
  x = (double *)fftw_malloc (sizeof (double) * len);
  y = (double *)fftw_malloc (sizeof (double) * len);
#endif // FFTW2
  CHECK_MALLOC (x, "main");
  CHECK_MALLOC (y, "main");

  /* power spectrum  */
  double *p = (double *)malloc (sizeof (double) * (len / 2 + 1));
  CHECK_MALLOC (p, "main");

  double *p0 = NULL;
  double *dphi = NULL;
  double *ph0 = NULL;
  double *ph1 = NULL;
  // time-period for FFT (inverse of smallest frequency)
  double t0 = (double)len/(double)SAMPLERATE;
  // initialization plan for FFTW
  double den = init_den (len, flag_window);
#ifdef FFTW2
  rfftw_plan plan;
  plan = rfftw_create_plan (len, FFTW_REAL_TO_COMPLEX, FFTW_ESTIMATE);
#else // FFTW3
  fftw_plan plan;
  plan = fftw_plan_r2r_1d (len, x, y, FFTW_R2HC, FFTW_ESTIMATE);
#endif
  // weight of window function for FFT
  
  /*calc power spectrum
       */ 
    //MAKE SURE TO AVERAGE CHANNELS
    windowing (len, x, flag_window, 1.0, x);

	  // no phase-vocoder correction
	  HC_to_amp2 (len, y, den, p);
/*      else
	{
	  // with phase-vocoder correction
	  HC_to_polar2 (len, y, 0, den, p, ph1);

	  if (icnt == 0) // first step, so no ph0[] yet
	    {
	      for (i = 0; i < (len/2+1); ++i) // full span
		{
		  // no correction
		  dphi[i] = 0.0;

		  // backup the phase for the next step
		  p0  [i] = p   [i];
		  ph0 [i] = ph1 [i];
		}	  
	    }
	  else // icnt > 0
	    {
	      // freq correction by phase difference
	      for (i = 0; i < (len/2+1); ++i) // full span
		{
		  double twopi = 2.0 * M_PI;
		  //double dphi;
		  dphi[i] = ph1[i] - ph0[i]
		    - twopi * (double)i / (double)len * (double)hop;
		  for (; dphi[i] >= M_PI; dphi[i] -= twopi);
		  for (; dphi[i] < -M_PI; dphi[i] += twopi);

		  // frequency correction
		  // NOTE: freq is (i / len + dphi) * samplerate [Hz]
		  dphi[i] = dphi[i] / twopi / (double)hop;

		  // backup the phase for the next step
		  p0  [i] = p   [i];
		  ph0 [i] = ph1 [i];

		  // then, average the power for the analysis
		  p[i] = 0.5 *(sqrt (p[i]) + sqrt (p0[i]));
		  p[i] = p[i] * p[i];
		}
	    }
	}*/
}