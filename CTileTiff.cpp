#include "CTileTiff.h"

#include <cmath>
#include <iostream>
using namespace std;

CTileTiff::CTileTiff()
{
	m_nTileHeight = 256;
	m_nTileWidth = 256;
}
CTileTiff::~CTileTiff()
{
	TIFFClose(m_pFile);
}

CTileTiff::CTileTiff(std::string fileName)
{
	m_fileName = fileName;
	m_pFile = TIFFOpen(fileName.c_str(), "w");
	if (NULL == m_pFile)
	{
		return;
	}

}

bool CTileTiff::SetTileInfo(int nTileW, int nTileH, int nLayer,int nHeight,int nWidth)
{
	m_nTileWidth = nTileW;
	m_nTileHeight = nTileH;
	m_nLayers = nLayer;
	m_nWidth = nWidth;
	m_nHeight = nHeight;

	//for (int i = 0; i < m_nLayers; i++)
	//{
	//	nLayer = i;
	//	TIFFSetDirectory(m_pFile, nLayer);
	//	TIFFSetField(m_pFile, TIFFTAG_TILEWIDTH, nTileW);
	//	TIFFSetField(m_pFile, TIFFTAG_TILELENGTH, nTileH);
	//	TIFFSetField(m_pFile, TIFFTAG_SUBFILETYPE, FILETYPE_REDUCEDIMAGE);
	//	TIFFSetField(m_pFile, TIFFTAG_BITSPERSAMPLE, 8);
	//	TIFFSetField(m_pFile, TIFFTAG_SAMPLESPERPIXEL, 3);
	//	TIFFSetField(m_pFile, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
	//	TIFFSetField(m_pFile, TIFFTAG_COMPRESSION, COMPRESSION_JPEG);
	//	TIFFSetField(m_pFile, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
	//	int nJpegQuality = 60;
	//	TIFFSetField(m_pFile, TIFFTAG_JPEGQUALITY, nJpegQuality);

	//	//根据需要设置分辨率
	//	/*if (nLayer == 0)
	//	{
	//		float fPixelSize = 0.00068f;
	//		int nScale = 20;
	//		fPixelSize = fPixelSize * 100 / max(1, nScale);
	//		TIFFSetField(m_pFile, TIFFTAG_XRESOLUTION, 1.0 / (fPixelSize / 10));
	//		TIFFSetField(m_pFile, TIFFTAG_YRESOLUTION, 1.0 / (fPixelSize / 10));
	//		TIFFSetField(m_pFile, TIFFTAG_RESOLUTIONUNIT, 3);
	//	}*/
	//}

	return true;
}




#include <opencv2/opencv.hpp>


using namespace cv;

bool ResizeImg(const uchar* pucImgIn, uchar* pucImgOut,
	const int nWidth, const int nHeight, const int nChannels, const int nPitch,
	const int nWidthDst, const int nHeightDst, const int nChannelsDst, const int nPitchDst)
{
	if (NULL == pucImgIn || NULL == pucImgOut
		|| (1 != nChannels && 3 != nChannels && 4 != nChannels)
		|| (1 != nChannelsDst && 3 != nChannelsDst && 4 != nChannelsDst)
		|| nChannels != nChannelsDst
		|| 0 == nWidth * nHeight
		|| 0 == nWidthDst * nHeightDst)
	{
		return false;
	}

	Mat src(nHeight, nWidth, CV_8UC(nChannels), (void*)pucImgIn, nPitch);
	Mat dst(nHeightDst, nWidthDst, CV_8UC(nChannelsDst), (void*)pucImgOut, nPitchDst);
	resize(src, dst, Size(nWidthDst, nHeightDst), 0.0, 0.0);

	return true;
}


