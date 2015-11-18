/*
 * ChoiceAlgorithms.cpp
 *
 *  Created on: Oct 26, 2015
 *      Author: sceik
 */

#include "ChoiceAlgorithms.h"

ChoiceAlgorithms::ChoiceAlgorithms() {
	// TODO Auto-generated constructor stub

}

ChoiceAlgorithms::~ChoiceAlgorithms() {
	// TODO Auto-generated destructor stub
}
int ChoiceAlgorithms::random(int segmentNumber, int offset, VideoInfo& videoInfo){
	int to_ret = 0;
	do{
		to_ret =   segmentNumber + ( std::rand() % ( offset + 1 ) );
	}while(videoInfo.checkSegment(to_ret));

	return to_ret;
}

int ChoiceAlgorithms::fixed(int segmentNumber, int offset, VideoInfo& videoInfo){
	int to_ret = 0;
	to_ret =   segmentNumber + offset;
	while(videoInfo.checkSegment(to_ret)){
		to_ret++;
	}
	return to_ret;
}

int ChoiceAlgorithms::caba( VideoInfo& videoInfo, long thr){

	//if Thr NaN return last + 1
	if(thr != thr)
		return videoInfo.getLastRequest() + 1;

	long sizeSegment = ((videoInfo.getLastReques_bps() * videoInfo.getSegmentDuration())/ 8000) ;//KB
	long expectedTime = sizeSegment/thr;
	int segmentNumber = videoInfo.getLastRequest() + (expectedTime/videoInfo.getSegmentDuration());
	printf("THREAD:: CABA algorithms    expected Time: %ld %ld %ld \n", expectedTime, thr, sizeSegment);

	if(segmentNumber > videoInfo.getSegmentNumber())
		return 0;

	return segmentNumber;
}

long ChoiceAlgorithms::stepByStep( VideoInfo& videoInfo, long thr_main, long thr){
	long bestQual = 0;

	printf("THREAD:: stepBystep algorithms    expected thr: %ld  \n", thr );
	if(thr != thr)
		return 0;

	long sizeSegment = ((videoInfo.getLastReques_bps() * videoInfo.getSegmentDuration())/ 8000) ;//KB
	long expectedTime = sizeSegment/thr_main;

	long expectedTimeLocal = 0;
	for (std::vector<long>::iterator it = videoInfo.qualityArray.begin() ; it != videoInfo.qualityArray.end(); ++it){
		long sizeSegmentLocal = (((*it) * videoInfo.getSegmentDuration())/ 8000) ;//KB
		expectedTimeLocal = sizeSegmentLocal/thr;
		if(expectedTimeLocal < expectedTime){
			bestQual = *it;
		} else
			break;
	}

	printf("THREAD:: stepBystep algorithms    expected Time: %ld %ld %ld \n", bestQual, expectedTime, expectedTimeLocal );
	return bestQual;
}
