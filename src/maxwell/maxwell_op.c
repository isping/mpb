/* Copyright (C) 1999, 2000 Massachusetts Institute of Technology.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "../config.h"
#include <check.h>

#include "maxwell.h"

/* assign a = v going from transverse to cartesian coordinates.  
   Here, a = (a[0],a[1],a[2]) is in cartesian coordinates.
   (v[0],v[vstride]) is in the transverse basis of k.m and k.n. */
static void assign_t2c(scalar *a, const k_data k,
		       const scalar *v, int vstride)
{
     scalar v0 = v[0], v1 = v[vstride];

     ASSIGN_SCALAR(a[0],
		   SCALAR_RE(v0)*k.mx + SCALAR_RE(v1)*k.nx,
		   SCALAR_IM(v0)*k.mx - SCALAR_IM(v1)*k.nx);
     ASSIGN_SCALAR(a[1],
		   SCALAR_RE(v0)*k.my + SCALAR_RE(v1)*k.ny,
		   SCALAR_IM(v0)*k.my - SCALAR_IM(v1)*k.ny);
     ASSIGN_SCALAR(a[2],
		   SCALAR_RE(v0)*k.mz + SCALAR_RE(v1)*k.nz,
		   SCALAR_IM(v0)*k.mz - SCALAR_IM(v1)*k.nz);
}

/* assign a = k x v (cross product), going from transverse to
   cartesian coordinates.
  
   Here, a = (a[0],a[1],a[2]) and k = (k.kx,k.ky,k.kz) are in
   cartesian coordinates.  (v[0],v[vstride]) is in the transverse basis of
   k.m and k.n. */
static void assign_cross_t2c(scalar *a, const k_data k,
			     const scalar *v, int vstride)
{
     scalar v0 = v[0], v1 = v[vstride];

     /* Note that k x m = |k| n, k x n = - |k| m.  Therefore,
        k x v = k x (v0 m + v1 n) = (v0 n - v1 m) * |k|. */

     ASSIGN_SCALAR(a[0],
		   (SCALAR_RE(v0)*k.nx - SCALAR_RE(v1)*k.mx) * k.kmag,
		   (SCALAR_IM(v0)*k.nx - SCALAR_IM(v1)*k.mx) * k.kmag);
     ASSIGN_SCALAR(a[1],
		   (SCALAR_RE(v0)*k.ny - SCALAR_RE(v1)*k.my) * k.kmag,
		   (SCALAR_IM(v0)*k.ny - SCALAR_IM(v1)*k.my) * k.kmag);
     ASSIGN_SCALAR(a[2],
		   (SCALAR_RE(v0)*k.nz - SCALAR_RE(v1)*k.mz) * k.kmag,
		   (SCALAR_IM(v0)*k.nz - SCALAR_IM(v1)*k.mz) * k.kmag);

#ifdef DEBUG
     {
	  real num;
	  num = SCALAR_NORMSQR(a[0])+SCALAR_NORMSQR(a[1])+SCALAR_NORMSQR(a[2]);
	  CHECK(!BADNUM(num), "yikes, crazy number!");
     }
#endif
}

/* assign v = scale * k x a (cross product), going from cartesian to
   transverse coordinates.
  
   Here, a = (a[0],a[1],a[2]) and k = (k.kx,k.ky,k.kz) are in
   cartesian coordinates.  (v[0],v[vstride]) is in the transverse basis of
   k.m and k.n. */
