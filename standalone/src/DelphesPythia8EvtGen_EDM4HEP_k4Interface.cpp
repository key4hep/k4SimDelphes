
#include "DelphesMain.h"
#include "DelphesPythia8EvtGenReader_marcin.h"

int main(int argc, char *argv[]) {
  DelphesPythia8EvtGenReader inputReader = DelphesPythia8EvtGenReader();
  return doit(argc, argv, inputReader);
}
