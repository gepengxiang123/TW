#ifndef TW_UTILS_H
#define	TW_UTILS_H

#include "TW.h"

#include <cuda_runtime.h>
#include <cmath>

namespace TW
{
	//!--------------------Inline and template functions----------------------------------------

	/// \brief Return the 1D index of a 2D matrix.
	///
	/// \param x position in x direction in a 2D matrix
	/// \param y position in y direction in a 2D matrix
	/// \param width width (number of cols) of a 2D matrix
	inline __host__ __device__ int_t ELT2D(int_t x, int_t y, int_t width)
	{
		return (y*width + x);
	}

	/// \brief Return the 1D index of a 3D matrix.
	///
	/// \param x position in x direction in a 3D matrix
	/// \param y position in y direction in a 3D matrix
	/// \param z position in z direction in a 3D matrix
	/// \param width width of a 3D matrix
	/// \param height height of a 3D matrix
	inline __host__ __device__ int_t ELT3D(int_t x, int_t y, int_t z, int_t width, int_t height)
	{
		return ((z*height + y)*width + x);
	}

	/// \brief lerp function for t
	///
	/// \param a lower bound 
	/// \param b upper bound
	/// \param lerp parameter
	inline __device__ __host__ real_t lerp(real_t a, real_t b, real_t t)
	{
		return a + t*(b - a);
	}

	/// \brief clamp function
	///
	/// \param a lower bound
	/// \param b upper bound
	/// \param f value to be clamped
	inline __device__ __host__ real_t clamp(real_t f, real_t a, real_t b)
	{
#ifdef TW_USE_DOUBLE
		return fmax(a, fmin(f, b));
#else
		return fmaxf(a, fminf(f, b));
#endif // TW_USE_DOUBL
	}

	/// \brief Template function for computing a block-sum reduction
	///
	/// \param sdata data in the block's shared memory
	/// \param mySum the value to be added on 
	/// \param tid thread id
	template <unsigned int blockSize, class Real>
	__device__
		void reduceBlock(
		volatile Real *sdata, Real mySum, const unsigned int tid)
	{
		sdata[tid] = mySum;
		__syncthreads();

		// do reduction in shared mem
		if (blockSize >= 512){ if (tid < 256) { sdata[tid] = mySum = mySum + sdata[tid + 256]; } __syncthreads(); }
		if (blockSize >= 256){ if (tid < 128) { sdata[tid] = mySum = mySum + sdata[tid + 128]; } __syncthreads(); }
		if (blockSize >= 128){ if (tid < 64) { sdata[tid] = mySum = mySum + sdata[tid + 64]; } __syncthreads(); }

		if (tid < 32)
		{
			if (blockSize >= 64){ sdata[tid] = mySum = mySum + sdata[tid + 32]; }
			if (blockSize >= 32){ sdata[tid] = mySum = mySum + sdata[tid + 16]; }
			if (blockSize >= 16){ sdata[tid] = mySum = mySum + sdata[tid + 8]; }
			if (blockSize >= 8)	{ sdata[tid] = mySum = mySum + sdata[tid + 4]; }
			if (blockSize >= 4)	{ sdata[tid] = mySum = mySum + sdata[tid + 2]; }
			if (blockSize >= 2) { sdata[tid] = mySum = mySum + sdata[tid + 1]; }
		}
		__syncthreads();
	}

