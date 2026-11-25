#include "CEmfInterpretatorRender.h"

namespace MetaFile
{
	CEmfInterpretatorRender::CEmfInterpretatorRender(IOutputDevice* pIOutputDevice) 
		: m_pMetaFileRenderer(static_cast<CMetaFileRenderer*>(pIOutputDevice)){}

	InterpretatorType CEmfInterpretatorRender::GetType() const
	{
		return InterpretatorType::Render;
	}
	
	void CEmfInterpretatorRender::CreateConditional(IMetaFileBase* pFile)
	{
		if (nullptr != m_pMetaFileRenderer)
			m_pMetaFileRenderer->CreateConditional(pFile);
	}

	void CEmfInterpretatorRender::ChangeConditional()
	{
		if (nullptr != m_pMetaFileRenderer)
			m_pMetaFileRenderer->ChangeConditional();
	}

	void CEmfInterpretatorRender::Begin()
	{
		if (nullptr != m_pMetaFileRenderer)
			m_pMetaFileRenderer->Begin();
	}

	void CEmfInterpretatorRender::End()
	{
		if (nullptr != m_pMetaFileRenderer)
			m_pMetaFileRenderer->End();
	}

	void CEmfInterpretatorRender::DrawBitmap(double dX, double dY, double dW, double dH, BYTE *pBuffer, unsigned int unWidth, unsigned int unHeight, unsigned int unBlendMode)
	{
		if (nullptr != m_pMetaFileRenderer)
			m_pMetaFileRenderer->DrawBitmap(dX, dY, dW, dH, pBuffer, unWidth, unHeight, unBlendMode);
	}

	void CEmfInterpretatorRender::DrawString(std::wstring &wsText, unsigned int unCharsCount, double dX, double dY, double *pDx,
	                                         int iGraphicsMode, double dXScale, double dYScale, bool bUseGID)
	{
		if (nullptr != m_pMetaFileRenderer)
			m_pMetaFileRenderer->DrawString(wsText, unCharsCount, dX, dY, pDx, iGraphicsMode, dXScale, dYScale, bUseGID);
	}

	void CEmfInterpretatorRender::DrawDriverString(const std::wstring& wsString, const std::vector<TPointD>& arPoints)
	{
		if (nullptr != m_pMetaFileRenderer)
			m_pMetaFileRenderer->DrawDriverString(wsString, arPoints);
	}

	void CEmfInterpretatorRender::StartPath()
	{
		if (nullptr != m_pMetaFileRenderer)
			m_pMetaFileRenderer->StartPath();
	}

	void CEmfInterpretatorRender::MoveTo(double dX, double dY)
	{
		if (nullptr != m_pMetaFileRenderer)
			m_pMetaFileRenderer->MoveTo(dX, dY);
	}

	void CEmfInterpretatorRender::LineTo(double dX, double dY)
	{
		if (nullptr != m_pMetaFileRenderer)
			m_pMetaFileRenderer->LineTo(dX, dY);
	}

	void CEmfInterpretatorRender::CurveTo(double dX1, double dY1, double dX2, double dY2, double dXe, double dYe)
	{
		if (nullptr != m_pMetaFileRenderer)
			m_pMetaFileRenderer->CurveTo(dX1, dY1, dX2, dY2, dXe, dYe);
	}

	void CEmfInterpretatorRender::ArcTo(double dLeft, double dTop, double dRight, double dBottom, double dStartAngle, double dSweepAngle)
	{
		if (nullptr != m_pMetaFileRenderer)
			m_pMetaFileRenderer->ArcTo(dLeft, dTop, dRight, dBottom, dStartAngle, dSweepAngle);
	}

	void CEmfInterpretatorRender::ClosePath()
	{
		if (nullptr != m_pMetaFileRenderer)
			m_pMetaFileRenderer->ClosePath();
	}

	void CEmfInterpretatorRender::DrawPath(int nType)
	{
		if (nullptr != m_pMetaFileRenderer)
			m_pMetaFileRenderer->DrawPath(nType);
	}

	void CEmfInterpretatorRender::EndPath()
	{
		if (nullptr != m_pMetaFileRenderer)
			m_pMetaFileRenderer->EndPath();
	}

	void CEmfInterpretatorRender::ResetClip()
	{
		if (nullptr != m_pMetaFileRenderer)
			m_pMetaFileRenderer->ResetClip();
	}

	void CEmfInterpretatorRender::IntersectClip(const TRectD &oClip)
	{
		if (nullptr != m_pMetaFileRenderer)
			m_pMetaFileRenderer->IntersectClip(oClip);
	}

	void CEmfInterpretatorRender::ExcludeClip(const TRectD &oClip, const TRectD &oBB)
	{
		if (nullptr != m_pMetaFileRenderer)
			m_pMetaFileRenderer->ExcludeClip(oClip, oBB);
	}

	void CEmfInterpretatorRender::PathClip(const CPath &oPath, int nClipMode, TXForm *pTransform)
	{
		if (nullptr != m_pMetaFileRenderer)
			m_pMetaFileRenderer->PathClip(oPath, nClipMode, pTransform);
	}

	void CEmfInterpretatorRender::StartClipPath(unsigned int unMode, int nFillMode)
	{
		if (nullptr != m_pMetaFileRenderer)
			m_pMetaFileRenderer->StartClipPath(unMode, nFillMode);
	}

