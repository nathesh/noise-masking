/* half-complex format routines
 * Copyright (C) 2007 Kengo Ichiki <kichiki@users.sourceforge.net>
 * $Id: hc.c,v 1.7 2007/10/22 04:26:38 kichiki Exp $
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#include <math.h>
#include <stdlib.h> // malloc()
#include <fftw3.h>
#include <stdio.h> // fprintf()
#include "memory-check.h" // CHECK_MALLOC() macro



/* return power (amp2) of the complex number (freq(k),freq(len-k));
 * where (real,imag) = (cos(angle), sin(angle)).
 * INPUT
 *  len        :
 *  freq [len] :
 *  scale      : scale factor for amp2[]
 * OUTPUT
 *  amp2 [len/2+1] := (real^2 + imag^2) / scale
 */
void HC_to_amp2 (int len, fftw_complex *freq, float scale,
		 float* amp2)
{
  int i;
  float rl, im;
  for (i = 0; i < (len+1)/2; i ++)
    {
      rl = freq[i][0];
      im = freq[i][1];
      amp2 [i] = ((rl * rl + im * im) / len);
     // amp2 [i] = 10*log10((rl * rl + im * im) / len);
    //  printf("%f\n",amp2[i]);
    }
  if (len%2 == 0)
    {
      amp2[len/2] = (freq[len/2][0]* freq[len/2][0] / len);
      //amp2[len/2] = 10*log10(freq[len/2][0]* freq[len/2][0] / len);
    }
}