static void assign_cross_c2t(scalar *v, int vstride,
			     const k_data k, const scalar *a,
			     real scale)
{
     scalar a0 = a[0], a1 = a[1], a2 = a[2];
     scalar at0, at1;

     /* First, compute at0 = a*m and at1 = a*n.  (Components of a that
	are parallel to k are killed anyway by the cross product.) */

     ASSIGN_SCALAR(at0,
          SCALAR_RE(a0)*k.mx + SCALAR_RE(a1)*k.my + SCALAR_RE(a2)*k.mz,
	  SCALAR_IM(a0)*k.mx + SCALAR_IM(a1)*k.my + SCALAR_IM(a2)*k.mz);
     ASSIGN_SCALAR(at1,
          SCALAR_RE(a0)*k.nx + SCALAR_RE(a1)*k.ny + SCALAR_RE(a2)*k.nz,
	  SCALAR_IM(a0)*k.nx + SCALAR_IM(a1)*k.ny + SCALAR_IM(a2)*k.nz);

     /* Now, k x a = k x (at0*m + at1*n) = (at0*n - at1*m) * |k|. */

     scale *= k.kmag;  /* combine scale factor and |k|*/
     ASSIGN_SCALAR(v[0],
		   - scale * SCALAR_RE(at1),
		   - scale * SCALAR_IM(at1));
     ASSIGN_SCALAR(v[vstride],
		   scale * SCALAR_RE(at0),
		   scale * SCALAR_IM(at0));

#ifdef DEBUG
     {
	  real dummy = SCALAR_NORMSQR(v[0]) + SCALAR_NORMSQR(v[vstride]);
	  CHECK(!BADNUM(dummy), "yikes, crazy number!");
     }
#endif
}

/* compute a = u x v, where a and u are in cartesian coordinates and
   v is in transverse coordinates. */
static void assign_ucross_t2c(scalar *a, const real u[3], const k_data k,
			     const scalar *v, int vstride)
{
     scalar v0 = v[0], v1 = v[vstride];
     real vx_r, vx_i, vy_r;
#ifdef SCALAR_COMPLEX
     real vy_i, vz_r, vz_i;
#endif

     /* Note that v = (vx,vy,vz) = (v0 m + v1 n). */

     vx_r = SCALAR_RE(v0)*k.mx + SCALAR_RE(v1)*k.nx;
     vy_r = SCALAR_RE(v0)*k.my + SCALAR_RE(v1)*k.ny;
     vz_r = SCALAR_RE(v0)*k.mz + SCALAR_RE(v1)*k.nz;

#ifdef SCALAR_COMPLEX
     vx_i = SCALAR_IM(v0)*k.mx + SCALAR_IM(v1)*k.nx;
     vy_i = SCALAR_IM(v0)*k.my + SCALAR_IM(v1)*k.ny;
     vz_i = SCALAR_IM(v0)*k.mz + SCALAR_IM(v1)*k.nz;
#endif

     ASSIGN_SCALAR(a[0],
		   u[1] * vz_r - u[2] * vy_r,
		   u[1] * vz_i - u[2] * vy_i);
     ASSIGN_SCALAR(a[1],
		   u[2] * vx_r - u[0] * vz_r,
		   u[2] * vx_i - u[0] * vz_i);
     ASSIGN_SCALAR(a[2],
		   u[0] * vy_r - u[1] * vx_r,
		   u[0] * vy_i - u[1] * vx_i);
}

void maxwell_compute_fft(int dir, maxwell_data *d, scalar *array, 
			 int howmany, int stride, int dist)
{
#ifdef HAVE_FFTW

#  ifdef SCALAR_COMPLEX

#    ifndef HAVE_MPI

     fftwnd(dir < 0 ? d->plan : d->iplan,
	    howmany,
	    (fftw_complex *) array, stride, dist,
	    0, 0, 0);

#    else /* HAVE_MPI */

     CHECK(dist == howmany && stride == 1,
	   "weird strides and dists don't work with fftwnd_mpi");

     fftwnd_mpi(dir < 0 ? d->plan : d->iplan,
		howmany,
		(fftw_complex *) array);

#    endif /* HAVE_MPI */

#  else /* not SCALAR_COMPLEX */

#    ifndef HAVE_MPI

     if (dir < 0)
	  rfftwnd_real_to_complex(d->plan,
				  howmany,
				  (fftw_real *) array, stride, dist,
				  0, 0, 0);
     else
	  rfftwnd_complex_to_real(d->iplan,
				  howmany,
				  (fftw_complex *) array, stride, dist,
				  0, 0, 0);

#    else /* HAVE_MPI */

#      error rfftwnd_mpi not supported yet.

#    endif /* HAVE_MPI */

#  endif /* not SCALAR_COMPLEX */

#else /* not HAVE_FFTW */
#  error only FFTW ffts are supported
#endif /* not HAVE_FFTW */
}

