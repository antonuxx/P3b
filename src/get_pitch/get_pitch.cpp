/// @file

#include <iostream>
#include <fstream>
#include <string.h>
#include <errno.h>
//Librerias extra
#include <cmath>
#include "ffft/FFTReal.h"

#include "wavfile_mono.h"
#include "pitch_analyzer.h"

#include "docopt.h"

#define FRAME_LEN   0.030 /* 30 ms. */
#define FRAME_SHIFT 0.015 /* 15 ms. */

using namespace std;
using namespace upc;

static const char USAGE[] = R"(
get_pitch - Pitch Detector 

Usage:
    get_pitch [options] <input-wav> <output-txt>
    get_pitch (-h | --help)
    get_pitch --version

Options:
    -h, --help  Show this screen
    --version   Show the version of the project

Arguments:
    input-wav   Wave file with the audio signal
    output-txt  Output file: ASCII file with the result of the detection:
                    - One line per frame with the estimated f0
                    - If considered unvoiced, f0 must be set to f0 = 0
)";

int main(int argc, const char *argv[]) {
	/// \TODO 
	///  Modify the program syntax and the call to **docopt()** in order to
	///  add options and arguments to the program.
    std::map<std::string, docopt::value> args = docopt::docopt(USAGE,
        {argv + 1, argv + argc},	// array of arguments, without the program name
        true,    // show help if requested
        "2.0");  // version string

	std::string input_wav = args["<input-wav>"].asString();
	std::string output_txt = args["<output-txt>"].asString();

  // Read input sound file
  unsigned int rate;
  vector<float> x;
  if (readwav_mono(input_wav, rate, x) != 0) {
    cerr << "Error reading input file " << input_wav << " (" << strerror(errno) << ")\n";
    return -2;
  }

  int n_len = rate * FRAME_LEN;
  int n_shift = rate * FRAME_SHIFT;

  // Define analyzer
  PitchAnalyzer analyzer(n_len, rate, PitchAnalyzer::HAMMING, 50, 500);

  // Definimos FFTReal
  ffft::FFTReal <float> fft_object (1024); //Creamos el objeto de la clase FFTReal (dónde estan las funciones)

  /// \TODO
  /// Preprocess the input signal in order to ease pitch estimation. For instance,
  /// central-clipping or low pass filtering may be used.
  float maxwav = *max_element(x.begin(),x.end());
  for (int i = 0; i < x.size(); i++) {
    if(abs(x[i])/maxwav < 0.05) x[i] = 0;
  }

  //Filtrado Paso-Bajo en frecuencia

  //Hacemos la transformada de fourier de la señal x
  std::vector<float> X; ///vector coeficientes frecuencia
  X.resize(x.size());
  fft_object.do_fft(X.data(),x.data());

  
  //Definimos el filtro Paso-Bajo
  std::vector<float> lpf;
  lpf.resize(x.size());
  float cutoff_f = 2000; //frecuencia de corte del filtro
  int k = (cutoff_f/rate)*x.size(); //Posicion de en la DFT

  for (int n = 0;n < k;n++) lpf[n] = 1;
  for(int n = k; n < x.size(); n++) lpf[n] = 0;

/*
  //Calculo del Cepstrum
  //================================================
  float Xa[X.size()];
  std::vector<float> xa; //Vector cesptrum
  xa.resize(x.size());
  for(int n = k; n < x.size(); n++) xa[n] = 0;
  //Calulamos su logaritmo
  for (int n = 0; n < X.size(); n++){
    Xa[n] = log(Xa[n]);
  }
  fft_object.do_ifft(Xa,xa.data());
  //================================================
*/
  //Filtramos la señal
  for(int n = 0 ; n<X.size();n++){
    X[n] *= lpf[n];
  }
  
  //Antitransformamos
  fft_object.do_ifft (X.data(), x.data());    
  fft_object.rescale (x.data());

  // Iterate for each frame and save values in f0 vector
  vector<float>::iterator iX;
  vector<float> f0;
  for (iX = x.begin(); iX + n_len < x.end(); iX = iX + n_shift) {
    float f = analyzer(iX, iX + n_len);
    f0.push_back(f);
  }

  /// \TODO
  /// Postprocess the estimation in order to supress errors. For instance, a median filter
  /// or time-warping may be used.

   /*
  int lengw = 5;
  int center = lengw/2;
  float medianw[lengw];
  for(int i = center; i < f0.size()-center; i++){
    for(int j = -center; j <= center; j++){
      medianw[j+center] = f0[i+j];
    }
    sort(medianw, medianw+lengw);
    f0[i] = medianw[center];
  }
*/

  // Write f0 contour into the output file
  ofstream os(output_txt);
  if (!os.good()) {
    cerr << "Error reading output file " << output_txt << " (" << strerror(errno) << ")\n";
    return -3;
  }

  os << 0 << '\n'; //pitch at t=0
  for (iX = f0.begin(); iX != f0.end(); ++iX) 
    os << *iX << '\n';
  os << 0 << '\n';//pitch at t=Dur

  return 0;
}
