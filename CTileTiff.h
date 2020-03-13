#pragma once
#include <string>
#include <opencv2/opencv.hpp>
#include "./libtiff/tiff.h"
#include "./libtiff/tiffio.h"


class CTileTiff
{
public:
	CTileTiff();
	~CTileTiff();
	CTileTiff(std::string fileName);
	bool SetTileInfo(int nTileW, int nTileH ,int nLayer, int nHeight, int nWidth);
	bool SaveImage(cv::Mat& img, const int nLeft, const int nTop,
		const int nRight, const int nBottom);

private:
private:
	bool saveTile(cv::Mat& roi, const int nL, const int nT, const int nR, const int nB, const int nLayer);
	int m_nTileWidth;
	int m_nTileHeight;
	int m_nLayers;
	std::string m_fileName;
	TIFF* m_pFile;
	int m_nWidth;
	int m_nHeight;
	
};