/* assigns newv = matrix * oldv.  matrix is symmetric and so is stored
   in "packed" format. */
void assign_symmatrix_vector(scalar_complex *newv,
			     const symmetric_matrix matrix,
			     const scalar_complex *oldv)
{
     scalar_complex v0 = oldv[0], v1 = oldv[1], v2 = oldv[2];

     newv[0].re = matrix.m00 * v0.re + matrix.m01 * v1.re + matrix.m02 * v2.re;
     newv[0].im = matrix.m00 * v0.im + matrix.m01 * v1.im + matrix.m02 * v2.im;

     newv[1].re = matrix.m01 * v0.re + matrix.m11 * v1.re + matrix.m12 * v2.re;
     newv[1].im = matrix.m01 * v0.im + matrix.m11 * v1.im + matrix.m12 * v2.im;

     newv[2].re = matrix.m02 * v0.re + matrix.m12 * v1.re + matrix.m22 * v2.re;
     newv[2].im = matrix.m02 * v0.im + matrix.m12 * v1.im + matrix.m22 * v2.im;

#ifdef DEBUG
     {
	  real dummy;
	  dummy = SCALAR_NORMSQR(newv[0]) + SCALAR_NORMSQR(newv[1])
	       + SCALAR_NORMSQR(newv[2]);
	  CHECK(!BADNUM(dummy), "yikes, crazy number!");
     }
#endif
}

/* compute the D field in position space from Hin, which holds the H
   field in Fourier space, for the specified bands; this amounts to
   taking the curl and then Fourier transforming.  The output array,
   dfield, is localN x cur_num_bands x 3, where localN is the local
   spatial indices and 3 is the field components. */
void maxwell_compute_d_from_H(maxwell_data *d, evectmatrix Hin, 
			      scalar_complex *dfield,
			      int cur_band_start, int cur_num_bands)
{
     scalar *fft_data = (scalar *) dfield;
     int i, j, b;

     CHECK(Hin.c == 2, "fields don't have 2 components!");
     CHECK(d, "null maxwell data pointer!");
     CHECK(dfield, "null field output data!");
     CHECK(cur_band_start >= 0 && cur_band_start + cur_num_bands <= Hin.p,
	   "invalid range of bands for computing fields");

     /* first, compute fft_data = curl(Hin): */
     for (i = 0; i < d->other_dims; ++i)
	  for (j = 0; j < d->last_dim; ++j) {
	       int ij = i * d->last_dim + j;
	       int ij2 = i * d->last_dim_size + j;
	       k_data cur_k = d->k_plus_G[ij];
	       
	       for (b = 0; b < cur_num_bands; ++b)
		    assign_cross_t2c(&fft_data[3 * (ij2*cur_num_bands 
						    + b)], 
				     cur_k, 
				     &Hin.data[ij * 2 * Hin.p + 
					      b + cur_band_start],
				     Hin.p);
	  }

     /* now, convert to position space via FFT: */
     maxwell_compute_fft(+1, d, fft_data,
			 cur_num_bands*3, cur_num_bands*3, 1);
}

/* Compute E (output in dfield) from D (input in dfield); this amounts
   to just dividing by the dielectric tensor.  dfield is in position
   space and corresponds to the output from maxwell_compute_d_from_H,
   above. */