bool CTileTiff::SaveImage(cv::Mat &img, const int nLeft, const int nTop,
	const int nRight, const int nBottom)
{
	int nX = nLeft, nY = nTop;

	//int nWidth = nRight - nX, nHeight = nBottom - nY;
	const int nRows = img.rows / m_nTileHeight;
	const int nCols = img.cols / m_nTileWidth;
	cout << "nRows,nCols:" << nRows << "," << nCols << endl;

	const int nPitch = (nRight - nLeft) * 3;
	const int nLayerID = 0;

	for (int i = 0; i < nRows; ++i)
	{
		const int nT = nY + i * m_nTileHeight;
		const int nB = nT + m_nTileWidth;
		for (int j = 0; j < nCols; ++j)
		{
			const int nL = nX + j * m_nTileWidth;
			const int nR = nL + m_nTileWidth;
			cout << nT << "," << nL  << endl;

			cv::Rect rect(j * m_nTileWidth, i * m_nTileHeight, m_nTileWidth, m_nTileHeight);

			cv::Mat roi = img(rect).clone();


			bool bOk = saveTile(roi, nL, nT, nR, nB, nLayerID);
			if (!bOk)
			{
				continue;
			}
		}
	}

	int nSubW = nCols, nSubH = nRows;
	int nSubLayer = nLayerID;
	while (true)
	{
		nSubW /= 2;
		nSubH /= 2;
		nSubLayer++;
		if (1 > nSubW || 1 > nSubH)
		{
			break;
		}

		const int nScale = 1 << nSubLayer;
		for (int i = 0; i < nSubH; ++i)
		{
			const int nT = nY + i * m_nTileWidth * nScale;
			const int nB = nT + m_nTileHeight * nScale;
			for (int j = 0; j < nSubW; ++j)
			{
				const int nL = nX + j * m_nTileWidth * nScale;
				const int nR = nL + m_nTileWidth * nScale;
       


				cv::Rect rect(nL - nX, nT - nY, m_nTileWidth * nScale, m_nTileHeight * nScale);

				cv::Mat roi = img(rect).clone();

				cv::Mat resized;

				ResizeImg(roi.data,
					resized.data, m_nTileWidth * nScale, m_nTileHeight * nScale, 3, nPitch, m_nTileWidth, m_nTileHeight,
					3, m_nTileWidth*3);

				
				bool bOk = saveTile(resized, nL, nT, nR, nB, nSubLayer);
				if (!bOk)
				{
					continue;
				}
			}
		}
	}
	

	return true;
}

bool CTileTiff::saveTile(cv::Mat& roi, const int nL, const int nT,const int nR, const int nB, const int nLayer)
{
	if (roi.empty())
	{
		return false;
	}
	const int nWidth = nR - nL;
	const int nHeight = nB - nT;
	int nLength = 0;

	cout << nL << "," << nT << "," << nR << "," << nB << endl;

	try
	{
		TIFFSetDirectory(m_pFile, nLayer);
		TIFFSetField(m_pFile, TIFFTAG_IMAGEWIDTH, m_nWidth/(nLayer+1));
		TIFFSetField(m_pFile, TIFFTAG_IMAGELENGTH, m_nHeight/(nLayer + 1));
		TIFFSetField(m_pFile, TIFFTAG_TILEWIDTH, m_nTileWidth);
		TIFFSetField(m_pFile, TIFFTAG_TILELENGTH, m_nTileHeight);
		TIFFSetField(m_pFile, TIFFTAG_SUBFILETYPE, FILETYPE_REDUCEDIMAGE);
		TIFFSetField(m_pFile, TIFFTAG_BITSPERSAMPLE, 8);
		TIFFSetField(m_pFile, TIFFTAG_SAMPLESPERPIXEL, 3);
		TIFFSetField(m_pFile, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
		TIFFSetField(m_pFile, TIFFTAG_COMPRESSION, COMPRESSION_JPEG);
		TIFFSetField(m_pFile, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
		int nJpegQuality = 60;
		TIFFSetField(m_pFile, TIFFTAG_JPEGQUALITY, nJpegQuality);

		//根据需要设置分辨率
		/*if (nLayer == 0)
		{
			float fPixelSize = 0.00068f;
			int nScale = 20;
			fPixelSize = fPixelSize * 100 / max(1, nScale);
			TIFFSetField(m_pFile, TIFFTAG_XRESOLUTION, 1.0 / (fPixelSize / 10));
			TIFFSetField(m_pFile, TIFFTAG_YRESOLUTION, 1.0 / (fPixelSize / 10));
			TIFFSetField(m_pFile, TIFFTAG_RESOLUTIONUNIT, 3);
		}*/


		// save tile information
		const int nTileRow = nT / m_nTileHeight;
		const int nTileCol = nL / m_nTileWidth;
		int tiffIndex = nTileRow * (m_nWidth / (nLayer+1)/m_nTileWidth) + nTileCol;
		cout << "tiffIndex:" << tiffIndex << "," << nTileRow << "," << nTileCol << ","   << endl;

		TIFFWriteEncodedTile(m_pFile, tiffIndex, (void*)roi.data, m_nTileWidth * m_nTileHeight * 3);

		TIFFWriteDirectory(m_pFile);

		
	}
	catch (const std::exception& e)
	{
		//LOG_E("write tiff error:%s", e.what());
	}

	return true;
}