/*
 * ChoiceAlgorithms.h
 *
 *  Created on: Oct 26, 2015
 *      Author: sceik
 */

#ifndef CHOICEALGORITHMS_H_
#define CHOICEALGORITHMS_H_
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include "VideoInfo.h"


class ChoiceAlgorithms {
	public:

		static int random(int segmentNumber, int offset, VideoInfo& videoInfo);
		static int fixed(int segmentNumber, int offset,  VideoInfo& videoInfo);
		static int caba(VideoInfo& videoInfo, long thr);

		ChoiceAlgorithms();
		virtual ~ChoiceAlgorithms();
		//static int (int segmentNumber, int offset);
		//static int (int segmentNumber, int offset);

};

#endif /* CHOICEALGORITHMS_H_ */