void maxwell_compute_e_from_d(maxwell_data *d,
			      scalar_complex *dfield,
			      int cur_num_bands)
{
     int i, b;

     CHECK(d, "null maxwell data pointer!");
     CHECK(dfield, "null field input/output data!");

     for (i = 0; i < d->fft_output_size; ++i) {
	  symmetric_matrix eps_inv = d->eps_inv[i];
	  for (b = 0; b < cur_num_bands; ++b) {
	       int ib = 3 * (i * cur_num_bands + b);
	       assign_symmatrix_vector(&dfield[ib], eps_inv, &dfield[ib]);
	  }
     }	  
}

/* Compute the magnetic (H) field in Fourier space from the electric
   field (e) in position space; this amouns to Fourier transforming and
   then taking the curl.  Also, multiply by scale.  Other
   parameters are as in compute_d_from_H. */
void maxwell_compute_H_from_e(maxwell_data *d, evectmatrix Hout, 
			      scalar_complex *efield,
			      int cur_band_start, int cur_num_bands,
			      real scale)
{
     scalar *fft_data = (scalar *) efield;
     int i, j, b;

     CHECK(Hout.c == 2, "fields don't have 2 components!");
     CHECK(d, "null maxwell data pointer!");
     CHECK(efield, "null field output data!");
     CHECK(cur_band_start >= 0 && cur_band_start + cur_num_bands <= Hout.p,
	   "invalid range of bands for computing fields");

     /* convert back to Fourier space */
     maxwell_compute_fft(-1, d, fft_data,
			 cur_num_bands*3, cur_num_bands*3, 1);
     
     /* then, compute Hout = curl(fft_data) (* scale factor): */
     
     for (i = 0; i < d->other_dims; ++i)
	  for (j = 0; j < d->last_dim; ++j) {
	       int ij = i * d->last_dim + j;
	       int ij2 = i * d->last_dim_size + j;
	       k_data cur_k = d->k_plus_G[ij];
	       
	       for (b = 0; b < cur_num_bands; ++b)
		    assign_cross_c2t(&Hout.data[ij * 2 * Hout.p + 
					       b + cur_band_start],
				     Hout.p, cur_k, 
				     &fft_data[3 * (ij2*cur_num_bands + b)],
				     scale);
	  }
}


/* Compute H field in position space from Hin.  Parameters and output
   formats are the same as for compute_d_from_H, above. */
void maxwell_compute_h_from_H(maxwell_data *d, evectmatrix Hin, 
			      scalar_complex *hfield,
			      int cur_band_start, int cur_num_bands)
{
     scalar *fft_data = (scalar *) hfield;
     int i, j, b;

     CHECK(Hin.c == 2, "fields don't have 2 components!");
     CHECK(d, "null maxwell data pointer!");
     CHECK(hfield, "null field output data!");
     CHECK(cur_band_start >= 0 && cur_band_start + cur_num_bands <= Hin.p,
	   "invalid range of bands for computing fields");

     /* first, compute fft_data = Hin, with the vector field converted 
	from transverse to cartesian basis: */
     for (i = 0; i < d->other_dims; ++i)
	  for (j = 0; j < d->last_dim; ++j) {
	       int ij = i * d->last_dim + j;
	       int ij2 = i * d->last_dim_size + j;
               k_data cur_k = d->k_plus_G[ij];
	       
	       for (b = 0; b < cur_num_bands; ++b)
		    assign_t2c(&fft_data[3 * (ij2*cur_num_bands 
					      + b)], 
			       cur_k,
			       &Hin.data[ij * 2 * Hin.p + 
					b + cur_band_start],
			       Hin.p);
	  }

     /* now, convert to position space via FFT: */
     maxwell_compute_fft(+1, d, fft_data,
			 cur_num_bands*3, cur_num_bands*3, 1);
}


#define MIN2(a,b) ((a) < (b) ? (a) : (b))

