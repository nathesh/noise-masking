/* headerfile for fft.c --
 * FFT subroutine for WaoN with FFTW library
 * Copyright (C) 1998-2007 Kengo Ichiki <kichiki@users.sourceforge.net>
 * $Id: fft.h,v 1.7 2007/02/28 08:34:23 kichiki Exp $
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
#ifndef	_FFT_H_
#define	_FFT_H_
#include <fftw3.h>

float parzen (int i, int nn);
float welch (int i, int nn);
float hanning (int i, int nn);
float hamming (int i, int nn);
float blackman (int i, int nn);
float steeper (int i, int nn);

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
windowing (int n, fftw_complex *data, int flag_window, float scale);
void
fprint_window_name (FILE *out, int flag_window);

/* apply FFT with the window and return amplitude and phase
 * this is a wrapper mainly for phase vocoder process
 * INPUT
 *  len : FFT length
 *  data[len] : data to analyze
 *  flag_window : window type
 *  plan, in, out : for FFTW3
 *  scale : amplitude scale factor
 * OUTPUT
 *  amp[len/2+1] : amplitude multiplied by the factor "scale" above
 *  phs[len/2+1] : phase
 */
void
apply_FFT (int len, const float *data, int flag_window,
	   fftw_plan plan, float *in, float *out,
	   float scale,
	   float *amp, float *phs);


/* prepare window for FFT
 * INPUT
 *  n : # of samples for FFT
 *  flag_window : 0 : no-window (default -- that is, other than 1 ~ 6)
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
init_den (int n, char flag_window);


/* calc power spectrum of real data x[n] and weight with A weighting
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
weighted_power_spectrum_fftw (int n, fftw_complex *x, fftw_complex *y, float *p,
			 float *weights,
		     float den,
		     char flag_window,
		     fftw_plan plan);


/* subtract average from the power spectrum
 * -- intend to remove non-tonal signal (such as drums, percussions)
 * INPUT
 *  n : FFT size
 *  p[(n+1)/2] : power spectrum
 *  m : number of bins to average out
 *  factor : factor * average is subtracted from the power
 *           (factor = 0.0) means no subtraction
 *           (factor = 1.0) means full subtraction of the average
 *           (factor = 2.0) means over subtraction
 * OUTPUT
 *  p[(n+1)/2] : subtracted power spectrum
 */
void
power_subtract_ave (int n, float *p, int m, float factor);

/* octave remover
 * INPUT
 *  n : FFT size
 *  p[(n+1)/2] : power spectrum
 *  factor : factor * average is subtracted from the power
 *           (factor = 0.0) means no subtraction
 *           (factor = 1.0) means full subtraction of the average
 *           (factor = 2.0) means over subtraction
 * OUTPUT
 *  p[(n+1)/2] : subtracted power spectrum
 */
void
power_subtract_octave (int n, float *p, float factor);


#endif /* !_FFT_H_ */