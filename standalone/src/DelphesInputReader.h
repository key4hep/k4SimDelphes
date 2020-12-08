#ifndef DELPHESEDM4HEP_DELPHESINPUTREADER
#define DELPHESEDM4HEP_DELPHESINPUTREADER

#include "TTree.h"

#include <string>

class TObjArray;
class Delphes;

class DelphesInputReader {
public:

  /** Initialize the reader and return the output file name on success or an
   * empty string in the failing case */
  virtual std::string init(Delphes* modularDelphes, int argc, char *argv[]) = 0;

  /** Usage message to be displayed if initialization is not successful */
  virtual std::string getUsage() const = 0;

  virtual int getNumberOfEvents() const = 0;
  virtual bool finished() const = 0;

  virtual bool readEvent(Delphes* modularDelphes,
                         TObjArray* allParticleOutputArray,
                         TObjArray* stableParticleOutputArray,
                         TObjArray* partonOutputArray) = 0;

  virtual TTree* converterTree() = 0;

};


#endif