/* Compute Xout = curl(1/epsilon * curl(Xin)) */
void maxwell_operator(evectmatrix Xin, evectmatrix Xout, void *data,
		      int is_current_eigenvector, evectmatrix Work)
{
     maxwell_data *d = (maxwell_data *) data;
     int cur_band_start;
     scalar_complex *cdata;
     real scale;
     
     CHECK(d, "null maxwell data pointer!");
     CHECK(Xin.c == 2, "fields don't have 2 components!");

     cdata = (scalar_complex *) d->fft_data;
     scale = -1.0 / Xout.N;  /* scale factor to normalize FFT; 
				negative sign comes from 2 i's from curls */

     /* compute the operator, num_fft_bands at a time: */
     for (cur_band_start = 0; cur_band_start < Xin.p; 
	  cur_band_start += d->num_fft_bands) {
	  int cur_num_bands = MIN2(d->num_fft_bands, Xin.p - cur_band_start);

	  maxwell_compute_d_from_H(d, Xin, cdata,
				   cur_band_start, cur_num_bands);
	  maxwell_compute_e_from_d(d, cdata, cur_num_bands);
	  maxwell_compute_H_from_e(d, Xout, cdata,
				   cur_band_start, cur_num_bands, scale);
     }
}

void maxwell_target_operator(evectmatrix Xin, evectmatrix Xout, void *data,
			     int is_current_eigenvector, evectmatrix Work)
{
     maxwell_target_data *d = (maxwell_target_data *) data;
     real omega_sqr = d->target_frequency * d->target_frequency;

     CHECK(Work.data && Work.data != Xin.data && Work.data != Xout.data,
	   "maxwell_target_operator must have distinct workspace!");

     maxwell_operator(Xin, Work, d->d, is_current_eigenvector, Xout);
     evectmatrix_aXpbY(1.0, Work, -omega_sqr, Xin);

     /* note that maxwell_operator() doesn't actually need the
	workspace (it can operate in-place, for that matter), so we
	can safely pass Work here for the scratch parameter: */
     maxwell_operator(Work, Xout, d->d, 0, Work);

     evectmatrix_aXpbY(1.0, Xout, -omega_sqr, Work);
}

/* Compute the operation Xout = curl 1/epsilon * i u x Xin, which 
   is useful operation in computing the group velocity (derivative
   of the maxwell operator).  u is a vector in cartesian coordinates. */
void maxwell_ucross_op(evectmatrix Xin, evectmatrix Xout,
		       maxwell_data *d, const real u[3])
{
     scalar *fft_data;
     scalar_complex *cdata;
     real scale;
     int cur_band_start;
     int i, j, b;

     CHECK(d, "null maxwell data pointer!");
     CHECK(Xin.c == 2, "fields don't have 2 components!");

     cdata = (scalar_complex *) (fft_data = d->fft_data);
     scale = -1.0 / Xout.N;  /* scale factor to normalize FFT;
                                negative sign comes from 2 i's from curls */

     /* compute the operator, num_fft_bands at a time: */
     for (cur_band_start = 0; cur_band_start < Xin.p;
          cur_band_start += d->num_fft_bands) {
          int cur_num_bands = MIN2(d->num_fft_bands, Xin.p - cur_band_start);
	  
	  /* first, compute fft_data = curl(Xin): */
	  for (i = 0; i < d->other_dims; ++i)
	       for (j = 0; j < d->last_dim; ++j) {
		    int ij = i * d->last_dim + j;
		    int ij2 = i * d->last_dim_size + j;
		    k_data cur_k = d->k_plus_G[ij];
		    
		    for (b = 0; b < cur_num_bands; ++b)
			 assign_ucross_t2c(&fft_data[3 * (ij2*cur_num_bands
							  + b)], 
					   u, cur_k, 
					   &Xin.data[ij * 2 * Xin.p + 
						    b + cur_band_start],
					   Xin.p);
	       }
	  
	  /* now, convert to position space via FFT: */
	  maxwell_compute_fft(+1, d, fft_data,
			      cur_num_bands*3, cur_num_bands*3, 1);
	  
          maxwell_compute_e_from_d(d, cdata, cur_num_bands);
          maxwell_compute_H_from_e(d, Xout, cdata,
                                   cur_band_start, cur_num_bands, scale);
     }
}
