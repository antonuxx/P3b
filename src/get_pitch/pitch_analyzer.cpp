/// @file

#include <iostream>
#include <math.h>
#include "pitch_analyzer.h"

//Añadimos librerias extra
#include <cmath>
#include "ffft/FFTReal.h"

using namespace std;

/// Name space of UPC
namespace upc {
  void PitchAnalyzer::autocorrelation(const vector<float> &x, vector<float> &r) const {
    for (unsigned int l = 0; l < r.size(); ++l) {
  		/// \TODO Compute the autocorrelation r[l]
      for(unsigned int n = 0; n < x.size()-l; n++){
        r[l] += x[n]*x[n+l];
      }
      r[l] /= x.size()-l; //Estimador no Sesgado
    }
    if (r[0] == 0.0F){ //to avoid log() and divide zero
      r[0] = 1e-10;
    }
  }

  void PitchAnalyzer::set_window(Window win_type) {
    if (frameLen == 0)
      return;

    window.resize(frameLen);

    switch (win_type) {
    case HAMMING:
      /// \TODO Implement the Hamming window
      for(int n = 0; n < frameLen; n++){
        window[n] = 0.53836-0.46164*cos((2*M_PI*n)/(frameLen-1));
      }
      break;
    case RECT:
    default:
      window.assign(frameLen, 1);
    }
  }

  void PitchAnalyzer::set_f0_range(float min_F0, float max_F0) {
    npitch_min = (unsigned int) samplingFreq/max_F0;
    if (npitch_min < 2)
      npitch_min = 2;  // samplingFreq/2

    npitch_max = 1 + (unsigned int) samplingFreq/min_F0;

    //frameLen should include at least 2*T0
    if (npitch_max > frameLen/2)
      npitch_max = frameLen/2;
  }

  bool PitchAnalyzer::unvoiced(float r2max, float r1norm, float rmaxnorm,float zcr) const {
    /// \TODO Implement a rule to decide whether the sound is voiced or not.
    /// * You can use the standard features (pot, r1norm, rmaxnorm),
    ///   or compute and use other ones.
    if (r1norm > 0.70){ //max a 70 90.83
     if(rmaxnorm < 0.40 && zcr > 5950) return true; //90.19 a 40; 90.61 a 5950
     else return false;
     }
    else{
      if(r2max > 0.30 && r2max < 0.80) return false;
      //if(zcr < 4500) return false;
      return true;
    }
  }

  float PitchAnalyzer::compute_pitch(vector<float> & x) const {
    if (x.size() != frameLen)
      return -1.0F;

    //Window input frame
    for (unsigned int i=0; i<x.size(); ++i) x[i] *= window[i];

    /// \TODO
	  /// Find the lag of the maximum value of the autocorrelation away from the origin.<br>
	  /// Choices to set the minimum value of the lag are:
	  ///    - The first negative value of the autocorrelation.
	  ///    - The lag corresponding to the maximum value of the pitch.
	  /// In either case, the lag should not exceed that of the minimum value of the pitch.

    // Pitch via autocorrelación
    // ===================================================================================
    vector<float> r(npitch_max);
    autocorrelation(x, r);     //Compute correlation

    unsigned int lag = 0;
    int max = samplingFreq/70;
    int min = samplingFreq/490;
    float maxval = 0;

    for (int n = min -1; n < max+1 ; n++ ){
      if (r[n] >= maxval) {
        lag = n;
        maxval = r[n];
      }
    }
    // =================================================================================== */

    // Tasa de Cruces por Cero
    // ===================================================================================
    float zcr = 0;
    float deltas = 0;
    for (int i = 0; i < x.size(); i++) {
      float sgn = x[i]*x[i-1];
      if(sgn <= 0) deltas += 1;
    }
    zcr = ((deltas*samplingFreq)/(2*(x.size()-1)));
    // ===================================================================================


    //You can print these (and other) features, look at them using wavesurfer
    //Based on that, implement a rule for unvoiced
    //change to #if 1 and compile
#if 0
    if (r[0] > 0.0F)
      cout << pot << '\t' << r[1]/r[0] << '\t' << r[lag]/r[0] << endl;
#endif

    if (unvoiced(r[2*lag]/r[0], r[1]/r[0], r[lag]/r[0],zcr))
      return 0;
    else
      return (float) samplingFreq/(float) lag;
  }
}
