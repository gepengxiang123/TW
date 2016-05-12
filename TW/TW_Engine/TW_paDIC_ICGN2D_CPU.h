#ifndef TW_paDIC_ICGN2D_CPU_H
#define TW_paDIC_ICGN2D_CPU_H

#include "opencv2\opencv.hpp"
#include "TW.h"
#include "TW_paDIC_ICGN2D.h"


namespace TW{
namespace paDIC{

/// \brief ICGN2D implementation based on CPU computation
class TW_LIB_DLL_EXPORTS ICGN2D_CPU : public ICGN2D
{
public:
	ICGN2D_CPU(const cv::Mat& refImg,
			   const cv::Mat& tarImg,
			   int iStartX, int iStartY,
			   int iROIWidth, int iROIHeight,
			   int iSubsetX, int iSubsetY,
			   int iNumberX, int iNumberY,
		       int iNumIterations,
		       real_t fDeltaP);
	~ICGN2D_CPU();

	virtual void ICGN2D_Precomputation_Prepare() override;
	virtual void ICGN2D_Precomputation() override;
	virtual void ICGN2D_Precomputation_Finalize() override;

	///\brief Allocate required memory for the ICNG2D application
	void ICGN2D_Prepare();

	///\brief Core ICGN2D algorithm. The calculation unit is subset.
	///
	///\param fU Initial guees for the displacement in U(x) direction
	///\param fV Initial guees for the displacement in V(Y) direction
	///\param iPOIx POI x-position 
	///\param iPOIy POI y-position
	///\param id the id of the current POI being processed (For multi-threaded computation)
	ICGN2DFlag ICGN2D_Compute(real_t &fU,
							  real_t &fV,
							  int &iNumIterations,
							  const int iPOIx,
							  const int iPOIy,
							  const int id);
	void ICGN2D_Finalize();

private:
	real_t **m_fRx;						// x-derivative of reference image ROI_H * ROI_W
	real_t **m_fRy;						// y-derivative of reference image ROI_H * ROI_W
	real_t ****m_fBsplineInterpolation;	// ROI_H * ROI_W * 4 * 4
	real_t ***m_fSubsetR;				// POI_N * Subset_H * Subset_W
	real_t ***m_fSubsetT;				// POI_N * Subset_H * Subset_W
	real_t ****m_fRDescent;				// POI_N * Subset_H * Subset_W * 6;
};

}
}

#endif // !TW_paDIC_ICGN2D_CPU_H