	/// \brief Template function for computing a block-max reduction
	///
	/// \param sdata data in the block's shared memory
	/// \param sindex index of the corresponding data in shared memory
	/// \param data initial data
	/// \param ind initial index
	/// \param tid thread id
	template <unsigned int blockSize, class Real>
	__device__
		void reduceToMaxBlock(
		volatile Real *sdata, volatile int *sindex,
		Real data, int ind, const unsigned int tid)
	{

		// do reduction in shared mem
		sdata[tid] = data;
		sindex[tid] = ind;
		__syncthreads();

		if (blockSize >= 512){ if (tid < 256){ if (sdata[tid] < sdata[tid + 256]){ sdata[tid] = sdata[tid + 256]; sindex[tid] = sindex[tid + 256]; } } __syncthreads(); }
		if (blockSize >= 256){ if (tid < 128){ if (sdata[tid] < sdata[tid + 128]){ sdata[tid] = sdata[tid + 128]; sindex[tid] = sindex[tid + 128]; } } __syncthreads(); }
		if (blockSize >= 128){ if (tid < 64) { if (sdata[tid] < sdata[tid + 64]){ sdata[tid] = sdata[tid + 64]; sindex[tid] = sindex[tid + 64]; } } __syncthreads(); }

		if (tid < 32)
		{
			if (blockSize >= 64) { if (sdata[tid] < sdata[tid + 32]){ sdata[tid] = sdata[tid + 32]; sindex[tid] = sindex[tid + 32]; } }
			if (blockSize >= 32) { if (sdata[tid] < sdata[tid + 16]){ sdata[tid] = sdata[tid + 16]; sindex[tid] = sindex[tid + 16]; } }
			if (blockSize >= 16) { if (sdata[tid] < sdata[tid + 8]){ sdata[tid] = sdata[tid + 8]; sindex[tid] = sindex[tid + 8]; } }
			if (blockSize >= 8) { if (sdata[tid] < sdata[tid + 4]){ sdata[tid] = sdata[tid + 4]; sindex[tid] = sindex[tid + 4]; } }
			if (blockSize >= 4)	{ if (sdata[tid] < sdata[tid + 2]){ sdata[tid] = sdata[tid + 2]; sindex[tid] = sindex[tid + 2]; } }
			if (blockSize >= 2) { if (sdata[tid] < sdata[tid + 1]){ sdata[tid] = sdata[tid + 1]; sindex[tid] = sindex[tid + 1]; } }
		}
		__syncthreads();
	}

	/// \brief Function for computing the CUFFT complex number scaling operation
	/// 
	/// \param a the input complex number
	/// \param s the scaling factor
	/// \return the scaled new complex number
	static __device__ __host__
		inline cudafftComplex ComplexScale(cudafftComplex a, real_t s)
	{
		cudafftComplex c;
		c.x = s * a.x;
		c.y = s * a.y;
		return c;
	}
	
	/// \brief Function for computing the CUFFT complex number multiplication
	/// 
	/// \param a the input complex number
	/// \param b another input complex number
	/// \return the result of the multiplication
	static __device__ __host__
		inline cudafftComplex ComplexMul(cudafftComplex a, cudafftComplex b)
	{
		cudafftComplex c;
		c.x = a.x * b.x + a.y * b.y;
		c.y = a.x * b.y - a.y * b.x;
		return c;
	}

	// ----------------------Inline and Template Functions End-------------------------!


	// !---------------------Host Utility Functions -------------------------------------

	/// \brief Function to compute positions of POIs on single-core CPU.
	/// No need to pre-allocate memory for the pointer.
	///
	/// \param Out_h_iPXY position array in device memory
	/// \param iNumberX number of POIs in x direction
	/// \param iNumberY number of POIs in y direction
	/// \param iMarginX number of extra safe pixels at ROI boundary in x direction
	/// \param iMarginY number of extra safe pixels at ROI boundary in y direction
	/// \param iSubsetX half size of the square subset in x direction
	/// \param iSubsetY half size of the square subset in y direction
	/// \param iGridSpaceX number of pixels between two POIs in x direction
	/// \param iGirdSpaceY number of pixels between two POIs in y direction
	TW_LIB_DLL_EXPORTS void ComputePOIPositions_s(
		// Output
		int_t *&Out_h_iPXY,						// Return the host handle
		// Inputs
		int_t iNumberX, int_t iNumberY,
		int_t iMarginX, int_t iMarginY,
		int_t iSubsetX, int_t iSubsetY,
		int_t iGridSpaceX, int_t iGridSpaceY);