	void CEmfInterpretatorRender::EndClipPath(unsigned int unMode)
	{
		if (nullptr != m_pMetaFileRenderer)
			m_pMetaFileRenderer->EndClipPath(unMode);
	}

	void CEmfInterpretatorRender::UpdateDC()
	{
		if (nullptr != m_pMetaFileRenderer)
			m_pMetaFileRenderer->UpdateDC();
	}

	void CEmfInterpretatorRender::SetTransform(const double& dM11, const double& dM12, const double& dM21, const double& dM22, const double& dX, const double& dY)
	{
		if (nullptr != m_pMetaFileRenderer)
			m_pMetaFileRenderer->SetTransform(dM11, dM12, dM21, dM22, dX, dY);
	}

	void CEmfInterpretatorRender::GetTransform(double *pdM11, double *pdM12, double *pdM21, double *pdM22, double *pdX, double *pdY)
	{
		if (nullptr != m_pMetaFileRenderer)
			m_pMetaFileRenderer->GetTransform(pdM11, pdM12, pdM21, pdM22, pdX, pdY);
	}

	CMetaFileRenderer *CEmfInterpretatorRender::GetRenderer() const
	{
		return m_pMetaFileRenderer;
	}
	
	double CEmfInterpretatorRender::GetScaleX() const
	{
		if (nullptr == m_pMetaFileRenderer)
			return 1.;

		return m_pMetaFileRenderer->GetScaleX();
	}

	double CEmfInterpretatorRender::GetScaleY() const
	{
		if (nullptr == m_pMetaFileRenderer)
			return 1.;

		return m_pMetaFileRenderer->GetScaleY();
	}

	void CEmfInterpretatorRender::HANDLE_EMR_RESTOREDC(const int & /*nIndexDC*/)
	{
		if (nullptr != m_pMetaFileRenderer)
			m_pMetaFileRenderer->NeedUpdateClip();
	}
	
	void CEmfInterpretatorRender::HANDLE_EMR_SELECTCLIPPATH(const unsigned int & /*unRegionMode*/)
	{
		if (nullptr != m_pMetaFileRenderer)
			m_pMetaFileRenderer->NeedUpdateClip();
	}
	
	void CEmfInterpretatorRender::HANDLE_EMR_EXCLUDECLIPRECT(const TRectL & /*oClip*/)
	{
		if (nullptr != m_pMetaFileRenderer)
			m_pMetaFileRenderer->NeedUpdateClip();
	}
	
	void CEmfInterpretatorRender::HANDLE_EMR_EXTSELECTCLIPRGN(const unsigned int & /*unRgnDataSize*/, const unsigned int & /*unRegionMode*/, CDataStream & /*oDataStream*/)
	{
		if (nullptr != m_pMetaFileRenderer)
			m_pMetaFileRenderer->NeedUpdateClip();
	}
	
	void CEmfInterpretatorRender::HANDLE_EMR_SETMETARGN()
	{
		if (nullptr != m_pMetaFileRenderer)
			m_pMetaFileRenderer->NeedUpdateClip();
	}
	
	void CEmfInterpretatorRender::HANDLE_EMR_INTERSECTCLIPRECT(const TRectL & /*oClip*/)
	{
		if (nullptr != m_pMetaFileRenderer)
			m_pMetaFileRenderer->NeedUpdateClip();
	}
	
	void CEmfInterpretatorRender::HANDLE_EMFPLUS_OFFSETCLIP(double, double)
	{
		if (nullptr != m_pMetaFileRenderer)
			m_pMetaFileRenderer->NeedUpdateClip();
	}
	
	void CEmfInterpretatorRender::HANDLE_EMFPLUS_RESETCLIP()
	{
		if (nullptr != m_pMetaFileRenderer)
			m_pMetaFileRenderer->NeedUpdateClip();
	}
	
	void CEmfInterpretatorRender::HANDLE_EMFPLUS_SETCLIPPATH(short, const CEmfPlusPath *)
	{
		if (nullptr != m_pMetaFileRenderer)
			m_pMetaFileRenderer->NeedUpdateClip();
	}
	
	void CEmfInterpretatorRender::HANDLE_EMFPLUS_SETCLIPRECT(short, const TEmfPlusRectF &)
	{
		if (nullptr != m_pMetaFileRenderer)
			m_pMetaFileRenderer->NeedUpdateClip();
	}
	
	void CEmfInterpretatorRender::HANDLE_EMFPLUS_SETCLIPREGION(short, short, const CEmfPlusRegion *)
	{
		if (nullptr != m_pMetaFileRenderer)
			m_pMetaFileRenderer->NeedUpdateClip();
	}
	
	void CEmfInterpretatorRender::HANDLE_EMFPLUS_ENDOFFILE()
	{
		if (nullptr != m_pMetaFileRenderer)
			m_pMetaFileRenderer->NeedUpdateClip();
	}
	
	void CEmfInterpretatorRender::HANDLE_EMFPLUS_GETDC()
	{
		if (nullptr != m_pMetaFileRenderer)
			m_pMetaFileRenderer->NeedUpdateClip();
	}
	
	void CEmfInterpretatorRender::HANDLE_EMFPLUS_RESTORE(unsigned int)
	{
		if (nullptr != m_pMetaFileRenderer)
			m_pMetaFileRenderer->NeedUpdateClip();
	}
}
