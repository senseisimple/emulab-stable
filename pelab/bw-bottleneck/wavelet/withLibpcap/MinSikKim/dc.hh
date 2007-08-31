/*
      Detecting Shared Congestion by wavelets

      Header file for shcon.cpp 
      By Taek H. Kim (thkim@ece.utexas.edu)
      
        Wavelet : Daubechies 12 (db6)
        Thresholding : MINIMAXI soft-thresholding
        Noise Scaling : Multi-level scaling
        Maximum decomposition level : 4

 */

//#include <iostream>
//#include <fstream>
//#include <algo.h>

#include <vector>

bool delay_correlation(const std::vector<double> &delay1,
		       const std::vector<double> &delay2,
		       int nSamples, double threshold = 0.512);