	/// \brief Function to compute positions of POIs on multi-core CPU.
	/// No need to pre-allocate memory for the pointer.
	///
	/// \param Out_h_iPXY position array in device memory
	/// \param iNumberX number of POIs in x direction
	/// \param iNumberY number of POIs in y direction
	/// \param iMarginX number of extra safe pixels at ROI boundary in x direction
	/// \param iMarginY number of extra safe pixels at ROI boundary in y direction
	/// \param iSubsetX half size of the square subset in x direction
	/// \param iSubsetY half size of the square subset in y direction
	/// \param iGridSpaceX number of pixels between two POIs in x direction
	/// \param iGirdSpaceY number of pixels between two POIs in y direction
	TW_LIB_DLL_EXPORTS void ComputePOIPositions_m(
		// Output
		int_t *&Out_h_iPXY,						// Return the host handle
		// Inputs
		int_t iNumberX, int_t iNumberY,
		int_t iMarginX, int_t iMarginY,
		int_t iSubsetX, int_t iSubsetY,
		int_t iGridSpaceX, int_t iGridSpaceY);

	// ---------------------------CPU Utility Functions End-------------------------------!

	// !----------------------------GPU Wrapper Functions----------------------------------

	/// \brief Function to compute positions of POIs on GPU, the result is stored in 
	/// an device array. NOTE: No need to pre-allocate memory for the device pointer
	///
	/// \param Out_d_iPXY position array in device memory
	/// \param iNumberX number of POIs in x direction
	/// \param iNumberY number of POIs in y direction
	/// \param iMarginX number of extra safe pixels at ROI boundary in x direction
	/// \param iMarginY number of extra safe pixels at ROI boundary in y direction
	/// \param iSubsetX half size of the square subset in x direction
	/// \param iSubsetY half size of the square subset in y direction
	/// \param iGridSpaceX number of pixels between two POIs in x direction
	/// \param iGirdSpaceY number of pixels between two POIs in y direction
	TW_LIB_DLL_EXPORTS void cuComputePOIPostions(
		// Output
		int_t *&Out_d_iPXY,						// Return the device handle
		// Inputs
		int_t iNumberX, int_t iNumberY,
		int_t iMarginX, int_t iMarginY,
		int_t iSubsetX, int_t iSubsetY,
		int_t iGridSpaceX, int_t iGridSpaceY);

	/// \brief Function to compute positions of POIs on GPU, the result is stored both in 
	/// an device array and a host array. NOTE: No need to pre-allocate memory for the two pointers.
	///
	/// \param Out_d_iPXY position array in device memory
	/// \param Out_h_iPXY position array in host memory
	/// \param iNumberX number of POIs in x direction
	/// \param iNumberY number of POIs in y direction
	/// \param iMarginX number of extra safe pixels at ROI boundary in x direction
	/// \param iMarginY number of extra safe pixels at ROI boundary in y direction
	/// \param iSubsetX half size of the square subset in x direction
	/// \param iSubsetY half size of the square subset in y direction
	/// \param iGridSpaceX number of pixels between two POIs in x direction
	/// \param iGirdSpaceY number of pixels between two POIs in y direction
	TW_LIB_DLL_EXPORTS void cuComputePOIPostions(
		// Outputs
		int_t *&Out_d_iPXY,						// Return the device handle
		int_t *&Out_h_iPXY,						// Retrun the host handle
		// Inputs
		int_t iNumberX, int_t iNumberY,
		int_t iMarginX, int_t iMarginY,
		int_t iSubsetX, int_t iSubsetY,
		int_t iGridSpaceX, int_t iGridSpaceY);



	// -----------------------------------GPU Wrapper Functions End-----------------------------!

} // namespace TW


#endif // !UTILS_H