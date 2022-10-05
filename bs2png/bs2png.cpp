#include <iostream>
#include <fstream>
#include <string>
#include <opencv2/opencv.hpp>

#include "mdec.h"

using namespace std;

unsigned char* readFile(const string& inFilename, int* filesize)
{

	ifstream inFile(inFilename.c_str(), ios::in | ios::binary | ios::ate);
	if (!inFile.is_open())
	{
		cout << "  Unable to open input file " << inFilename << "\n";
		exit(1);
	}
	ifstream::pos_type size = inFile.tellg();
	inFile.seekg(0, ios::beg);

	char* buffer = new char[size];
	inFile.read(buffer, size);

	inFile.close();

	*filesize = (int)size;

	return (unsigned char*)buffer;
}

int main(int argc, char** argv)
{
		
	if (argc != 4) {
		cout << "Required arguments: bs_file width height\n";
		return -1;
	}
	
	string inFilename = argv[1];
	int inW = atoi(argv[2]);
	int inH = atoi(argv[3]);
	 
	int size;
	unsigned char *buffer = readFile(inFilename, &size);
	if (buffer == NULL) {
		return -1;
	}

	MdecOutput _mdec;
	memset(&_mdec, 0, sizeof(_mdec));

	static const int w = (inW + 15) & ~15;
	static const int h = (inH + 15) & ~15;
	static const int w2 = w / 2;
	static const int h2 = h / 2;
	_mdec.planes[kOutputPlaneY].ptr = (uint8_t*)malloc(w * h);
	_mdec.planes[kOutputPlaneY].pitch = w;
	_mdec.planes[kOutputPlaneCb].ptr = (uint8_t*)malloc(w2 * h2);
	_mdec.planes[kOutputPlaneCb].pitch = w2;
	_mdec.planes[kOutputPlaneCr].ptr = (uint8_t*)malloc(w2 * h2);
	_mdec.planes[kOutputPlaneCr].pitch = w2;

	decodeMDEC(buffer, size, 0, 0, w, h, &_mdec);

	cv::Size actual_size(w, h);
	cv::Size half_size(w / 2, h / 2);

	cv::Mat y(actual_size, CV_8UC1, (char*)_mdec.planes[kOutputPlaneY].ptr);
	cv::Mat cr(half_size, CV_8UC1, (char*)_mdec.planes[kOutputPlaneCr].ptr);
	cv::Mat cb(half_size, CV_8UC1, (char*)_mdec.planes[kOutputPlaneCb].ptr);

	cv::Mat cr_resized, cb_resized;
	cv::resize(cr, cr_resized, actual_size, 0, 0, cv::INTER_NEAREST); //repeat u values 4 times
	cv::resize(cb, cb_resized, actual_size, 0, 0, cv::INTER_NEAREST); //repeat v values 4 times

	cv::Mat yCrCb;

	std::vector<cv::Mat> yCrCb_channels = { y, cr_resized, cb_resized };
	cv::merge(yCrCb_channels, yCrCb);

	cv::Mat bgr;
	cv::cvtColor(yCrCb, bgr, cv::COLOR_YCrCb2BGR);

	char outFilename[255];
	sprintf(outFilename, "%s.png", inFilename.c_str());
	cv::imwrite(outFilename, bgr);

	return 0;
}
