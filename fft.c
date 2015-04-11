/* Copyright (C) 1998-2007 Kengo Ichiki <kichiki@users.sourceforge.net>
 * $Id: fft.c,v 1.9 2007/11/04 23:44:44 kichiki Exp $
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
#include <stdlib.h> /* realloc()  */
#include <stdio.h> /* fprintf()  */

/* FFTW library  */
#ifdef FFTW2
#include <rfftw.h>
#else // FFTW3
#include <fftw3.h>
#endif // FFTW2

#include "memory-check.h" // CHECK_MALLOC() macro

#include "hc.h" // HC_to_amp2()


/* Reference: "Numerical Recipes in C" 2nd Ed.
 * by W.H.Press, S.A.Teukolsky, W.T.Vetterling, B.P.Flannery
 * (1992) Cambridge University Press.
 * ISBN 0-521-43108-5
 * Sec.13.4 - Data Windowing
 */
float
parzen (int i, int nn)
{
  return (1.0 - fabs (((float)i-0.5*(float)(nn-1))
		      /(0.5*(float)(nn+1))));
}

float
welch (int i, int nn)
{
  return (1.0-(((float)i-0.5*(float)(nn-1))
	       /(0.5*(float)(nn+1)))
	  *(((float)i-0.5*(float)(nn-1))
	    /(0.5*(float)(nn+1))));
}

float
hanning (int i, int nn)
{
  return ( 0.5 * (1.0 - cos (2.0*M_PI*(float)i/(float)(nn-1))) );
}

/* Reference: "Digital Filters and Signal Processing" 2nd Ed.
 * by L. B. Jackson. (1989) Kluwer Academic Publishers.
 * ISBN 0-89838-276-9
 * Sec.7.3 - Windows in Spectrum Analysis
 */
float
hamming (int i, int nn)
{
  return ( 0.54 - 0.46 * cos (2.0*M_PI*(float)i/(float)(nn-1)) );
}

float
blackman (int i, int nn)
{
  return ( 0.42 - 0.5 * cos (2.0*M_PI*(float)i/(float)(nn-1))
	  + 0.08 * cos (4.0*M_PI*(float)i/(float)(nn-1)) );
}

float
steeper (int i, int nn)
{
  return ( 0.375
	  - 0.5 * cos (2.0*M_PI*(float)i/(float)(nn-1))
	  + 0.125 * cos (4.0*M_PI*(float)i/(float)(nn-1)) );
}

/* apply window function to data[]
 * INPUT
 *  flag_window : 0 : no-window (default -- that is, other than 1 ~ 6)
 *                1 : parzen window
 *                2 : welch window
 *                3 : hanning window
 *                4 : hamming window
 *                5 : blackman window
 *                6 : steeper 30-dB/octave rolloff window
 */
void
windowing (int n, fftw_complex *data, int flag_window, float scale)
{
  int i;
  for (i = 0; i < n; i ++)
    {
      switch (flag_window)
	{
	case 1: // parzen window
	  data[i][0] = data[i][0] * parzen (i, n) / scale;
	  break;

	case 2: // welch window
	  data[i][0] = data[i][0] * welch (i, n) / scale;
	  break;

	case 3: // hanning window
	  data[i][0] = data [i][0] * hanning (i, n) / scale;
	  break;

	case 4: // hamming window
	  data[i][0] = data [i][0] * hamming (i, n) / scale;
	  break;

	case 5: // blackman window
	  data[i][0] = data [i][0] * blackman (i, n) / scale;
	  break;

	case 6: // steeper 30-dB/octave rolloff window
	  data[i][0] = data [i][0] * steeper (i, n) / scale;
	  break;

	default:
	  fprintf (stderr, "invalid flag_window\n");
	case 0: // square (no window)
	  data[i][0] = data [i][0] / scale;
	  break;
	}
    }
}

/* apply FFT with the window and return amplitude and phase
 * this is a wrapper mainly for phase vocoder process
 * INPUT
 *  len : FFT length
 *  data[len] : data to analyze
 *  flag_window : window type
 *  plan, in[len], out[len] : for FFTW3
 *  scale : amplitude scale factor
 * OUTPUT
 *  amp[len/2+1] : amplitude multiplied by the factor "scale" above
 *  phs[len/2+1] : phase
 */
void
apply_FFT (int len, fftw_complex *data, int flag_window,
	   fftw_plan plan, fftw_complex *in, fftw_complex *out,
	   float scale,
	   float *amp, float *phs)
{
  int i;

  windowing (len, data, flag_window, 1.0);
  fftw_execute (plan); // FFT: in[] -> out[]
  HC_to_polar (len, out, 0, amp, phs); // freq[] -> (amp, phs)

  // some scaling
  for (i = 0; i < (len/2)+1; i ++)
    {
      amp [i] /= scale;
    }
}


/* prepare window for FFT
 * INPUT
 *  n : # of samples for FFT
 *  flag_w/indow : 0 : no-window (default -- that is, other than 1 ~ 6)
 *                1 : parzen window
 *                2 : welch window
 *                3 : hanning window
 *                4 : hamming window
 *                5 : blackman window
 *                6 : steeper 30-dB/octave rolloff window
 * OUTPUT
 *  density factor as RETURN VALUE
 */
float
init_den (int n, char flag_window)
{
  float den;
  int i;

  den = 0.0;
  for (i = 0; i < n; i ++)
    {
      switch (flag_window)
	{
	case 1: // parzen window
	  den += parzen (i, n) * parzen (i, n);
	  break;

	case 2: // welch window
	  den += welch (i, n) * welch (i, n);
	  break;

	case 3: // hanning window
	  den += hanning (i, n) * hanning (i, n);
	  break;

	case 4: // hamming window
	  den += hamming (i, n) * hamming (i, n);
	  break;

	case 5: // blackman window
	  den += blackman (i, n) * blackman (i, n);
	  break;

	case 6: // steeper 30-dB/octave rolloff window
	  den += steeper (i, n) * steeper (i, n);
	  break;

	default:
	  fprintf (stderr, "invalid flag_window\n");
	case 0: // square (no window)
	  den += 1.0;
	  break;
	}
    }

  den *= (float)n;

  return den;
}


/* calc power spectrum of real data x[n]
 * INPUT
 *  n : # of data in x
 *  x[] : data
 *  y[] : for output (you have to allocate before calling)
 *  den : weight of window function; calculated by init_den().
 *  flag_window : 0 : no-window (default -- that is, other than 1 ~ 6)
 *                1 : parzen window
 *                2 : welch window
 *                3 : hanning window
 *                4 : hamming window
 *                5 : blackman window
 *                6 : steeper 30-dB/octave rolloff window
 * OUTPUT
 *  y[] : fourier transform of x[]
 *  p[(n+1)/2] : stored only n/2 data
 */
void
power_spectrum_fftw (int n, fftw_complex *x, fftw_complex *y, float *p, 
		     float den,
		     char flag_window,
		     fftw_plan plan)
{
  static float maxamp = 2147483647.0; /* 2^32-1  */

  /* window */
  windowing (n, x, flag_window, maxamp);

/* FFTW library  */
  fftw_execute (plan); // x[] -> y[]
  printf("lol");
  HC_to_amp2 (n, y, den, p);
}

